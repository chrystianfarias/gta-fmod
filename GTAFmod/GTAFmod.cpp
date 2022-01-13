#include "plugin.h"
#include "CTimer.h"
#include "CHud.h"
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

    static void ProcessKey() {
        if (KeyPressed(VK_TAB) && CTimer::m_snTimeInMilliseconds > (m_nLastSpawnedTime + 1000)) { // Если нажата клавиша и прошло больше секунды с момента последнего спавна

            InitializeFmod();
        }
        if (system != NULL)
        {
            CheckError(system->update(), "Update Failed");
        }
    }
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

        CheckError(system->initialize(1024, FMOD_STUDIO_INIT_NORMAL, FMOD_INIT_NORMAL, extraDriverData), "Failed to initialize");

        FMOD::Studio::Bank* masterBank = NULL;
        CheckError(system->loadBankFile(GAME_PATH((char*)"banks\\Master.bank"), FMOD_STUDIO_LOAD_BANK_NORMAL, &masterBank), "Failed on load bank Master");

        FMOD::Studio::Bank* stringsBank = NULL;
        CheckError(system->loadBankFile(GAME_PATH((char*)"banks\\Master.strings.bank"), FMOD_STUDIO_LOAD_BANK_NORMAL, &stringsBank), "Failed on load bank Master String");

        FMOD::Studio::Bank* vehiclesBank = NULL;
        CheckError(system->loadBankFile(GAME_PATH((char*)"banks\\Vehicles.bank"), FMOD_STUDIO_LOAD_BANK_NORMAL, &vehiclesBank), "Failed on load bank SFX");

        FMOD::Studio::EventDescription* eventDescription = NULL;
        CheckError(system->getEvent("event:/Vehicles/Car Engine", &eventDescription), "Failed on get event");
        //CheckError(system->getEvent("event:/Vehicles/Ride-on Mower", &eventDescription), "Failed on get event");

        CheckError(eventDescription->getParameterDescriptionByIndex(0, &rpmDesc), "Failed on get parameter");
        CheckError(eventDescription->getParameterDescriptionByIndex(1, &loadDesc), "Failed on get parameter");

        eventInstance = NULL;
        CheckError(eventDescription->createInstance(&eventInstance), "Failed on create instance");

        eventInstance->setVolume(.5f);
        eventInstance->setParameterByID(rpmDesc.id, 0.0f);
        eventInstance->setParameterByID(loadDesc.id, 1.0f);
        eventInstance->start();

        CHud::SetHelpMessage(rpmDesc.name, true, false, false);
    }
    GTAFmod() {
        //Events::initRwEvent.Add(InitializeFmod);
        Events::gameProcessEvent.Add(ProcessKey);
        Events::drawHudEvent += [] {
            CVehicle* vehicle = FindPlayerVehicle(-1, true);

            if (vehicle && vehicle->m_nVehicleSubClass != VEHICLE_PLANE && vehicle->m_nVehicleSubClass != VEHICLE_HELI
                && vehicle->m_nVehicleSubClass != VEHICLE_BOAT && vehicle->m_nVehicleSubClass != VEHICLE_BMX &&
                vehicle->m_nVehicleSubClass != VEHICLE_TRAIN)
            {
                vehicle->m_vehicleAudio.m_nEngineState = -1;
                //vehicle->m_vehicleAudio.m_nEngineAccelerateSoundBankId = 0;
                //vehicle->m_vehicleAudio.m_nEngineDecelerateSoundBankId = 0;
                float speed = 850 + (vehicle->m_vecMoveSpeed.Magnitude() * 10000 / vehicle->m_nCurrentGear);
                if (speed > rpmDesc.maximum) {
                    speed = rpmDesc.maximum;
                }
                if (speed < rpmDesc.minimum) {
                    speed = rpmDesc.minimum;
                }
                eventInstance->setParameterByID(rpmDesc.id, speed); 
                eventInstance->setParameterByID(loadDesc.id, -1 + (vehicle->m_fGasPedal * 2));
            }
        };
    }
} gTAFmod;
unsigned int GTAFmod::m_nLastSpawnedTime = 0;
FMOD::Studio::EventInstance* GTAFmod::eventInstance = NULL;
FMOD::Studio::System* GTAFmod::system = NULL;
FMOD_STUDIO_PARAMETER_DESCRIPTION GTAFmod::rpmDesc;
FMOD_STUDIO_PARAMETER_DESCRIPTION GTAFmod::loadDesc;