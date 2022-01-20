#pragma once
#include <string>
#include "IniReader/IniReader.h"
class INIConfig
{
public:
	//Audio
	float m_fFinalRPM;
	float m_fRPMAcceleration;
	float m_fRPMDesaceleration;
	//GearRatio
	float m_fRalationR;
	float m_fRalation1;
	float m_fRalation2;
	float m_fRalation3;
	float m_fRalation4;
	float m_fRalation5;
	float m_fRalation6;

	INIConfig(std::string iniPath)
	{
		CIniReader ini(iniPath);
		m_fFinalRPM = ini.ReadFloat("Audio", "FinalRPM", 8000);
		m_fRPMAcceleration = ini.ReadFloat("Audio", "RPMAcceleration", 50);
		m_fRPMDesaceleration = ini.ReadFloat("Audio", "RPMDesaceleration", 60);

		m_fRalationR = ini.ReadFloat("GearRatio", "RelationR", -1);
		m_fRalation1 = ini.ReadFloat("GearRatio", "Relation1", 3);
		m_fRalation2 = ini.ReadFloat("GearRatio", "Relation2", 2);
		m_fRalation3 = ini.ReadFloat("GearRatio", "Relation3", 1.6);
		m_fRalation4 = ini.ReadFloat("GearRatio", "Relation4", 1.2);
		m_fRalation5 = ini.ReadFloat("GearRatio", "Relation5", 0.9);
		m_fRalation6 = ini.ReadFloat("GearRatio", "Relation6", 0.7);
	}
};

