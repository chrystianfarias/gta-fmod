#pragma once
#include "fmod.hpp"
#include "fmod_studio.hpp"
#include "CTimer.h"
#include "fmod_errors.h"
#include "INIConfig.h"

class FMODAudio
{
public:

    bool m_bIsLoaded = false;
    bool m_bIsPlaying = false;
    FMOD::Studio::EventInstance* m_RpmEventInstance;
	FMOD::Studio::EventInstance* m_BackFireEventInstance;
	FMOD::Studio::EventInstance* m_GearEventInstance;
	FMOD_STUDIO_PARAMETER_DESCRIPTION m_RpmDesc;
	FMOD_STUDIO_PARAMETER_DESCRIPTION m_LoadDesc;
    FMOD_3D_ATTRIBUTES m_Attributes;
    FMOD_3D_ATTRIBUTES m_ListenerAttributes;

    void LoadBank(FMOD::Studio::System* fmodSystem, INIConfig* ini, char* bank, char* absolutePath);

    static void CheckError(FMOD_RESULT result, const char* text)
    {
        if (result != FMOD_OK)
        {
            std::ofstream lg;
            lg.open("GTAFmod.log", std::ofstream::app);

            lg << "Error\n";
            lg << FMOD_ErrorString(result);
            lg << "\n";
            lg << text;
            lg << "\n---\n";
            lg.flush();

            lg.close();
        }
    }
};