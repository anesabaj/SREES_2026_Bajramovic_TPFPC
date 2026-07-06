#include "TpfpcPluginApi.h"

#include "MatpowerPowerFlow.h"
#include "PowerFlowMatrixModel.h"

#include <algorithm>
#include <cstring>
#include <filesystem>
#include <sstream>
#include <string>

namespace
{
TpfpcPluginPolarPower makePluginPolarPower(double magnitude, double angleDeg)
{
    return {magnitude, angleDeg};
}

void copyText(char* destination, std::size_t destinationSize, const std::string& text)
{
    if (destination == nullptr || destinationSize == 0)
        return;

    const auto copySize = std::min(destinationSize - 1, text.size());
    std::memcpy(destination, text.c_str(), copySize);
    destination[copySize] = '\0';
}

std::string makeBusName(int bus)
{
    constexpr int threePhaseBusOffset = 1000000;
    if (bus >= threePhaseBusOffset)
    {
        const int encoded = bus - threePhaseBusOffset;
        const int physicalBus = encoded / 10;
        const int phaseIndex = encoded % 10 - 1;
        static constexpr char phases[] = {'A', 'B', 'C'};
        const char phase = phaseIndex >= 0 && phaseIndex < 3 ? phases[phaseIndex] : 'X';
        return std::to_string(physicalBus) + phase;
    }

    return std::to_string(bus);
}

std::string makeBusList(const std::vector<int>& buses)
{
    if (buses.empty())
        return "-";

    std::ostringstream out;
    for (std::size_t i = 0; i < buses.size(); ++i)
    {
        if (i != 0)
            out << ", ";
        out << makeBusName(buses[i]);
    }

    return out.str();
}

void clearMatpowerResult(TpfpcMatpowerResult& result)
{
    result.baseMVA = 0.0;
    result.busCount = 0;
    result.generatorCount = 0;
    result.branchCount = 0;
    result.pqCount = 0;
    result.pvCount = 0;
    result.slackCount = 0;
    result.ybusNonZeroCount = 0;
    result.pqBuses[0] = '\0';
    result.pvBuses[0] = '\0';
    result.slackBuses[0] = '\0';
    result.outputPath[0] = '\0';
    result.message[0] = '\0';
}

void fillMatpowerResult(const tpfpc::matpower::PowerFlowModel& model, const std::filesystem::path& outputPath, TpfpcMatpowerResult& result)
{
    result.baseMVA = model.inputCase.baseMVA;
    result.busCount = static_cast<unsigned int>(model.inputCase.buses.size());
    result.generatorCount = static_cast<unsigned int>(model.inputCase.generators.size());
    result.branchCount = static_cast<unsigned int>(model.inputCase.branches.size());
    result.pqCount = static_cast<unsigned int>(model.pqBuses.size());
    result.pvCount = static_cast<unsigned int>(model.pvBuses.size());
    result.slackCount = static_cast<unsigned int>(model.slackBuses.size());
    result.ybusNonZeroCount = static_cast<unsigned int>(model.ybusSparse.size());
    copyText(result.pqBuses, sizeof(result.pqBuses), makeBusList(model.pqBuses));
    copyText(result.pvBuses, sizeof(result.pvBuses), makeBusList(model.pvBuses));
    copyText(result.slackBuses, sizeof(result.slackBuses), makeBusList(model.slackBuses));
    copyText(result.outputPath, sizeof(result.outputPath), outputPath.string());
    copyText(result.message, sizeof(result.message), "OK! MATPOWER .dmodl je generisan");
}
}

const char* tpfpcGetPluginName()
{
    return "SREES_2026_Bajramovic_TPFPC plugin";
}

unsigned int tpfpcGetPluginVersion()
{
    return 2;
}

int tpfpcConvertThreePhase(const TpfpcPluginInput* input, TpfpcPluginResult* result)
{
    if (input == nullptr || result == nullptr)
        return 0;

    tpfpc::ThreePhaseRectangularPower rectangularPower;
    rectangularPower.phaseA = {input->phaseA.p, input->phaseA.q};
    rectangularPower.phaseB = {input->phaseB.p, input->phaseB.q};
    rectangularPower.phaseC = {input->phaseC.p, input->phaseC.q};

    tpfpc::PowerFlowMatrixModel matrixModel;
    const auto conversionResult = matrixModel.convert(rectangularPower);
    auto polar = conversionResult.polarMatrix.getManipulator();

    result->phaseA = makePluginPolarPower(polar(0, 0), polar(0, 1));
    result->phaseB = makePluginPolarPower(polar(1, 0), polar(1, 1));
    result->phaseC = makePluginPolarPower(polar(2, 0), polar(2, 1));
    result->pTotal = conversionResult.pTotal;
    result->qTotal = conversionResult.qTotal;
    result->total = makePluginPolarPower(conversionResult.totalPolarPower.magnitude, conversionResult.totalPolarPower.angleDeg);
    result->sparseInputNonZeroCount = static_cast<unsigned int>(conversionResult.sparseInputNonZeroCount);
    result->sparseOutputNonZeroCount = static_cast<unsigned int>(conversionResult.sparseOutputNonZeroCount);

    return 1;
}

int tpfpcConvertMatpowerToDmodl(const char* inputPath, const char* outputPath, TpfpcMatpowerResult* result)
{
    if (result == nullptr)
        return 0;

    clearMatpowerResult(*result);

    if (inputPath == nullptr || *inputPath == '\0')
    {
        copyText(result->message, sizeof(result->message), "GRESKA! Prazna putanja MATPOWER fajla");
        return 0;
    }

    if (outputPath == nullptr || *outputPath == '\0')
    {
        copyText(result->message, sizeof(result->message), "GRESKA! Prazna putanja izlaznog .dmodl fajla");
        return 0;
    }

    std::string error;
    tpfpc::matpower::MatpowerCase matpowerCase;
    const std::filesystem::path inputFile(inputPath);
    const std::filesystem::path outputFile(outputPath);

    if (!tpfpc::matpower::loadCase(inputFile, matpowerCase, error))
    {
        copyText(result->message, sizeof(result->message), "GRESKA! MATPOWER parser: " + error);
        return 0;
    }

    tpfpc::matpower::PowerFlowModel model;
    if (!tpfpc::matpower::buildPowerFlowModel(matpowerCase, model, error))
    {
        copyText(result->message, sizeof(result->message), "GRESKA! Power-flow model: " + error);
        return 0;
    }

    if (!tpfpc::matpower::writeDmodl(model, inputFile, outputFile, error))
    {
        copyText(result->message, sizeof(result->message), "GRESKA! .dmodl export: " + error);
        return 0;
    }

    fillMatpowerResult(model, outputFile, *result);
    return 1;
}
