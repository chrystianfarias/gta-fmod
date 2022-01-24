#include "plugin.h"
#include "FMODAudio.h"
#include <string>
#include <game_sa/CHud.h>

void FMODAudio::LoadBank(FMOD::Studio::System* fmodSystem, INIConfig* ini, char* bank, char* absolutePath)
{
    m_bIsLoaded = false;

    const std::string eventName = bank;
    const std::string event = "event:/cars/" + eventName;

    FMOD::Studio::Bank* vehiclesBank = NULL;
    CheckError(fmodSystem->loadBankFile(absolutePath, FMOD_STUDIO_LOAD_BANK_NORMAL, &vehiclesBank), absolutePath);

    //Load events
    FMOD::Studio::EventDescription* rpmEventDescription = NULL;
    CheckError(fmodSystem->getEvent((event + "/engine_ext").c_str(), &rpmEventDescription), "Failed on get event");

    FMOD::Studio::EventDescription* rpmLimiterEventDescription = NULL;
    CheckError(fmodSystem->getEvent((event + "/limiter").c_str(), &rpmLimiterEventDescription), "Failed on get event");

    FMOD::Studio::EventDescription* backFireEventDescription = NULL;
    CheckError(fmodSystem->getEvent((event + "/backfire_ext").c_str(), &backFireEventDescription), "Failed on get event");

    FMOD::Studio::EventDescription* gearEventDescription = NULL;
    CheckError(fmodSystem->getEvent((event + "/gear_ext").c_str(), &gearEventDescription), "Failed on get event");

    //RPM Instance
    CheckError(rpmEventDescription->createInstance(&m_RpmEventInstance), "Failed on create instance");

    //RPM Limiter Instance
    CheckError(rpmLimiterEventDescription->createInstance(&m_RpmLimiterEventInstance), "Failed on create instance");

    //Backfire Instance
    CheckError(backFireEventDescription->createInstance(&m_BackFireEventInstance), "Failed on create instance");

    //Gear Change Instance
    CheckError(gearEventDescription->createInstance(&m_GearEventInstance), "Failed on create instance");

    //Get parameters
    CheckError(rpmEventDescription->getParameterDescriptionByName("rpms", &m_RpmDesc), "Failed on get parameter");
    CheckError(rpmEventDescription->getParameterDescriptionByName("throttle", &m_LoadDesc), "Failed on get parameter");

    //Set 3D space
    m_RpmEventInstance->setVolume(ini->m_fVolume);
    m_RpmEventInstance->setReverbLevel(0, 2);
    m_RpmEventInstance->set3DAttributes(&m_Attributes);
    m_RpmLimiterEventInstance->setVolume(0);
    m_RpmLimiterEventInstance->setReverbLevel(0, 2);
    m_RpmLimiterEventInstance->set3DAttributes(&m_Attributes);

    m_RpmEventInstance->setParameterByID(m_RpmDesc.id, 0.0f);
    m_RpmEventInstance->setParameterByID(m_LoadDesc.id, 1.0f);

    m_ListenerAttributes = { { 0 } };
    m_ListenerAttributes.forward.x = 1.0f;
    m_ListenerAttributes.up.z = 1.0f;

    CheckError(fmodSystem->setListenerAttributes(0, &m_ListenerAttributes), "Failed on set 3d ambient");

    m_bIsLoaded = true;
}