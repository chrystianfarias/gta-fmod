#include "plugin.h"
#include "FMODAudio.h"
#include <string>
#include <game_sa/CHud.h>

void FMODAudio::LoadBank(FMOD::Studio::System* fmodSystem, std::string bank)
{
    const std::string path = "modloader\\GTAFmod\\banks\\" + bank + ".bank";
    const std::string event = "event:/cars/" + bank;

    FMOD::Studio::Bank* vehiclesBank = NULL;
    CheckError(fmodSystem->loadBankFile(GAME_PATH((char*)path.c_str()), FMOD_STUDIO_LOAD_BANK_NORMAL, &vehiclesBank), "Failed on load bank SFX");

    //Load events
    FMOD::Studio::EventDescription* rpmEventDescription = NULL;
    CheckError(fmodSystem->getEvent((event + "/engine_ext").c_str(), &rpmEventDescription), "Failed on get event");

    FMOD::Studio::EventDescription* backFireEventDescription = NULL;
    CheckError(fmodSystem->getEvent((event + "/backfire_int").c_str(), &backFireEventDescription), "Failed on get event");

    FMOD::Studio::EventDescription* gearEventDescription = NULL;
    CheckError(fmodSystem->getEvent((event + "/gear_ext").c_str(), &gearEventDescription), "Failed on get event");

    //RPM Instance
    CheckError(rpmEventDescription->createInstance(&m_RpmEventInstance), "Failed on create instance");

    //Backfire Instance
    CheckError(backFireEventDescription->createInstance(&m_BackFireEventInstance), "Failed on create instance");

    //Gear Change Instance
    CheckError(gearEventDescription->createInstance(&m_GearEventInstance), "Failed on create instance");

    //Get parameters
    CheckError(rpmEventDescription->getParameterDescriptionByName("rpms", &m_RpmDesc), "Failed on get parameter");
    CheckError(rpmEventDescription->getParameterDescriptionByName("throttle", &m_LoadDesc), "Failed on get parameter");

    //Set 3D space
    m_RpmEventInstance->set3DAttributes(&m_Attributes);
    m_RpmEventInstance->setParameterByID(m_RpmDesc.id, 0.0f);
    m_RpmEventInstance->setParameterByID(m_LoadDesc.id, 1.0f);

    CHud::SetHelpMessage((event + "/engine_ext").c_str(), true, false, false);

    m_Attributes = { { 0 } };
    m_Attributes.forward.z = 1.0f;
    m_Attributes.up.y = 1.0f;
    CheckError(fmodSystem->setListenerAttributes(0, &m_Attributes), "Failed on set 3d ambient");

    m_bIsLoaded = true;
}