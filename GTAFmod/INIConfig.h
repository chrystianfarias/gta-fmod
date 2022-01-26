#pragma once
#include <string>
#include "IniReader/IniReader.h"
class INIConfig
{
public:
	//FMOD
	bool m_bUseLogo;
	std::string m_sDefaultBank;

	//Audio
	float m_fMasterVolume;

	INIConfig(std::string iniPath)
	{
		CIniReader ini(iniPath);
		m_bUseLogo = ini.ReadBoolean("FMOD", "UseLogo", true);
		m_sDefaultBank = ini.ReadString("FMOD", "DefaultBank", "");

		m_fMasterVolume = ini.ReadFloat("Audio", "MasterVolume", 1.0);
	}
};

