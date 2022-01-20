#pragma once
#include <string>
#include "IniReader/IniReader.h"
class INIConfig
{
public:
	//Audio
	float m_fMinRPM;
	float m_fMaxRPM;
	float m_fRPMAcceleration;
	float m_fRPMDesaceleration;

	INIConfig(std::string iniPath)
	{
		CIniReader ini(iniPath);
		m_fMinRPM = ini.ReadFloat("Audio", "FinalRPM", 800);
		m_fMaxRPM = ini.ReadFloat("Audio", "FinalRPM", 6200);
		m_fRPMAcceleration = ini.ReadFloat("Audio", "RPMAcceleration", 50);
		m_fRPMDesaceleration = ini.ReadFloat("Audio", "RPMDesaceleration", 60);
	}
};

