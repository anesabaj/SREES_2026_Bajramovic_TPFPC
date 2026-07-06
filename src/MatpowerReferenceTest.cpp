#include "PowerFlowMatrixModel.h"

#include <cmath>
#include <iostream>

namespace
{
constexpr double kTolerance = 1.0e-6;

bool nearlyEqual(double actual, double expected, double tolerance = kTolerance)
{
    return std::abs(actual - expected) <= tolerance;
}

bool expectNear(const char* label, double actual, double expected)
{
    if (nearlyEqual(actual, expected))
        return true;

    std::cerr << label << " mismatch. actual=" << actual << " expected=" << expected << '\n';
    return false;
}

bool expectEqual(const char* label, std::size_t actual, std::size_t expected)
{
    if (actual == expected)
        return true;

    std::cerr << label << " mismatch. actual=" << actual << " expected=" << expected << '\n';
    return false;
}
}

int main()
{
    // MATPOWER t_case3p_a generator result, used only as reference P/Q input data.
    tpfpc::ThreePhaseRectangularPower input;
    input.phaseA = {1341.42, 970.52};
    input.phaseB = {2096.10, 1341.41};
    input.phaseC = {2672.34, 1894.59};

    tpfpc::PowerFlowMatrixModel model;
    const auto result = model.convert(input);

    auto denseInput = result.inputMatrix.getManipulator();
    auto denseOutput = result.polarMatrix.getManipulator();

    bool ok = true;

    ok = expectNear("input A P", denseInput(0, 0), 1341.42) && ok;
    ok = expectNear("input A Q", denseInput(0, 1), 970.52) && ok;
    ok = expectNear("input B P", denseInput(1, 0), 2096.10) && ok;
    ok = expectNear("input B Q", denseInput(1, 1), 1341.41) && ok;
    ok = expectNear("input C P", denseInput(2, 0), 2672.34) && ok;
    ok = expectNear("input C Q", denseInput(2, 1), 1894.59) && ok;

    ok = expectNear("output A magnitude", denseOutput(0, 0), 1655.6922077488) && ok;
    ok = expectNear("output A angleDeg", denseOutput(0, 1), 35.8858120250) && ok;
    ok = expectNear("output B magnitude", denseOutput(1, 0), 2488.5771031053) && ok;
    ok = expectNear("output B angleDeg", denseOutput(1, 1), 32.6174202191) && ok;
    ok = expectNear("output C magnitude", denseOutput(2, 0), 3275.8010232155) && ok;
    ok = expectNear("output C angleDeg", denseOutput(2, 1), 35.3352273780) && ok;

    ok = expectEqual("sparse input nnz", result.sparseInputNonZeroCount, 6) && ok;
    ok = expectEqual("sparse output nnz", result.sparseOutputNonZeroCount, 6) && ok;

    ok = expectNear("total P", result.pTotal, 6109.86) && ok;
    ok = expectNear("total Q", result.qTotal, 4206.52) && ok;
    ok = expectNear("total magnitude", result.totalPolarPower.magnitude, 7417.8972579836) && ok;
    ok = expectNear("total angleDeg", result.totalPolarPower.angleDeg, 34.5466566573) && ok;

    if (!ok)
        return 1;

    std::cout << "MATPOWER t_case3p_a reference conversion: OK\n";
    return 0;
}
