#include "plugin.h"
#include "CTimer.h"
#include "CHud.h"
#include "CMessages.h"
#include "CCamera.h"
#include "fmod.hpp"
#include "fmod_studio.hpp"

using namespace plugin;

class GTAFmod {
public:
    static unsigned int m_nLastSpawnedTime;
    static FMOD::Studio::EventInstance* eventInstance;
    static FMOD::Studio::System* system;
    static FMOD_STUDIO_PARAMETER_DESCRIPTION rpmDesc;
    static FMOD_STUDIO_PARAMETER_DESCRIPTION loadDesc;
    static FMOD_3D_ATTRIBUTES attributes;

    static void UpdateFmod() {
        if (system != NULL)
        {
        }
    }
    /*static void ProcessKey() {
        if (KeyPressed(VK_TAB) && CTimer::m_snTimeInMilliseconds > (m_nLastSpawnedTime + 1000)) { // Если нажата клавиша и прошло больше секунды с момента последнего спавна

            InitializeFmod();
        }
        if (system != NULL)
        {
            CheckError(system->update(), "Update Failed");
        }
    }*/
    static void CheckError(FMOD_RESULT result, const char* text)
    {
        if (result != FMOD_OK && CTimer::m_snTimeInMilliseconds > (m_nLastSpawnedTime + 1000))
        {
            CHud::SetHelpMessage(text, true, false, false);
            m_nLastSpawnedTime = CTimer::m_snTimeInMilliseconds;
        }
    }
    static void InitializeFmod()
    {
        void* extraDriverData = NULL;
        CheckError(FMOD::Studio::System::create(&system), "Failed on create FMOD System");

        // The example Studio project is authored for 5.1 sound, so set up the system output mode to match
        FMOD::System* coreSystem = NULL;
        CheckError(system->getCoreSystem(&coreSystem), "Failed on create FMOD CORE System");
        CheckError(coreSystem->setSoftwareFormat(0, FMOD_SPEAKERMODE_5POINT1, 0), "Failed on set software format");

        CheckError(system->initialize(1024, FMOD_STUDIO_INIT_NORMAL, FMOD_INIT_NORMAL, extraDriverData), "Failed to initialize");

        FMOD::Studio::Bank* masterBank = NULL;
        CheckError(system->loadBankFile(GAME_PATH((char*)"banks\\Master.bank"), FMOD_STUDIO_LOAD_BANK_NORMAL, &masterBank), "Failed on load bank Master");

        FMOD::Studio::Bank* stringsBank = NULL;
        CheckError(system->loadBankFile(GAME_PATH((char*)"banks\\Master.strings.bank"), FMOD_STUDIO_LOAD_BANK_NORMAL, &stringsBank), "Failed on load bank Master String");

        FMOD::Studio::Bank* vehiclesBank = NULL;
        CheckError(system->loadBankFile(GAME_PATH((char*)"banks\\Vehicles.bank"), FMOD_STUDIO_LOAD_BANK_NORMAL, &vehiclesBank), "Failed on load bank SFX");

        attributes = { { 0 } };
        attributes.forward.z = 1.0f;
        attributes.up.y = 1.0f;
        CheckError(system->setListenerAttributes(0, &attributes), "Failed on set 3d ambient");

        FMOD::Studio::EventDescription* eventDescription = NULL;
        CheckError(system->getEvent("event:/Vehicles/Car Engine", &eventDescription), "Failed on get event");
        //CheckError(system->getEvent("event:/Vehicles/Ride-on Mower", &eventDescription), "Failed on get event");

        CheckError(eventDescription->getParameterDescriptionByIndex(0, &rpmDesc), "Failed on get parameter");
        CheckError(eventDescription->getParameterDescriptionByIndex(1, &loadDesc), "Failed on get parameter");

        eventInstance = NULL;
        CheckError(eventDescription->createInstance(&eventInstance), "Failed on create instance");

        eventInstance->set3DAttributes(&attributes);
        eventInstance->setVolume(.5f);
        eventInstance->setParameterByID(rpmDesc.id, 0.0f);
        eventInstance->setParameterByID(loadDesc.id, 1.0f);
        eventInstance->start();

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

    static unsigned int m_nLastGearChangeTime;
    static unsigned int automatic;
    static float clutch;
    static float rpm;
    static int gear;
    static int targetGear;

    static bool bPlaying;

    GTAFmod() {

        Events::gameProcessEvent.Add(UpdateFmod);
        Events::initGameEvent.after.Add(InitializeFmod);
        Events::gameProcessEvent.before += [](){
            CVehicle* vehicle = FindPlayerVehicle(-1, true);
            if (vehicle && vehicle->m_nVehicleSubClass != VEHICLE_PLANE && vehicle->m_nVehicleSubClass != VEHICLE_HELI
                && vehicle->m_nVehicleSubClass != VEHICLE_BOAT && vehicle->m_nVehicleSubClass != VEHICLE_BMX &&
                vehicle->m_nVehicleSubClass != VEHICLE_TRAIN && CTimer::m_UserPause == false)
            {
                if (bPlaying == false)
                {
                    eventInstance->start();
                    bPlaying = true;
                }
                CVector camPos = TheCamera.GetPosition();
                CVector vehiclePos = vehicle->GetPosition();

                attributes.position.x = vehiclePos.x - camPos.x;
                attributes.position.y = vehiclePos.y - camPos.y;
                attributes.position.z = vehiclePos.z - camPos.z;
                eventInstance->set3DAttributes(&attributes);
                CMessages::AddMessageJumpQWithNumber(new char[] {"x ~1~ y ~1~ z ~1~"}, 3000, 0, attributes.position.x, attributes.position.y, attributes.position.z, 0, 0, 0, false);

                float gasPedal = vehicle->m_fGasPedal;
                if (CTimer::m_snTimeInMilliseconds < (m_nLastGearChangeTime + 800))
                {
                    gasPedal = 0;
                }
                if (CTimer::m_snTimeInMilliseconds >= (m_nLastGearChangeTime + 800))
                {
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

                torqueBias = (relation[gear-1] - speed) / relation[gear-1];
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

                vehicle->m_pHandlingData->m_transmissionData.m_fEngineAcceleration = (rpm / 6500) * (torqueBias * (1-clutch)) * gasPedal;

                
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
                if (automatic == 1 && CTimer::m_snTimeInMilliseconds > (m_nLastGearChangeTime + 1500))
                {
                    if (gasPedal > 0 && rpm > 6000)
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
                //CMessages::AddMessageJumpQWithNumber(new char[] {"RPM ~1~ GEAR ~1~ SPEED ~1~ CLUTCH ~1~ GAS ~1~"}, 3000, 0, rpm, targetGear, speed, clutch, gasPedal, 0, false);

                eventInstance->setParameterByID(rpmDesc.id, rpm);
                eventInstance->setParameterByID(loadDesc.id, -1 + (gasPedal * 2));

                CheckError(system->update(), "Update Failed");
            }
            else
            {
                if (bPlaying == true)
                {
                    eventInstance->stop(FMOD_STUDIO_STOP_IMMEDIATE);
                    CheckError(system->update(), "Update Failed");
                    bPlaying = false;
                }
            }
        };
    }
} gTAFmod;
FMOD::Studio::EventInstance* GTAFmod::eventInstance = NULL;
FMOD::Studio::System* GTAFmod::system = NULL;
FMOD_STUDIO_PARAMETER_DESCRIPTION GTAFmod::rpmDesc;
FMOD_STUDIO_PARAMETER_DESCRIPTION GTAFmod::loadDesc; 
FMOD_3D_ATTRIBUTES GTAFmod::attributes;

int GTAFmod::gear = 1;
int GTAFmod::targetGear = 1;
float GTAFmod::rpm = 850;
float GTAFmod::clutch = 1;
bool GTAFmod::bPlaying;
unsigned int GTAFmod::m_nLastGearChangeTime = 0;
unsigned int GTAFmod::m_nLastSpawnedTime = 0;
unsigned int GTAFmod::automatic = 1;