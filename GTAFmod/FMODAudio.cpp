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
    CheckError(fmodSystem->getEvent(m_Ini->m_sEngineExtEvent.c_str(), &rpmEventDescription), "Failed on get event");

    FMOD::Studio::EventDescription* backFireEventDescription = NULL;
    CheckError(fmodSystem->getEvent(m_Ini->m_sBackfireExtEvent.c_str(), &backFireEventDescription), "Failed on get event");

    FMOD::Studio::EventDescription* gearEventDescription = NULL;
    CheckError(fmodSystem->getEvent(m_Ini->m_sGearExtEvent.c_str(), &gearEventDescription), "Failed on get event");

    //RPM Instance
    CheckError(rpmEventDescription->createInstance(&m_RpmEventInstance), "Failed on create instance");

    //Backfire Instance
    CheckError(backFireEventDescription->createInstance(&m_BackFireEventInstance), "Failed on create instance");

    //Gear Change Instance
    CheckError(gearEventDescription->createInstance(&m_GearEventInstance), "Failed on create instance");

    //Get parameters
    CheckError(rpmEventDescription->getParameterDescriptionByName(m_Ini->m_sRPMParameter.c_str(), &m_RpmDesc), "Failed on get parameter");
    CheckError(rpmEventDescription->getParameterDescriptionByName(m_Ini->m_sThrottleParameter.c_str(), &m_LoadDesc), "Failed on get parameter");

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