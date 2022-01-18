#include "plugin.h"
#include "CMessages.h"
#include "CTimer.h"
#include "CHud.h"

using namespace plugin;

typedef float (__cdecl* GTAFmod_Ext_GetCurrentRPM)();
GTAFmod_Ext_GetCurrentRPM gtaFmod_Ext_GetCurrentRPM;

typedef int(__cdecl* GTAFmod_Ext_GetCurrentGear)();
GTAFmod_Ext_GetCurrentGear gtaFmod_Ext_GetCurrentGear;

typedef void(__cdecl* GTAFmod_Ext_SetVehicleFMODBank)(CVehicle* vehicle, char* bank, char* absolutePath);
GTAFmod_Ext_SetVehicleFMODBank gtaFmod_Ext_SetVehicleFMODBank;

typedef void(__cdecl* GTAFmod_Ext_SetModelIdFMODBank)(int id, char* bank, char* absolutePath);
GTAFmod_Ext_SetModelIdFMODBank gtaFmod_Ext_SetModelIdFMODBank;

typedef void(__cdecl* GTAFmod_Ext_SetEngineSoundIdFMODBank)(int id, char* bank, char* absolutePath);
GTAFmod_Ext_SetEngineSoundIdFMODBank gtaFmod_Ext_SetEngineSoundIdFMODBank;


int keyTime;

class GTAFmodDashboard {
public:
    GTAFmodDashboard() {
        HINSTANCE moduleGTAFmod = GetModuleHandleA("GTAFmod.SA.asi");
        if (moduleGTAFmod) {
            gtaFmod_Ext_GetCurrentRPM = (GTAFmod_Ext_GetCurrentRPM)GetProcAddress(moduleGTAFmod, "Ext_GetCurrentRPM");
            gtaFmod_Ext_GetCurrentGear = (GTAFmod_Ext_GetCurrentGear)GetProcAddress(moduleGTAFmod, "Ext_GetCurrentGear");
            gtaFmod_Ext_SetVehicleFMODBank = (GTAFmod_Ext_SetVehicleFMODBank)GetProcAddress(moduleGTAFmod, "Ext_SetVehicleFMODBank");
            gtaFmod_Ext_SetModelIdFMODBank = (GTAFmod_Ext_SetModelIdFMODBank)GetProcAddress(moduleGTAFmod, "Ext_SetModelIdFMODBank");
            gtaFmod_Ext_SetEngineSoundIdFMODBank = (GTAFmod_Ext_SetEngineSoundIdFMODBank)GetProcAddress(moduleGTAFmod, "Ext_SetEngineSoundIdFMODBank");
        }
        else {
            moduleGTAFmod = 0;
            gtaFmod_Ext_GetCurrentRPM = 0;
            gtaFmod_Ext_GetCurrentGear = 0;
            gtaFmod_Ext_SetVehicleFMODBank = 0;
            gtaFmod_Ext_SetModelIdFMODBank = 0;
            gtaFmod_Ext_SetEngineSoundIdFMODBank = 0;
        }

        Events::gameProcessEvent.before += []() {
            CVehicle* vehicle = FindPlayerVehicle(-1, true);

            if (vehicle)
            {
                if (gtaFmod_Ext_GetCurrentRPM) {
                    float rpm = gtaFmod_Ext_GetCurrentRPM();
                    int gear = gtaFmod_Ext_GetCurrentGear();
                    CMessages::AddMessageJumpQWithNumber(new char[] {"Gear ~1~ RPM ~1~"}, 3000, 0, gear, rpm, 0, 0, 0, 0, false);
                }

                if (KeyPressed(VK_F7) && CTimer::m_snTimeInMilliseconds >= (keyTime + 2000))
                {
                    gtaFmod_Ext_SetModelIdFMODBank(vehicle->m_nModelIndex, (char*)"ks_mazda_rx7_spirit_r", GAME_PATH((char*)"modloader\\GTAFmod\\banks\\ks_mazda_rx7_spirit_r.bank"));
                    keyTime = CTimer::m_snTimeInMilliseconds;
                    CHud::SetHelpMessage("Current Vehicle Model ID update", true, false, false);
                }
                if (KeyPressed(VK_F8) && CTimer::m_snTimeInMilliseconds >= (keyTime + 2000))
                {
                    gtaFmod_Ext_SetVehicleFMODBank(vehicle, (char*)"ks_mazda_rx7_spirit_r", GAME_PATH((char*)"modloader\\GTAFmod\\banks\\ks_mazda_rx7_spirit_r.bank"));
                    keyTime = CTimer::m_snTimeInMilliseconds;
                    CHud::SetHelpMessage("Current Vehicle Instance update", true, false, false);
                }
            }
        };
    }
} gTAFmodDashboard;
