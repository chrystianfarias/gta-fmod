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

unsigned int m_nLastGearChangeTime = 0;
unsigned int automatic = 0;
float clutch = 0;
float rpm = 800;
int gear = 1;
int targetGear = 1;
std::fstream lg;
float lastPedal = 0;

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
        targetGear -= 1;
        if (targetGear < 1)
            targetGear = 1;

        if (gear != targetGear)
        {
            m_nLastGearChangeTime = CTimer::m_snTimeInMilliseconds;
            clutch = 1;
        }
    }
    static void NextGear()
    {
        targetGear += 1;
        if (targetGear > 5)
            targetGear = 5;

        if (gear != targetGear)
        {
            m_nLastGearChangeTime = CTimer::m_snTimeInMilliseconds;
            clutch = 1;
        }
    }

    GTAFmod() {
        lg.open("GTAFmod.log", std::fstream::out | std::fstream::trunc);

        //Events::gameProcessEvent.Add(UpdateFmod);
        Events::initGameEvent.after.Add(InitializeFmod);
        Events::gameProcessEvent.before += [](){
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
                if (CTimer::m_snTimeInMilliseconds < (m_nLastGearChangeTime + 800))
                {
                    gasPedal = 0;
                }
                if (CTimer::m_snTimeInMilliseconds >= (m_nLastGearChangeTime + 800))
                {
                    if (gear != targetGear)
                    {
                        gearEventInstance->start();
                    }
                    gear = targetGear;
                    clutch = 0;
                    gasPedal = vehicle->m_fGasPedal;
                }
                if (KeyPressed(VK_F7))
                {
                    clutch = 1;
                }

                vehicle->m_vehicleAudio.m_nEngineState = -1;

                float torqueBias = 0;
                float speed = vehicle->m_vecMoveSpeed.Magnitude() * 175;
                float targetRpm = 850 + (vehicle->m_vecMoveSpeed.Magnitude() * 20000) / gear;

                float relation[] = {
                    60, 100, 140, 180, 200
                };

                torqueBias = ((relation[gear-1] - speed) / relation[gear-1]) * ((rpm / 4000) / gear) * 0.02;
                if (torqueBias <= 0)
                {
                    torqueBias = 0;
                }

                if (clutch > 0)
                {
                    rpm += (gasPedal * CTimer::ms_fTimeStep) * 200 * clutch;
                    if (rpm > 8000)
                    {
                        rpm = 8000;
                    }
                    if (gasPedal == 0)
                    {
                        rpm -= (CTimer::ms_fTimeStep) * 50;
                    }
                }
                else
                {
                    if (rpm < targetRpm)
                    {
                        rpm += (CTimer::ms_fTimeStep) * 50;
                        if (rpm > 9500)
                        {
                            CHud::SetHelpMessage("CABUM MOTOR", true, false, false);
                        }
                    }
                    else
                    {
                        rpm -= (CTimer::ms_fTimeStep) * 50;
                    }
                }

                vehicle->m_pHandlingData->m_transmissionData.m_fEngineAcceleration = (torqueBias * (1 - clutch)) * gasPedal;
                
                if (rpm < 800)
                {
                    rpm = 800;
                }
                if (KeyPressed(VK_SHIFT) && CTimer::m_snTimeInMilliseconds > (m_nLastGearChangeTime + 200))
                {
                    NextGear();
                }
                if (KeyPressed(VK_F6) && CTimer::m_snTimeInMilliseconds > (m_nLastSpawnedTime + 200))
                {
                    if (automatic == 0)
                    {
                        automatic = 1;
                        CHud::SetHelpMessage("Automatic: ON", true, false, false);
                    }
                    else
                    {
                        automatic = 0;
                        CHud::SetHelpMessage("Automatic: OFF", true, false, false);
                    }
                    m_nLastSpawnedTime = CTimer::m_snTimeInMilliseconds;
                }
                if (KeyPressed(VK_CONTROL) && CTimer::m_snTimeInMilliseconds > (m_nLastGearChangeTime + 200))
                {
                    PrevGear();
                }
                if (automatic == 1 && CTimer::m_snTimeInMilliseconds > (m_nLastGearChangeTime + 1500) && clutch == 0)
                {
                    if (gasPedal > 0 && rpm > 5500)
                    {
                        NextGear();
                    }
                    if (gasPedal > 0 && rpm < 1000)
                    {
                        PrevGear();
                    }
                    if (gasPedal == 0 && rpm > 3000)
                    {
                        PrevGear();
                    }
                }
                /*CVector2D points[] = {
                    CVector2D(0, 0),
                    CVector2D(1000, 180),
                    CVector2D(3500, 210),
                    CVector2D(6000, 120),
                };
                Curve cv = Curve(points);
                CMessages::AddMessageJumpQWithNumber(new char[] {"TORQUE ~1~"}, 3000, 0, cv.Evaluate(rpm) , targetGear, speed, clutch, gasPedal, 0, false);
                */
                CMessages::AddMessageJumpQWithNumber(new char[] {"RPM ~1~ GEAR ~1~"}, 3000, 0, rpm, targetGear, vehicle->m_pHandlingData->m_transmissionData.m_fEngineAcceleration * 1000, 0, 0, 0, false);

                rpmEventInstance->setParameterByID(rpmDesc.id, rpm);
                rpmEventInstance->setParameterByID(loadDesc.id, -1 + (gasPedal * 2));

                CheckError(fmodSystem->update(), "Update Failed");

                if (gasPedal != lastPedal)
                {
                    if (gasPedal == 0)
                    {
                        if (rpm > 5000 && CTimer::m_snTimeInMilliseconds > (m_nLastSpawnedTime + 1000))
                        {
                            backFireEventInstance->start();
                            m_nLastSpawnedTime = CTimer::m_snTimeInMilliseconds;
                        }
                    }
                }
                lastPedal = gasPedal;
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