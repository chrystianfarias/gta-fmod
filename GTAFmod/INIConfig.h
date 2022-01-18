#pragma once
#include <string>
#include "IniReader/IniReader.h"
class INIConfig
{
public:
	//Audio
	float m_fMinRPM;
	float m_fMidRPM;
	float m_fFinalRPM;
	float m_fMinRPMTorque;
	float m_fMidRPMTorque;
	float m_fFinalRPMTorque;
	float m_fRPMVolume;
	float m_fRPMAcceleration;
	float m_fRPMDesaceleration;
	bool m_bAutomaticGearbox;
	float m_fPitchMultiply;
	//Damage
	bool m_bDamageWhenShiftingWrong;
	float m_fMaxRPMDamage;
	float m_fDamageMultiply;
	//GearRatio
	float m_fRalationR;
	float m_fRalationN;
	float m_fRalation1;
	float m_fRalation2;
	float m_fRalation3;
	float m_fRalation4;
	float m_fRalation5;
	float m_fRalation6;
	//Keys
	int m_nUpGearKey;
	int m_nDownGearKey;
	int m_nClutchGearKey;

	INIConfig(std::string iniPath)
	{
		CIniReader ini(iniPath);
		m_fMinRPM = ini.ReadFloat("Audio", "MinRPM", 800);
		m_fMidRPM = ini.ReadFloat("Audio", "MidRPM", 4500);
		m_fFinalRPM = ini.ReadFloat("Audio", "FinalRPM", 8000);
		m_fMinRPMTorque = ini.ReadFloat("Audio", "MinRPMTorque", 0.8);
		m_fMidRPMTorque = ini.ReadFloat("Audio", "MidRPMTorque", 1.2);
		m_fFinalRPMTorque = ini.ReadFloat("Audio", "FinalRPMTorque", 0.1);
		m_fRPMVolume = ini.ReadFloat("Audio", "RPMVolume", 0.4);
		m_fRPMAcceleration = ini.ReadFloat("Audio", "RPMAcceleration", 50);
		m_fRPMDesaceleration = ini.ReadFloat("Audio", "RPMDesaceleration", 60);
		m_bAutomaticGearbox = ini.ReadFloat("Audio", "AutomaticGearbox", 0);
		m_fPitchMultiply = ini.ReadFloat("Audio", "PitchMultiply", 60);

		m_bDamageWhenShiftingWrong = ini.ReadBoolean("Damage", "DamageWhenShiftingWrong", 1);
		m_fMaxRPMDamage = ini.ReadFloat("Damage", "MaxRPMDamage", 800);
		m_fDamageMultiply = ini.ReadFloat("Damage", "DamageMultiply", 80);

		m_fRalationR = ini.ReadFloat("GearRatio", "RelationR", -1);
		m_fRalationN = ini.ReadFloat("GearRatio", "RelationN", 0);
		m_fRalation1 = ini.ReadFloat("GearRatio", "Relation1", 3);
		m_fRalation2 = ini.ReadFloat("GearRatio", "Relation2", 2);
		m_fRalation3 = ini.ReadFloat("GearRatio", "Relation3", 1.6);
		m_fRalation4 = ini.ReadFloat("GearRatio", "Relation4", 1.2);
		m_fRalation5 = ini.ReadFloat("GearRatio", "Relation5", 0.9);
		m_fRalation6 = ini.ReadFloat("GearRatio", "Relation6", 0.7);

		m_nUpGearKey = ini.ReadInteger("Keys", "UpGearKey", 16);
		m_nDownGearKey = ini.ReadInteger("Keys", "DownGearKey", 17);
		m_nClutchGearKey = ini.ReadInteger("Keys", "ClutchKey", 117);
	}
};

