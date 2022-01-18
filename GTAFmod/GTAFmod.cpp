#include "plugin.h"
#include "CTimer.h"
#include "CHud.h"
#include "CMessages.h"
#include "CCamera.h"
#include "fmod.hpp"
#include "fmod_studio.hpp"
#include "fmod_errors.h"
#include "Curve.h"

#include <stdio.h>
#include <string>
#include <iomanip>
#include <sstream>
#include "FMODAudio.h"
#include "IniReader/IniReader.h"
#include "INIConfig.h"

using namespace plugin;

int m_nLastSpawnedTime;
FMOD::Studio::System* fmodSystem = NULL;
FMODAudio* audios[600];
std::map<CVehicle*, FMODAudio*> audioInstance;
CVehicle* lastVehicle;
INIConfig* iniConfig;

int nLastGearChangeTime = 0;
float fClutch = 0;
float fRPM = 800;
int nGear = 0;
int nTargetGear = 1;
float fLastPedal = 0;
bool bEngineState = true;
bool bFmodBusy = false;
int nLastId = -1;

void CAEVehicleAudioEntity__StoppedUsingBankSlot(int id)
{
    return ((void(__cdecl*)(int))0x4F4DF0)(id);
}
void CVehicle__CancelVehicleEngineSound(CAEVehicleAudioEntity* _this, int soundId)
{
    return ((void(__thiscall*)(CAEVehicleAudioEntity*, int))0x4F55C0)(_this, soundId);
}
void TurnEngine(CVehicle* v, bool value)
{
    bEngineState = value;
    ((void(__thiscall*)(CVehicle*, bool))0x41BDD0)(v, value);
}

class GTAFmod {
public:
    static void LoadConfigs()
    {
        iniConfig = new INIConfig("config.ini");
        InitializeFmod();
    }
    static void InitializeFmod()
    {

        void* extraDriverData = NULL;
        FMODAudio::CheckError(FMOD::Studio::System::create(&fmodSystem), "Failed on create FMOD System");

        //Initialize Core System
        FMOD::System* coreSystem = NULL;
        FMODAudio::CheckError(fmodSystem->getCoreSystem(&coreSystem), "Failed on create FMOD CORE System");
        FMODAudio::CheckError(coreSystem->setSoftwareFormat(0, FMOD_SPEAKERMODE_5POINT1, 0), "Failed on set software format");
        FMODAudio::CheckError(coreSystem->setPluginPath(PLUGIN_PATH((char*)"plugins")), "Failed on set path");
        FMODAudio::CheckError(coreSystem->loadPlugin("fmod_distance_filterL.dll", 0, 0), "Failed on load plugin");

        FMODAudio::CheckError(fmodSystem->initialize(1024, FMOD_STUDIO_INIT_NORMAL, FMOD_INIT_NORMAL, extraDriverData), "Failed to initialize");

        //Load banks
        FMOD::Studio::Bank* masterBank = NULL;
        FMODAudio::CheckError(fmodSystem->loadBankFile(PLUGIN_PATH((char*)"banks\\common.bank"), FMOD_STUDIO_LOAD_BANK_NORMAL, &masterBank), "Failed on load bank Master");

        FMOD::Studio::Bank* stringsBank = NULL;
        FMODAudio::CheckError(fmodSystem->loadBankFile(PLUGIN_PATH((char*)"banks\\common.strings.bank"), FMOD_STUDIO_LOAD_BANK_NORMAL, &stringsBank), "Failed on load bank Master String");
    }
    static void PrevGear()
    {
        nTargetGear -= 1;
        if (nTargetGear < -1)
            nTargetGear = -1;

        if (nGear != nTargetGear)
        {
            nLastGearChangeTime = CTimer::m_snTimeInMilliseconds;
            fClutch = 1;
        }
    }
    static void SetToGear(int gear)
    {
        nTargetGear = gear;
        if (nGear != nTargetGear)
        {
            nLastGearChangeTime = CTimer::m_snTimeInMilliseconds;
            fClutch = 1;
        }
    }
    static void NextGear()
    {
        nTargetGear += 1;
        if (nTargetGear > 5)
            nTargetGear = 5;

        if (nGear != nTargetGear)
        {
            nLastGearChangeTime = CTimer::m_snTimeInMilliseconds;
            fClutch = 1;
        }
    }
    static void SetIndexFMODBank(int id, char* bank, char* absolutePath)
    {
        StopCurrentAudio();

        FMODAudio* audio = new FMODAudio();
        audio->LoadBank(fmodSystem, bank, absolutePath);

        audios[id] = audio;
    }
    static void SetVehicleFMODBank(CVehicle* vehicle, char* bank, char* absolutePath)
    {
        if (vehicle == NULL)
            return;
        bFmodBusy = true;

        StopCurrentAudio();
        if (audioInstance[vehicle] == NULL)
        {
            FMODAudio* audio = new FMODAudio();
            audioInstance.insert(std::make_pair(vehicle, audio));
        }
        FMODAudio* audio = new FMODAudio();
        audio->LoadBank(fmodSystem, bank, absolutePath);

        audioInstance[vehicle] = audio;

        bFmodBusy = false;
    }
    static void StopCurrentAudio()
    {
        if (lastVehicle != NULL)
        {
            if (audioInstance[lastVehicle] != NULL)
            {
                FMODAudio* audio = audioInstance[lastVehicle];
                audio->m_RpmEventInstance->stop(FMOD_STUDIO_STOP_IMMEDIATE);
                audio->m_bIsPlaying = false;
                FMODAudio::CheckError(fmodSystem->update(), "Update Failed");
                lastVehicle = NULL;
            }
        }

        //Stop FMOD when exiting the vehicle
        if (nLastId != -1)
        {
            FMODAudio* audio = audios[nLastId];
            audio->m_RpmEventInstance->stop(FMOD_STUDIO_STOP_IMMEDIATE);
            audio->m_bIsPlaying = false;
            FMODAudio::CheckError(fmodSystem->update(), "Update Failed");
            nLastId = -1;
        }
    }

