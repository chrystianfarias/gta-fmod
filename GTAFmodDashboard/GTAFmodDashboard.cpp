#include "plugin.h"
#include "CMessages.h"
#include "CTimer.h"

using namespace plugin;

typedef float (__cdecl* GTAFmod_Ext_GetCurrentRPM)();
GTAFmod_Ext_GetCurrentRPM gtaFmod_Ext_GetCurrentRPM;

typedef int(__cdecl* GTAFmod_Ext_GetCurrentGear)();
GTAFmod_Ext_GetCurrentGear gtaFmod_Ext_GetCurrentGear;

typedef void(__cdecl* GTAFmod_Ext_SetVehicleFMODBank)(CVehicle* vehicle, std::string bank);
GTAFmod_Ext_SetVehicleFMODBank gtaFmod_Ext_SetVehicleFMODBank;


int keyTime;

class GTAFmodDashboard {
public:
    GTAFmodDashboard() {
        HINSTANCE moduleGTAFmod = GetModuleHandleA("GTAFmod.SA.asi");
        if (moduleGTAFmod) {
            gtaFmod_Ext_GetCurrentRPM = (GTAFmod_Ext_GetCurrentRPM)GetProcAddress(moduleGTAFmod, "Ext_GetCurrentRPM");
            gtaFmod_Ext_GetCurrentGear = (GTAFmod_Ext_GetCurrentGear)GetProcAddress(moduleGTAFmod, "Ext_GetCurrentGear");
            gtaFmod_Ext_SetVehicleFMODBank = (GTAFmod_Ext_SetVehicleFMODBank)GetProcAddress(moduleGTAFmod, "Ext_SetVehicleFMODBank");
        }
        else {
            moduleGTAFmod = 0;
            gtaFmod_Ext_GetCurrentRPM = 0;
            gtaFmod_Ext_GetCurrentGear = 0;
            gtaFmod_Ext_SetVehicleFMODBank = 0;
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

                if (KeyPressed(VK_F8) && CTimer::m_snTimeInMilliseconds >= (keyTime + 2000))
                {
                    gtaFmod_Ext_SetVehicleFMODBank(vehicle, "ks_mazda_rx7_spirit_r");
                    keyTime = CTimer::m_snTimeInMilliseconds;
                }
            }
        };
    }
} gTAFmodDashboard;
