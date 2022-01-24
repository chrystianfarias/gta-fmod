#pragma once
#include <string>
#include "IniReader/IniReader.h"
class INIConfig
{
public:
	//Audio
	float m_fVolume;
	float m_fMinRPM;
	float m_fMaxRPM;
	float m_fRPMAcceleration;
	float m_fRPMDesaceleration;
	bool m_bUseLogo;

	INIConfig(std::string iniPath)
	{
		CIniReader ini(iniPath);
		m_fVolume = ini.ReadFloat("Audio", "Volume", 1.0);
		m_fMinRPM = ini.ReadFloat("Audio", "MinRPM", 800);
		m_fMaxRPM = ini.ReadFloat("Audio", "MaxRPM", 6200);
		m_bUseLogo = ini.ReadBoolean("FMOD", "UseLogo", true);
		m_fRPMAcceleration = ini.ReadFloat("Audio", "RPMAcceleration", 50);
		m_fRPMDesaceleration = ini.ReadFloat("Audio", "RPMDesaceleration", 60);
	}
};

