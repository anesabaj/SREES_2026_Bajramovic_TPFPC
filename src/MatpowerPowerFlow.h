#pragma once

#include <complex>
#include <filesystem>
#include <string>
#include <vector>

namespace tpfpc::matpower
{
constexpr int kBusTypePQ = 1;
constexpr int kBusTypePV = 2;
constexpr int kBusTypeSlack = 3;

struct Bus
{
    int id = 0;
    int type = kBusTypePQ;
    double pd = 0.0;
    double qd = 0.0;
    double gs = 0.0;
    double bs = 0.0;
    double vm = 1.0;
    double vaDeg = 0.0;
};

struct Generator
{
    int bus = 0;
    double pg = 0.0;
    double qg = 0.0;
    double vg = 1.0;
    int status = 1;
};

struct Branch
{
    int fromBus = 0;
    int toBus = 0;
    double r = 0.0;
    double x = 0.0;
    double b = 0.0;
    double ratio = 0.0;
    double angleDeg = 0.0;
    int status = 1;
};

struct YbusContribution
{
    int rowBus = 0;
    int colBus = 0;
    std::complex<double> value = {0.0, 0.0};
};

struct MatpowerCase
{
    double baseMVA = 100.0;
    double baseKVA = 0.0;
    bool isThreePhase = false;
    int threePhaseBusCount = 0;
    int threePhaseGeneratorCount = 0;
    int threePhaseLineCount = 0;
    int threePhaseTransformerCount = 0;
    int threePhaseLoadCount = 0;
    std::vector<Bus> buses;
    std::vector<Generator> generators;
    std::vector<Branch> branches;
    std::vector<YbusContribution> ybusContributions;
};

struct YbusEntry
{
    int rowBus = 0;
    int colBus = 0;
    std::complex<double> value = {0.0, 0.0};
    double magnitude = 0.0;
    double angleRad = 0.0;
};

struct BusInjection
{
    int bus = 0;
    double p = 0.0;
    double q = 0.0;
};

struct PowerFlowModel
{
    MatpowerCase inputCase;
    std::vector<int> pqBuses;
    std::vector<int> pvBuses;
    std::vector<int> slackBuses;
    std::vector<BusInjection> injections;
    std::vector<std::vector<std::complex<double>>> ybusDense;
    std::vector<YbusEntry> ybusSparse;
};

bool loadCase(const std::filesystem::path& filePath, MatpowerCase& matpowerCase, std::string& error);
bool buildPowerFlowModel(const MatpowerCase& matpowerCase, PowerFlowModel& model, std::string& error);
std::string createDmodl(const PowerFlowModel& model);
std::string createDmodl(const PowerFlowModel& model, const std::filesystem::path& inputPath);
std::string createVoltagePhaseVisualModel(const PowerFlowModel& model);
bool writeDmodl(const PowerFlowModel& model, const std::filesystem::path& filePath, std::string& error);
bool writeDmodl(const PowerFlowModel& model, const std::filesystem::path& inputPath, const std::filesystem::path& filePath, std::string& error);
std::string makeSummary(const PowerFlowModel& model, const std::filesystem::path& dmodlPath);
}