    GTAFmod() {
        //Initialize FMOD on game start
        Events::initGameEvent.after.Add(LoadConfigs);

        //Update FMOD
        Events::gameProcessEvent += [](){
            //Torque Curve
            CVector2D points[] = {
                CVector2D(iniConfig->m_fMinRPM, iniConfig->m_fMinRPMTorque),
                CVector2D(iniConfig->m_fMidRPM, iniConfig->m_fMidRPMTorque),
                CVector2D(iniConfig->m_fFinalRPM, iniConfig->m_fFinalRPMTorque)
            };
            Curve torqueCurve = Curve(points);

            //Find Vehicle
            CVehicle* vehicle = FindPlayerVehicle(-1, true);
            CAutomobile* automobile = reinterpret_cast<CAutomobile*>(vehicle);

            if (vehicle && vehicle->m_nVehicleSubClass != VEHICLE_PLANE && vehicle->m_nVehicleSubClass != VEHICLE_HELI
                && vehicle->m_nVehicleSubClass != VEHICLE_BOAT && vehicle->m_nVehicleSubClass != VEHICLE_BMX &&
                vehicle->m_nVehicleSubClass != VEHICLE_TRAIN && CTimer::m_UserPause == false)
            {
                if (bFmodBusy)
                    return;

                nLastId = -1;
                //Try with instance
                FMODAudio* audio = audioInstance[vehicle];
                //Try with sound bank
                if (audio == NULL)
                {
                    nLastId = vehicle->m_vehicleAudio.m_nEngineAccelerateSoundBankId;
                    audio = audios[nLastId];
                }
                //Try with model index
                if (audio == NULL)
                {
                    nLastId = vehicle->m_nModelIndex;
                    audio = audios[nLastId];
                }

                if (audio == NULL)
                {
                    nLastId = vehicle->m_vehicleAudio.m_nEngineAccelerateSoundBankId;
                    std::string section = "Bank" + std::to_string(nLastId);
                    std::string sectionId = "Id" + std::to_string(vehicle->m_nModelIndex);
                    CIniReader ini("banks\\banks.ini");
                    std::string defaultBank = ini.ReadString("EngineBanks", "Default", "");
                    std::string custom = ini.ReadString("CarBanks", sectionId, "");
                    std::string bank = ini.ReadString("EngineBanks", section, defaultBank);
                    std::string path = "banks\\" + bank + ".bank";

                    audio = new FMODAudio();
                    //Load INI
                    if (custom.empty())
                    {
                        audio->LoadBank(fmodSystem, (char*)bank.c_str(), PLUGIN_PATH((char*)path.c_str()));
                    }
                    else
                    {
                        audio->LoadBank(fmodSystem, (char*)custom.c_str(), PLUGIN_PATH((char*)path.c_str()));
                        nLastId = vehicle->m_nModelIndex;
                    }

                    audios[nLastId] = audio;
                }
                if (audio->m_bIsLoaded == false)
                {
                    return;
                }
                if (audio->m_bIsPlaying == false)
                {
                    fRPM = 800;
                    audio->m_RpmEventInstance->start();
                    audio->m_bIsPlaying = true;
                }

                //Set 3D space position
                CVector camPos = TheCamera.GetPosition();
                CVector vehiclePos = vehicle->GetPosition();

                audio->m_Attributes.position.x = vehiclePos.x - camPos.x;
                audio->m_Attributes.position.y = vehiclePos.y - camPos.y;
                audio->m_Attributes.position.z = vehiclePos.z - camPos.z;
                audio->m_RpmEventInstance->set3DAttributes(&audio->m_Attributes);
                audio->m_BackFireEventInstance->set3DAttributes(&audio->m_Attributes);
                audio->m_GearEventInstance->set3DAttributes(&audio->m_Attributes);

                //Get gas pedal
                float gasPedal = abs(vehicle->m_fGasPedal);

                //Gear change time
                if (CTimer::m_snTimeInMilliseconds < (nLastGearChangeTime + 800))
                {
                    gasPedal = 0;
                }
                if (CTimer::m_snTimeInMilliseconds >= (nLastGearChangeTime + 800))
                {
                    if (nGear != nTargetGear)
                    {
                        audio->m_GearEventInstance->start();
                    }
                    nGear = nTargetGear;
                    fClutch = 0;
                    gasPedal = abs(vehicle->m_fGasPedal);
                }
                //Clutch Key
                if (KeyPressed(iniConfig->m_nClutchGearKey))
                {
                    fClutch = 1;
                }

                //Remove Vehicle default Sound
                vehicle->m_vehicleAudio.m_nEngineState = -1;

                //Gear relation
                float relation[] = {
                    iniConfig->m_fRalationR,
                    iniConfig->m_fRalationN,
                    iniConfig->m_fRalation1,
                    iniConfig->m_fRalation2,
                    iniConfig->m_fRalation3,
                    iniConfig->m_fRalation4,
                    iniConfig->m_fRalation5
                };

                //Calculate Speed
                float speed = vehicle->m_vecMoveSpeed.Magnitude() * 175;

                //Calculate target RPM
                float targetRpm = 400 + (vehicle->m_vecMoveSpeed.Magnitude() * abs(relation[nGear + 1])) * (iniConfig->m_fFinalRPM - 2000);

                //If Neutral (0) is equals clutch
                if (nGear == 0 || automobile->m_nWheelsOnGround == 0)
                    fClutch = 1;

                if (fClutch > 0)
                {
                    fRPM += (gasPedal * CTimer::ms_fTimeStep) * 200 * fClutch;
                    if (fRPM > torqueCurve.maxX)
                    {
                        fRPM = torqueCurve.maxX;
                    }
                    if (gasPedal == 0 && fRPM > 800)
                    {
                        fRPM -= (CTimer::ms_fTimeStep) * 20;
                    }
                }
                else
                {
                    if (fRPM < targetRpm)
                    {
                        fRPM += (CTimer::ms_fTimeStep) * iniConfig->m_fRPMAcceleration;
                        if (iniConfig->m_bDamageWhenShiftingWrong)
                        {
                            if (fRPM > torqueCurve.maxX + iniConfig->m_fMaxRPMDamage)
                            {
                                vehicle->m_fHealth -= (CTimer::ms_fTimeStep) * iniConfig->m_fDamageMultiply;
                            }
                        }
                    }
                    else
                    {
                        fRPM -= (CTimer::ms_fTimeStep) *iniConfig->m_fRPMDesaceleration;
                    }
                }
                //Engine off if RPM is less than 300
                /*if (fRPM < 300 && fClutch == 0 && engineState == true)
                {
                    TurnEngine(vehicle, false);
                    CHud::SetHelpMessage("Engine OFF", true, false, false);
                }*/
                //Next Gear Key
                if (KeyPressed(iniConfig->m_nUpGearKey) && CTimer::m_snTimeInMilliseconds > (nLastGearChangeTime + 200))
                {
                    NextGear();
                }
                //Prev Gear
                if (KeyPressed(iniConfig->m_nDownGearKey) && CTimer::m_snTimeInMilliseconds > (nLastGearChangeTime + 200))
                {
                    PrevGear();
                }
                //Engine on/off Key
                /*if (KeyPressed(VK_F8) && CTimer::m_snTimeInMilliseconds > (m_nLastSpawnedTime + 2000))
                {
                    if (engineState == false)
                    {
                        TurnEngine(vehicle, true);
                        CHud::SetHelpMessage("Engine: ON", true, false, false);
                    }
                    else
                    {
                        TurnEngine(vehicle, false);
                        CHud::SetHelpMessage("Engine: OFF", true, false, false);
                    }
                    m_nLastSpawnedTime = CTimer::m_snTimeInMilliseconds;
                }*/
                //Automatic Gearbox 
                if (iniConfig->m_bAutomaticGearbox)
                {
                    if (nGear < 5 && fRPM > 6500 && CTimer::m_snTimeInMilliseconds > (nLastGearChangeTime + 800) && fClutch == 0)
                    {
                        NextGear();
                    }
                    if (nTargetGear > 1 && fRPM < 3000 && fClutch == 0)
                    {
                        PrevGear();
                    }
                }

                //Evaluate Torque Curve
                float torqueBias = torqueCurve.Evaluate(fRPM) * 0.002;
                if (fRPM > torqueCurve.maxX)
                {
                    torqueBias = -0.1 * (fRPM / iniConfig->m_fFinalRPM);
                }
                if (nGear == -1)
                {
                    if (torqueBias > 0)
                        torqueBias *= -1;
                }
                //Set engine acceleration
                vehicle->m_pHandlingData->m_transmissionData.m_fEngineAcceleration = (torqueBias * (1 - fClutch)) * gasPedal * (vehicle->m_fHealth / 1000);
               
                //Clamp RPM sound
                float soundRpm = fRPM * iniConfig->m_fPitchMultiply;
                if (soundRpm < iniConfig->m_fMinRPM)
                {
                    soundRpm = iniConfig->m_fMinRPM;
                }
                if (soundRpm > iniConfig->m_fFinalRPM + 1000)
                {
                    soundRpm = iniConfig->m_fFinalRPM + 1000;
                }
                if (bEngineState == false)
                    soundRpm = 0;

                //Set FMOD parameters
                audio->m_RpmEventInstance->setParameterByID(audio->m_RpmDesc.id, soundRpm);
                audio->m_RpmEventInstance->setParameterByID(audio->m_LoadDesc.id, -1 + (gasPedal * 2));

                //CMessages::AddMessageJumpQWithNumber(new char[] {"RPM ~1~ Gear ~1~ G ~1~"}, 3000, 0, lastId, soundRpm, fRPM, 0, 0, 0, false);

                //Backfire event
                if (gasPedal != fLastPedal)
                {
                    if (gasPedal == 0)
                    {
                        if (fRPM > 5000 && CTimer::m_snTimeInMilliseconds > (m_nLastSpawnedTime + 1000))
                        {
                            audio->m_BackFireEventInstance->start();
                            m_nLastSpawnedTime = CTimer::m_snTimeInMilliseconds;
                        }
                    }
                }
                lastVehicle = vehicle;
                fLastPedal = gasPedal;

                //Update FMOD
                FMODAudio::CheckError(fmodSystem->update(), "Update Failed");
            }
            else
            {
                StopCurrentAudio();
            }
        };
    }
} gTAFmod;

//Exports
extern "C" float __declspec(dllexport) Ext_GetCurrentRPM()
{
    return fRPM;
}
extern "C" int __declspec(dllexport) Ext_GetCurrentGear()
{
    return nGear;
}
extern "C" void __declspec(dllexport) Ext_SetCurrentGear(int gear)
{
    return GTAFmod::SetToGear(gear);
}
extern "C" void __declspec(dllexport) Ext_SetClutchValue(float value)
{
    fClutch = value;
}
extern "C" float __declspec(dllexport) Ext_GetClutchValue()
{
    return fClutch;
}

extern "C" void __declspec(dllexport) Ext_SetEngineSoundIdFMODBank(int id, char* bank, char* absolutePath)
{
    return GTAFmod::SetIndexFMODBank(id, bank, absolutePath);
}
extern "C" void __declspec(dllexport) Ext_SetModelIdFMODBank(int id, char* bank, char* absolutePath)
{
    return GTAFmod::SetIndexFMODBank(id, bank, absolutePath);
}
extern "C" void __declspec(dllexport) Ext_SetVehicleFMODBank(CVehicle* vehicle, char* bank, char* absolutePath)
{
    return GTAFmod::SetVehicleFMODBank(vehicle, bank, absolutePath);
}