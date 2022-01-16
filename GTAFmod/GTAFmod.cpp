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

using namespace plugin;

int m_nLastSpawnedTime;
FMOD::Studio::EventInstance* rpmEventInstance = NULL;
FMOD::Studio::EventInstance* backFireEventInstance = NULL;
FMOD::Studio::EventInstance* gearEventInstance = NULL;
FMOD::Studio::System* fmodSystem = NULL;
FMOD_STUDIO_PARAMETER_DESCRIPTION rpmDesc;
FMOD_STUDIO_PARAMETER_DESCRIPTION loadDesc;
FMOD_3D_ATTRIBUTES attributes;

int nLastGearChangeTime = 0;
unsigned int bAutomatic = 0;
float fClutch = 0;
float fRPM = 800;
int nGear = 0;
int nTargetGear = 1;
std::fstream lg;
float fLastPedal = 0;
bool engineState = true;

static bool bPlaying;
class GTAFmod {
public:

    static void UpdateFmod() {
        if (KeyPressed(VK_F8) && CTimer::m_snTimeInMilliseconds > (m_nLastSpawnedTime + 200))
        {
            InitializeFmod();
        }
    }
    static void CheckError(FMOD_RESULT result, const char* text)
    {
        if (result != FMOD_OK && CTimer::m_snTimeInMilliseconds > (m_nLastSpawnedTime + 1000))
        {
            CHud::SetHelpMessage(text, true, false, false);
            lg << "Error\n";
            lg << FMOD_ErrorString(result);
            lg << "\n";
            lg << text;
            lg << "\n---\n";
            lg.flush();
            m_nLastSpawnedTime = CTimer::m_snTimeInMilliseconds;
        }
    }
    static void InitializeFmod()
    {
        void* extraDriverData = NULL;
        CheckError(FMOD::Studio::System::create(&fmodSystem), "Failed on create FMOD System");

        // The example Studio project is authored for 5.1 sound, so set up the system output mode to match
        FMOD::System* coreSystem = NULL;
        CheckError(fmodSystem->getCoreSystem(&coreSystem), "Failed on create FMOD CORE System");
        CheckError(coreSystem->setSoftwareFormat(0, FMOD_SPEAKERMODE_5POINT1, 0), "Failed on set software format");
        CheckError(coreSystem->setPluginPath(GAME_PATH((char*)"modloader\\GTAFmod\\plugins")), "Failed on set path");
        CheckError(coreSystem->loadPlugin("fmod_distance_filterL.dll", 0, 0), "Failed on load plugin");

        CheckError(fmodSystem->initialize(1024, FMOD_STUDIO_INIT_NORMAL, FMOD_INIT_NORMAL, extraDriverData), "Failed to initialize");

        FMOD::Studio::Bank* masterBank = NULL;
        CheckError(fmodSystem->loadBankFile(GAME_PATH((char*)"modloader\\GTAFmod\\banks\\common.bank"), FMOD_STUDIO_LOAD_BANK_NORMAL, &masterBank), "Failed on load bank Master");

        FMOD::Studio::Bank* stringsBank = NULL;
        CheckError(fmodSystem->loadBankFile(GAME_PATH((char*)"modloader\\GTAFmod\\banks\\common.strings.bank"), FMOD_STUDIO_LOAD_BANK_NORMAL, &stringsBank), "Failed on load bank Master String");

        FMOD::Studio::Bank* vehiclesBank = NULL;
        CheckError(fmodSystem->loadBankFile(GAME_PATH((char*)"modloader\\GTAFmod\\banks\\ks_toyota_ae86.bank"), FMOD_STUDIO_LOAD_BANK_NORMAL, &vehiclesBank), "Failed on load bank SFX");

        attributes = { { 0 } };
        attributes.forward.z = 1.0f;
        attributes.up.y = 1.0f;
        CheckError(fmodSystem->setListenerAttributes(0, &attributes), "Failed on set 3d ambient");

        //Load events
        FMOD::Studio::EventDescription* rpmEventDescription = NULL;
        CheckError(fmodSystem->getEvent("event:/cars/ks_toyota_ae86/engine_ext", &rpmEventDescription), "Failed on get event");

        FMOD::Studio::EventDescription* backFireEventDescription = NULL;
        CheckError(fmodSystem->getEvent("event:/cars/ks_toyota_ae86/backfire_int", &backFireEventDescription), "Failed on get event");

        FMOD::Studio::EventDescription* gearEventDescription = NULL;
        CheckError(fmodSystem->getEvent("event:/cars/ks_toyota_ae86/gear_ext", &gearEventDescription), "Failed on get event");

        CheckError(rpmEventDescription->getParameterDescriptionByName("rpms", &rpmDesc), "Failed on get parameter");
        CheckError(rpmEventDescription->getParameterDescriptionByName("throttle", &loadDesc), "Failed on get parameter");
        
        rpmEventInstance = NULL;
        CheckError(rpmEventDescription->createInstance(&rpmEventInstance), "Failed on create instance");

        backFireEventInstance = NULL;
        CheckError(backFireEventDescription->createInstance(&backFireEventInstance), "Failed on create instance");

        gearEventInstance = NULL;
        CheckError(gearEventDescription->createInstance(&gearEventInstance), "Failed on create instance");

        lg << "8";
        lg.flush();

        rpmEventInstance->set3DAttributes(&attributes);
        rpmEventInstance->setVolume(.5f);
        rpmEventInstance->setParameterByID(rpmDesc.id, 0.0f);
        rpmEventInstance->setParameterByID(loadDesc.id, 1.0f);
        rpmEventInstance->start();
        CHud::SetHelpMessage(rpmDesc.name, true, false, false);
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
    static void TurnEngine(CVehicle* v, bool value)
    {
        engineState = value;
        ((void(__thiscall*)(CVehicle*, bool))0x41BDD0)(v, value);
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

    GTAFmod() {
        //CTorqueCurve->Draw(CRect(100, 100, 500, 300), 2);

        lg.open("GTAFmod.log", std::fstream::out | std::fstream::trunc);
        //Events::gameProcessEvent.Add(UpdateFmod);
        Events::initGameEvent.after.Add(InitializeFmod);
        Events::gameProcessEvent.before += [](){
            //CTorqueCurve->Render();
            CVector2D points[] = {
                CVector2D(0, 0.8),
                CVector2D(4500, 1.2),
                CVector2D(8000, 0.1)
            };
            Curve torqueCurve = Curve(points);
            CVehicle* vehicle = FindPlayerVehicle(-1, true);
            if (vehicle && vehicle->m_nVehicleSubClass != VEHICLE_PLANE && vehicle->m_nVehicleSubClass != VEHICLE_HELI
                && vehicle->m_nVehicleSubClass != VEHICLE_BOAT && vehicle->m_nVehicleSubClass != VEHICLE_BMX &&
                vehicle->m_nVehicleSubClass != VEHICLE_TRAIN && CTimer::m_UserPause == false)
            {
                if (bPlaying == false)
                {
                    rpmEventInstance->start();
                    bPlaying = true;
                }
                CVector camPos = TheCamera.GetPosition();
                CVector vehiclePos = vehicle->GetPosition();

                attributes.position.x = vehiclePos.x - camPos.x;
                attributes.position.y = vehiclePos.y - camPos.y;
                attributes.position.z = vehiclePos.z - camPos.z;
                rpmEventInstance->set3DAttributes(&attributes);
                backFireEventInstance->set3DAttributes(&attributes);
                gearEventInstance->set3DAttributes(&attributes);

                float gasPedal = vehicle->m_fGasPedal;
                if (CTimer::m_snTimeInMilliseconds < (nLastGearChangeTime + 800))
                {
                    gasPedal = 0;
                }
                if (CTimer::m_snTimeInMilliseconds >= (nLastGearChangeTime + 800))
                {
                    if (nGear != nTargetGear)
                    {
                        gearEventInstance->start();
                    }
                    nGear = nTargetGear;
                    fClutch = 0;
                    gasPedal = vehicle->m_fGasPedal;
                }
                if (KeyPressed(VK_F7))
                {
                    fClutch = 1;
                }

                vehicle->m_vehicleAudio.m_nEngineState = -1;

                float relation[] = {
                    -3, 0, 3, 2, 1.6, 1.2, 0.9, 0.7
                };

                float torqueBias = 0;
                float speed = vehicle->m_vecMoveSpeed.Magnitude() * 175;
                float targetRpm = 400 + (vehicle->m_vecMoveSpeed.Magnitude() * abs(relation[nGear + 1])) * 6000;

                if (nGear == 0)
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
                        fRPM += (CTimer::ms_fTimeStep) * 50;
                        if (fRPM > torqueCurve.maxX)
                        {
                            vehicle->m_fHealth -= 50.0;
                        }
                    }
                    else
                    {
                        fRPM -= (CTimer::ms_fTimeStep) * 20;
                    }
                }
                if (fRPM < 300 && fClutch == 0 && engineState == true)
                {
                    TurnEngine(vehicle, false);
                    CHud::SetHelpMessage("Engine OFF", true, false, false);
                }
                if (KeyPressed(VK_SHIFT) && CTimer::m_snTimeInMilliseconds > (nLastGearChangeTime + 200))
                {
                    NextGear();
                }
                if (KeyPressed(VK_F8) && CTimer::m_snTimeInMilliseconds > (m_nLastSpawnedTime + 2000))
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
                }
                if (KeyPressed(VK_F6) && CTimer::m_snTimeInMilliseconds > (m_nLastSpawnedTime + 200))
                {
                    if (bAutomatic == 0)
                    {
                        bAutomatic = 1;
                        CHud::SetHelpMessage("Automatic: ON", true, false, false);
                    }
                    else
                    {
                        bAutomatic = 0;
                        CHud::SetHelpMessage("Automatic: OFF", true, false, false);
                    }
                    m_nLastSpawnedTime = CTimer::m_snTimeInMilliseconds;
                }
                if (KeyPressed(VK_CONTROL) && CTimer::m_snTimeInMilliseconds > (nLastGearChangeTime + 200))
                {
                    PrevGear();
                }
                if (bAutomatic == 1 && CTimer::m_snTimeInMilliseconds > (nLastGearChangeTime + 1500) && fClutch == 0)
                {
                    if (gasPedal > 0 && fRPM > 5500)
                    {
                        NextGear();
                    }
                    if (gasPedal > 0 && fRPM < 1000)
                    {
                        PrevGear();
                    }
                    if (gasPedal == 0 && fRPM > 3000)
                    {
                        PrevGear();
                    }
                }
                //CMessages::AddMessageJumpQWithNumber(new char[] {"TORQUE ~1~"}, 3000, 0, cv.Evaluate(fRPM) , nTargetGear, speed, fClutch, gasPedal, 0, false);
                char sGear;
                if (nGear > 0)
                {
                    sGear = nGear;
                }

                torqueBias = torqueCurve.Evaluate(fRPM) * 0.002;
                if (fRPM > torqueCurve.maxX)
                {
                    torqueBias = -0.1 * (fRPM / 8000);
                }
                if (nGear == -1)
                {
                    sGear = 'R';
                    torqueBias *= -1;
                }
                if (nGear == 0)
                {
                    sGear = 'N';
                    torqueBias = 0;
                }
                //torqueBias *= relation[nGear];
                //torqueBias = ((relation[nGear-1] - speed) / relation[nGear-1]);

                vehicle->m_pHandlingData->m_transmissionData.m_fEngineAcceleration = (torqueBias * (1 - fClutch)) * gasPedal * (vehicle->m_fHealth / 1000);
                //vehicle->m_pHandlingData->m_fTractionMultiplier = torqueBias > 0.1 ? 0.5 : 1;
                

                CMessages::AddMessageJumpQWithNumber(new char[] {"fRPM ~1~ sGear ~1~ TRPM ~1~"}, 3000, 0, fRPM, sGear, torqueBias * 1000, 0, 0, 0, false);

                float soundRpm = fRPM;
                if (soundRpm < 800)
                {
                    soundRpm = 800;
                }
                if (soundRpm > 9000)
                {
                    soundRpm = 9000;
                }
                if (engineState == false)
                    soundRpm = 0;

                rpmEventInstance->setParameterByID(rpmDesc.id, soundRpm);
                rpmEventInstance->setParameterByID(loadDesc.id, -1 + (gasPedal * 2));

                CheckError(fmodSystem->update(), "Update Failed");

                if (gasPedal != fLastPedal)
                {
                    if (gasPedal == 0)
                    {
                        if (fRPM > 5000 && CTimer::m_snTimeInMilliseconds > (m_nLastSpawnedTime + 1000))
                        {
                            backFireEventInstance->start();
                            m_nLastSpawnedTime = CTimer::m_snTimeInMilliseconds;
                        }
                    }
                }
                fLastPedal = gasPedal;
            }
            else
            {
                if (bPlaying == true)
                {
                    rpmEventInstance->stop(FMOD_STUDIO_STOP_IMMEDIATE);
                    CheckError(fmodSystem->update(), "Update Failed");
                    bPlaying = false;
                }
            }
        };
    }
} gTAFmod;