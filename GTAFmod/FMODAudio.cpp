#include "plugin.h"
#include "FMODAudio.h"
#include "CModelInfo.h"
#include <string>
#include <filesystem>
#include <game_sa/CHud.h>
#include "Utils.h"

using std::string;
namespace fs = std::filesystem;

void FMODAudio::LoadBank(FMOD::Studio::System* fmodSystem, char* absolutePath)
{
    m_bIsLoaded = false;

    fs::path bankPath = fs::path(absolutePath);
    string dir = bankPath.parent_path().string();
    string modelName = bankPath.stem().string();
    string iniPath = dir + "\\" + modelName + ".ini";

    m_Ini = new BankINIConfig(iniPath);

    if (m_Ini->m_sEngineExtEvent == "")
    {
        string bnk = "INI or EngineExtEvent not found: " + iniPath + "\nFrom: " + absolutePath;
        CheckError(FMOD_ERR_FILE_NOTFOUND, bnk.data());
        return;
    }

    int modelIndex;
    CBaseModelInfo* vehinfo = CModelInfo::GetModelInfo(modelName.data(), &modelIndex);
    if (vehinfo)
    {
        m_Ini->m_nModelId = modelIndex;
    }

    FMOD::Studio::Bank* vehiclesBank = NULL;
    CheckError(fmodSystem->loadBankFile(absolutePath, FMOD_STUDIO_LOAD_BANK_NORMAL, &vehiclesBank), absolutePath);

    //Load events
    FMOD::Studio::EventDescription* rpmEventDescription = NULL;
    FMOD::Studio::ID guidRpmEvent = { 0 };
    CheckError(FMOD::Studio::parseID(m_Ini->m_sEngineExtEvent.c_str(), &guidRpmEvent), m_Ini->m_sEngineExtEvent.c_str());
    CheckError(fmodSystem->getEventByID(&guidRpmEvent, &rpmEventDescription), "Failed on get rpm event");

    FMOD::Studio::EventDescription* backFireEventDescription = NULL;
    FMOD::Studio::ID guidBackFireEvent = { 0 };
    CheckError(FMOD::Studio::parseID(m_Ini->m_sBackfireExtEvent.c_str(), &guidBackFireEvent), m_Ini->m_sBackfireExtEvent.c_str());
    CheckError(fmodSystem->getEventByID(&guidBackFireEvent, &backFireEventDescription), "Failed on get backfire event");

    FMOD::Studio::EventDescription* gearEventDescription = NULL;
    FMOD::Studio::ID guidGearEvent = { 0 };
    CheckError(FMOD::Studio::parseID(m_Ini->m_sBackfireExtEvent.c_str(), &guidGearEvent), m_Ini->m_sBackfireExtEvent.c_str());
    CheckError(fmodSystem->getEventByID(&guidGearEvent, &gearEventDescription), "Failed on get gear event");

    //RPM Instance
    CheckError(rpmEventDescription->createInstance(&m_RpmEventInstance), "Failed on create rpm instance");

    //Backfire Instance
    CheckError(backFireEventDescription->createInstance(&m_BackFireEventInstance), "Failed on create backfire instance");

    //Gear Change Instance
    CheckError(gearEventDescription->createInstance(&m_GearEventInstance), "Failed on create gear instance");

    //Get parameters
    CheckError(rpmEventDescription->getParameterDescriptionByName(m_Ini->m_sRPMParameter.c_str(), &m_RpmDesc), "Failed on get rpms parameter");
    CheckError(rpmEventDescription->getParameterDescriptionByName(m_Ini->m_sThrottleParameter.c_str(), &m_LoadDesc), "Failed on get toggle parameter");

    //Set 3D space
    m_RpmEventInstance->setReverbLevel(0, 2);
    m_RpmEventInstance->set3DAttributes(&m_Attributes);

    m_RpmEventInstance->setParameterByID(m_RpmDesc.id, 0.0f);
    m_RpmEventInstance->setParameterByID(m_LoadDesc.id, 1.0f);

    m_ListenerAttributes = { { 0 } };
    m_ListenerAttributes.forward.x = 1.0f;
    m_ListenerAttributes.up.z = 1.0f;

    CheckError(fmodSystem->setListenerAttributes(0, &m_ListenerAttributes), "Failed on set 3d ambient");

    m_bIsLoaded = true;
}