#include "plugin.h"
#include "CMessages.h"

using namespace plugin;

typedef float (__cdecl* GTAFmod_Ext_GetCurrentRPM)();
GTAFmod_Ext_GetCurrentRPM gtaFmod_Ext_GetCurrentRPM;

class GTAFmodDashboard {
public:
    GTAFmodDashboard() {
        HINSTANCE moduleGTAFmod = GetModuleHandleA("GTAFmod.SA.asi");
        if (moduleGTAFmod) {
            gtaFmod_Ext_GetCurrentRPM = (GTAFmod_Ext_GetCurrentRPM)GetProcAddress(moduleGTAFmod, "Ext_GetCurrentRPM");
        }
        else {
            moduleGTAFmod = 0;
            gtaFmod_Ext_GetCurrentRPM = 0;
        }

        Events::gameProcessEvent.before += []() {
            if (gtaFmod_Ext_GetCurrentRPM) {
                float rpm = gtaFmod_Ext_GetCurrentRPM();
                CMessages::AddMessageJumpQWithNumber(new char[] {"RPM ~1~"}, 3000, 0, rpm, 0, 0, 0, 0, 0, false);
            }
        };
    }
} gTAFmodDashboard;
