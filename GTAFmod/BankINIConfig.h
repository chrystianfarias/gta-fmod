#pragma once
#include <string>
#include "IniReader/IniReader.h"
class BankINIConfig
{
public:
	//Bank
	int m_nModelId;
	int m_nSoundEngineId;
	std::string m_sEngineExtEvent;
	std::string m_sBackfireExtEvent;
	std::string m_sGearExtEvent;
	std::string m_sRPMParameter;
	std::string m_sThrottleParameter;

	//Audio
	float m_fGearTime;
	float m_fMinRPM;
	float m_fMaxRPM;
	float m_fRPMAcceleration;
	float m_fRPMDesaceleration;

	BankINIConfig(std::string iniPath)
	{
		CIniReader ini(iniPath);

		m_nModelId = ini.ReadFloat("Bank", "ModelId", -1);
		m_nSoundEngineId = ini.ReadFloat("Bank", "SoundEngineId", -1);

		m_sEngineExtEvent = ini.ReadString("Event", "EngineExtEvent", "");
		m_sBackfireExtEvent = ini.ReadString("Event", "BackfireExtEvent", "");
		m_sGearExtEvent = ini.ReadString("Event", "GearExtEvent", "");

		m_sRPMParameter = ini.ReadString("Parameter", "RPMParameter", "rpms");
		m_sThrottleParameter = ini.ReadString("Parameter", "ThrottleParameter", "throttle");

		m_fMinRPM = ini.ReadFloat("Audio", "MinRPM", 800);
		m_fMaxRPM = ini.ReadFloat("Audio", "MaxRPM", 8500);
		m_fGearTime = ini.ReadFloat("Audio", "GearTime", 1000);
		m_fRPMAcceleration = ini.ReadFloat("Audio", "RPMAcceleration", 50);
		m_fRPMDesaceleration = ini.ReadFloat("Audio", "RPMDesaceleration", 60);
	}
};

