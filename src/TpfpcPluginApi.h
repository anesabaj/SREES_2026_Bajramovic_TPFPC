#pragma once

#ifdef MU_WINDOWS
    #ifdef TPFPC_PLUGIN_EXPORTS
        #define TPFPC_PLUGIN_API __declspec(dllexport)
    #else
        #define TPFPC_PLUGIN_API __declspec(dllimport)
    #endif
#else
    #ifdef TPFPC_PLUGIN_EXPORTS
        #define TPFPC_PLUGIN_API __attribute__((visibility("default")))
    #else
        #define TPFPC_PLUGIN_API
    #endif
#endif

extern "C"
{
struct TpfpcPluginRectangularPower
{
    double p;
    double q;
};

struct TpfpcPluginPolarPower
{
    double magnitude;
    double angleDeg;
};

struct TpfpcPluginInput
{
    TpfpcPluginRectangularPower phaseA;
    TpfpcPluginRectangularPower phaseB;
    TpfpcPluginRectangularPower phaseC;
};

struct TpfpcPluginResult
{
    TpfpcPluginPolarPower phaseA;
    TpfpcPluginPolarPower phaseB;
    TpfpcPluginPolarPower phaseC;
    double pTotal;
    double qTotal;
    TpfpcPluginPolarPower total;
    unsigned int sparseInputNonZeroCount;
    unsigned int sparseOutputNonZeroCount;
};

struct TpfpcMatpowerResult
{
    double baseMVA;
    unsigned int busCount;
    unsigned int generatorCount;
    unsigned int branchCount;
    unsigned int pqCount;
    unsigned int pvCount;
    unsigned int slackCount;
    unsigned int ybusNonZeroCount;
    char pqBuses[128];
    char pvBuses[128];
    char slackBuses[128];
    char outputPath[512];
    char message[512];
};

TPFPC_PLUGIN_API const char* tpfpcGetPluginName();
TPFPC_PLUGIN_API unsigned int tpfpcGetPluginVersion();
TPFPC_PLUGIN_API int tpfpcConvertThreePhase(const TpfpcPluginInput* input, TpfpcPluginResult* result);
TPFPC_PLUGIN_API int tpfpcConvertMatpowerToDmodl(const char* inputPath, const char* outputPath, TpfpcMatpowerResult* result);
}
