#include "plugin.h"
#include "CTimer.h"
#include "CHud.h"
#include "CMessages.h"
#include "CCamera.h"
#include "fmod.hpp"
#include "fmod_studio.hpp"
#include "fmod_errors.h"
#include "Curve.h"
#include "CModelInfo.h"
#include "CFileLoader.h"

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
FMODAudio* audio;
std::map<CVehicle*, FMODAudio*> audioInstance;
std::map<int, FMODAudio*> audios;
CVehicle* lastVehicle;
INIConfig* iniConfig;

int nLastGearChangeTime = 0;
float fRPM = 800;
int nGear = 0;
int nTargetGear = 1;
float fLastPedal = 0;
int nLastId = -1;
float nLastEngineAccel;

RwTexDictionary* m_txd;
RwTexture* m_logoTex;

static bool CamNoRain()
{
    return ((bool(__thiscall*)())0x72DDB0)();
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
        FMODAudio::CheckError(coreSystem->setSoftwareFormat(0, FMOD_SPEAKERMODE_DEFAULT, 0), "Failed on set software format");
        FMODAudio::CheckError(coreSystem->loadPlugin(PLUGIN_PATH((char*)"plugins\\fmod_distance_filter.dll"), 0, 0), "Failed on load plugin 'plugins\\fmod_distance_filter.dll'");
        
        FMODAudio::CheckError(fmodSystem->initialize(1024, FMOD_STUDIO_INIT_NORMAL, FMOD_INIT_NORMAL, extraDriverData), "Failed to initialize");

        FMOD_REVERB_PROPERTIES props = FMOD_PRESET_HANGAR;
        FMODAudio::CheckError(coreSystem->setReverbProperties(0, &props), "Failed on set reverb");

        //Load banks
        FMOD::Studio::Bank* masterBank = NULL;
        FMODAudio::CheckError(fmodSystem->loadBankFile(PLUGIN_PATH((char*)"banks\\common.bank"), FMOD_STUDIO_LOAD_BANK_NORMAL, &masterBank), "Failed on load bank Master");

        FMOD::Studio::Bank* stringsBank = NULL;
        FMODAudio::CheckError(fmodSystem->loadBankFile(PLUGIN_PATH((char*)"banks\\common.strings.bank"), FMOD_STUDIO_LOAD_BANK_NORMAL, &stringsBank), "Failed on load bank Master String");

        //Projects using FMOD Studio must include an in-app credit line and the FMOD logo.
        if (iniConfig->m_bUseLogo)
        {
            // Load txd and texture
            m_txd = CFileLoader::LoadTexDictionary(PLUGIN_PATH((char*)"logo.txd"));
            m_logoTex = GetFirstTexture(m_txd);

            Events::drawMenuBackgroundEvent += [] {

                // Setup texture
                RwRenderStateSet(rwRENDERSTATETEXTURERASTER, m_logoTex->raster);

                // Background
                CRGBA color = CRGBA(255, 255, 255, 255);

                CSprite2d::SetVertices(CRect(SCREEN_COORD_LEFT(50.0f),
                    SCREEN_COORD_BOTTOM(30.0f + 100.0f), SCREEN_COORD_LEFT(50.0f + 200.0f), SCREEN_COORD_BOTTOM(30.0f)), color, color, color, color);
                RwIm2DRenderPrimitive(rwPRIMTYPETRIFAN, CSprite2d::maVertices, 4);

                // Reset texture
                RwRenderStateSet(rwRENDERSTATETEXTURERASTER, 0);
            };
        }
    }
    static void SetIndexFMODBank(int id, char* bank, char* absolutePath)
    {
        StopCurrentAudio();

        FMODAudio* audio = new FMODAudio();
        audio->LoadBank(fmodSystem, iniConfig, bank, absolutePath);

        audios.insert(std::make_pair(id, audio));
    }
    static void SetVehicleFMODBank(CVehicle* vehicle, char* bank, char* absolutePath)
    {
        if (vehicle == NULL)
            return;

        StopCurrentAudio();
        if (audioInstance[vehicle] == NULL)
        {
            FMODAudio* audio = new FMODAudio();
            audioInstance.insert(std::make_pair(vehicle, audio));
        }
        FMODAudio* audio = new FMODAudio();
        audio->LoadBank(fmodSystem, iniConfig, bank, absolutePath);

        audioInstance[vehicle] = audio;
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
    static void _stdcall OnExitVehicle()
    {
        CVehicle* veh = FindPlayerVehicle(-1, true);
        if (!veh)
        {
            StopCurrentAudio();
        }
    }
    static void _stdcall OnEnterVehicle()
    {
        lastVehicle = FindPlayerVehicle(-1, true);

        if (lastVehicle)
        {
            //Try with instance
            audio = audioInstance[lastVehicle];
            //Try with model index
            if (audio == NULL)
            {
                nLastId = lastVehicle->m_nModelIndex;
                audio = audios[nLastId];
            }
            //Try with sound bank
            if (audio == NULL)
            {
                nLastId = lastVehicle->m_vehicleAudio.m_nEngineAccelerateSoundBankId;
                audio = audios[nLastId];
            }
            
            if (audio == NULL)
            {
                nLastId = lastVehicle->m_vehicleAudio.m_nEngineAccelerateSoundBankId;
                std::string section = "Bank" + std::to_string(nLastId);
                std::string sectionId = "Id" + std::to_string(lastVehicle->m_nModelIndex);
                CIniReader ini("banks\\banks.ini");
                std::string defaultBank = ini.ReadString("EngineBanks", "Default", "");
                std::string custom = ini.ReadString("CarBanks", sectionId, "");
                std::string bank = ini.ReadString("EngineBanks", section, defaultBank);
                std::string path = "banks\\" + bank + ".bank";
                std::string customPath = "banks\\" + custom + ".bank";

                audio = new FMODAudio();
                //Load INI
                if (custom.empty())
                {
                    audio->LoadBank(fmodSystem, iniConfig, (char*)bank.c_str(), PLUGIN_PATH((char*)path.c_str()));
                }
                else
                {
                    audio->LoadBank(fmodSystem, iniConfig, (char*)custom.c_str(), PLUGIN_PATH((char*)customPath.c_str()));
                    nLastId = lastVehicle->m_nModelIndex;
                }

                audios.insert(std::make_pair(nLastId, audio));
                audios[nLastId] = audio;
            }
            if (audio->m_bIsLoaded == false)
            {
                return;
            }
            if (audio->m_bIsPlaying == false)
            {
                fRPM = iniConfig->m_fMinRPM;
                audio->m_RpmEventInstance->start();
                audio->m_bIsPlaying = true;
            }
        }
    }
    static void Backfire(float time)
    {
        if (fRPM > 5200 && CTimer::m_snTimeInMilliseconds > (m_nLastSpawnedTime + time))
        {
            audio->m_BackFireEventInstance->start();
            m_nLastSpawnedTime = CTimer::m_snTimeInMilliseconds;
        }
    }
    static void _stdcall ProcessVehicleEngine(cVehicleParams* params)
    {
        
        if (lastVehicle && !CTimer::m_UserPause && audio)
        {
            lastVehicle->m_vehicleAudio.m_nEngineState = 5;
            cTransmission* m_pTransmission = &lastVehicle->m_pHandlingData->m_transmissionData;
            float velocity = fabs(params->m_fVelocity);
            params->m_nCurrentGear = lastVehicle->m_nCurrentGear;
            params->m_bHandbrakeOn = lastVehicle->m_nVehicleFlags.bIsHandbrakeOn;
            float currentRatio = (velocity - m_pTransmission->m_aGears[nGear].m_fChangeDownVelocity)
                / (*(float*)&m_pTransmission->m_aGears[nGear].m_fMaxVelocity
                    - m_pTransmission->m_aGears[nGear].m_fChangeDownVelocity);
            if (currentRatio > 1.0 || currentRatio >= 0.0)
            {
                if (currentRatio > 1.0)
                    currentRatio = 1.0;
            }
            else
            {
                currentRatio = 0.0;
            }
            params->m_fVelocityChangingPercentage = currentRatio;

            if (!nGear)
                params->m_fVelocityChangingPercentage = 0.0;
           
            //Calculate target RPM 
            float targetRpm = iniConfig->m_fMinRPM + ((iniConfig->m_fMaxRPM - 2000 )* params->m_fVelocityChangingPercentage);

            //Set 3D space position
            CVector camPos = TheCamera.GetPosition();
            CVector vehiclePos = lastVehicle->GetPosition();
            CVector dirFor;
            CVector dirUp;
            CVector offsetFor = CVector(0, -1, 0);
            CVector offsetUp = CVector(0, 0, 1);
            CMatrix* matrix = lastVehicle->m_matrix;
            RwV3dTransformPoint((RwV3d*)&dirFor, (RwV3d*)&offsetFor, (RwMatrix*)matrix);
            RwV3dTransformPoint((RwV3d*)&dirUp, (RwV3d*)&offsetUp, (RwMatrix*)matrix);

            audio->m_Attributes.position.x = vehiclePos.x;
            audio->m_Attributes.position.y = vehiclePos.y;
            audio->m_Attributes.position.z = vehiclePos.z;
            audio->m_Attributes.velocity.x = lastVehicle->m_vecMoveSpeed.x;
            audio->m_Attributes.velocity.y = lastVehicle->m_vecMoveSpeed.y;
            audio->m_Attributes.velocity.z = lastVehicle->m_vecMoveSpeed.z;

            audio->m_Attributes.forward.x = dirFor.x - vehiclePos.x;
            audio->m_Attributes.forward.y = dirFor.y - vehiclePos.y;
            audio->m_Attributes.forward.z = dirFor.z - vehiclePos.z;
            audio->m_Attributes.up.x = dirUp.x - vehiclePos.x;
            audio->m_Attributes.up.y = dirUp.y - vehiclePos.y;
            audio->m_Attributes.up.z = dirUp.z - vehiclePos.z;

            FMOD::ChannelGroup* grp;
            audio->m_RpmEventInstance->getChannelGroup(&grp);
            grp->setReverbProperties(0, CamNoRain() ? 1.0 : 0.0);

            audio->m_RpmEventInstance->set3DAttributes(&audio->m_Attributes);
            audio->m_BackFireEventInstance->set3DAttributes(&audio->m_Attributes);
            audio->m_GearEventInstance->set3DAttributes(&audio->m_Attributes);

            audio->m_ListenerAttributes.position.x = camPos.x;
            audio->m_ListenerAttributes.position.y = camPos.y;
            audio->m_ListenerAttributes.position.z = camPos.z;
            audio->m_ListenerAttributes.forward = audio->m_Attributes.forward;
            audio->m_ListenerAttributes.up = audio->m_Attributes.up;

            fmodSystem->setListenerAttributes(0, &audio->m_ListenerAttributes);

            //Get gas pedal
            float gasPedal = abs(lastVehicle->m_fGasPedal);
            CAutomobile* automobile = reinterpret_cast<CAutomobile*>(lastVehicle);
            bool clutch = automobile->m_nWheelsOnGround == 0 || params->m_bHandbrakeOn || lastVehicle->m_fWheelSpinForAudio > 0.6f;

            //Gear change time
            if (CTimer::m_snTimeInMilliseconds < (nLastGearChangeTime + (iniConfig->m_fGearTime * 0.5)))
            {
                clutch = true;
                gasPedal = 0;
            }
            if (CTimer::m_snTimeInMilliseconds >= (nLastGearChangeTime + (iniConfig->m_fGearTime * 0.5)))
            {
                if (nGear != nTargetGear)
                {
                    lastVehicle->m_nVehicleFlags.bAudioChangingGear = true;
                    audio->m_GearEventInstance->start();
                }
                nGear = nTargetGear;
            }
            if (CTimer::m_snTimeInMilliseconds >= (nLastGearChangeTime + (iniConfig->m_fGearTime * 0.8)))
            {
                lastVehicle->m_nVehicleFlags.bAudioChangingGear = false;
            }
            if (nTargetGear != params->m_nCurrentGear)
            {
                nTargetGear = params->m_nCurrentGear;
                nLastGearChangeTime = CTimer::m_snTimeInMilliseconds;
            }

            if (clutch > 0)
            {
                if (fRPM < iniConfig->m_fMaxRPM)
                {
                    fRPM += (gasPedal * CTimer::ms_fTimeStep) * (iniConfig->m_fRPMAcceleration * 2) * clutch;
                }
                if (fRPM > iniConfig->m_fMaxRPM)
                {
                    fRPM -= (gasPedal * CTimer::ms_fTimeStep) * (iniConfig->m_fRPMDesaceleration * 3) * clutch;
                }
                if (gasPedal == 0 && fRPM > iniConfig->m_fMinRPM)
                {
                    fRPM -= (CTimer::ms_fTimeStep) * (iniConfig->m_fRPMDesaceleration / 2);
                }
            }
            else
            {
                if (fRPM < targetRpm)
                {
                    fRPM += (CTimer::ms_fTimeStep)*iniConfig->m_fRPMAcceleration;
                }
                if (fRPM > targetRpm)
                {
                    fRPM -= (CTimer::ms_fTimeStep)*iniConfig->m_fRPMDesaceleration;
                }
            }

            //Set FMOD parameters
            audio->m_RpmEventInstance->setParameterByID(audio->m_RpmDesc.id, fRPM);
            audio->m_RpmEventInstance->setParameterByID(audio->m_LoadDesc.id, -1 + (gasPedal * 2));

            if (clutch > 0 && fRPM >= iniConfig->m_fMaxRPM)
            {
                Backfire(250);
            }

            //Backfire event
            if (gasPedal != fLastPedal)
            {
                if (gasPedal == 0)
                {
                    Backfire(4000);
                }
            }
            fLastPedal = gasPedal;
        }
    }

    GTAFmod() {
        //Replace Engine Events
        patch::ReplaceFunctionCall(0x5021C2, ProcessVehicleEngine);
        patch::ReplaceFunctionCall(0x501FD6, ProcessVehicleEngine);

        //OnEnterVehicle Event
        injector::MakeCALL(0x64BBC4, OnEnterVehicle);

        //OnExitVehicle Event
        injector::MakeCALL(0x647D52, OnExitVehicle);

        //Initialize FMOD on game start
        Events::initGameEvent.after.Add(LoadConfigs);

        //Update FMOD on game Process
        Events::gameProcessEvent += []() {
            if (audio)
            {
                audio->m_RpmEventInstance->setVolume(CTimer::m_UserPause ? 0.0 : iniConfig->m_fVolume);
            }
            //Update FMOD
            FMODAudio::CheckError(fmodSystem->update(), "Update Failed");
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