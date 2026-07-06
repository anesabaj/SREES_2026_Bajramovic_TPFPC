#include "PowerFlowMatrixModel.h"

#include <sparse/IMatrix.h>
#include <td/Types.h>

namespace
{
constexpr td::UINT4 kPhaseA = 0;
constexpr td::UINT4 kPhaseB = 1;
constexpr td::UINT4 kPhaseC = 2;
constexpr td::UINT4 kPColumn = 0;
constexpr td::UINT4 kQColumn = 1;
constexpr td::UINT4 kMagnitudeColumn = 0;
constexpr td::UINT4 kAngleColumn = 1;
constexpr int kPhaseCount = 3;
constexpr int kColumnCount = 2;
constexpr double kZeroTolerance = 1.0e-12;

bool isNonZero(double value)
{
    return value > kZeroTolerance || value < -kZeroTolerance;
}

void addSparseValue(sparse::CmplxMatrixReleaser& matrix, int row, int col, double value, std::size_t& nonZeroCount)
{
    if (!isNonZero(value))
        return;

    matrix->addTriple(row, col, td::cmplx(value, 0.0));
    ++nonZeroCount;
}

void writePolarToMatrix(dense::DblMatrix& polarMatrix, td::UINT4 row, const tpfpc::PolarPower& polarPower)
{
    auto polar = polarMatrix.getManipulator();
    polar(row, kMagnitudeColumn) = polarPower.magnitude;
    polar(row, kAngleColumn) = polarPower.angleDeg;
}
}

namespace tpfpc
{
MatrixConversionResult::MatrixConversionResult()
    : inputMatrix(3, 2)
    , polarMatrix(3, 2)
{
}

MatrixConversionResult PowerFlowMatrixModel::convert(const ThreePhaseRectangularPower& rectangularPower) const
{
    MatrixConversionResult result;
    sparse::CmplxMatrixReleaser sparseInput(
        sparse::createCmplxMatrix(kPhaseCount, kColumnCount, kPhaseCount * kColumnCount));
    sparse::CmplxMatrixReleaser sparseOutput(
        sparse::createCmplxMatrix(kPhaseCount, kColumnCount, kPhaseCount * kColumnCount));

    auto input = result.inputMatrix.getManipulator();
    input(kPhaseA, kPColumn) = rectangularPower.phaseA.p;
    input(kPhaseA, kQColumn) = rectangularPower.phaseA.q;
    input(kPhaseB, kPColumn) = rectangularPower.phaseB.p;
    input(kPhaseB, kQColumn) = rectangularPower.phaseB.q;
    input(kPhaseC, kPColumn) = rectangularPower.phaseC.p;
    input(kPhaseC, kQColumn) = rectangularPower.phaseC.q;

    addSparseValue(sparseInput, kPhaseA, kPColumn, input(kPhaseA, kPColumn), result.sparseInputNonZeroCount);
    addSparseValue(sparseInput, kPhaseA, kQColumn, input(kPhaseA, kQColumn), result.sparseInputNonZeroCount);
    addSparseValue(sparseInput, kPhaseB, kPColumn, input(kPhaseB, kPColumn), result.sparseInputNonZeroCount);
    addSparseValue(sparseInput, kPhaseB, kQColumn, input(kPhaseB, kQColumn), result.sparseInputNonZeroCount);
    addSparseValue(sparseInput, kPhaseC, kPColumn, input(kPhaseC, kPColumn), result.sparseInputNonZeroCount);
    addSparseValue(sparseInput, kPhaseC, kQColumn, input(kPhaseC, kQColumn), result.sparseInputNonZeroCount);

    const auto phaseA = _converter.convertPhase({input(kPhaseA, kPColumn), input(kPhaseA, kQColumn)});
    const auto phaseB = _converter.convertPhase({input(kPhaseB, kPColumn), input(kPhaseB, kQColumn)});
    const auto phaseC = _converter.convertPhase({input(kPhaseC, kPColumn), input(kPhaseC, kQColumn)});

    writePolarToMatrix(result.polarMatrix, kPhaseA, phaseA);
    writePolarToMatrix(result.polarMatrix, kPhaseB, phaseB);
    writePolarToMatrix(result.polarMatrix, kPhaseC, phaseC);

    auto polar = result.polarMatrix.getManipulator();
    addSparseValue(sparseOutput, kPhaseA, kMagnitudeColumn, polar(kPhaseA, kMagnitudeColumn), result.sparseOutputNonZeroCount);
    addSparseValue(sparseOutput, kPhaseA, kAngleColumn, polar(kPhaseA, kAngleColumn), result.sparseOutputNonZeroCount);
    addSparseValue(sparseOutput, kPhaseB, kMagnitudeColumn, polar(kPhaseB, kMagnitudeColumn), result.sparseOutputNonZeroCount);
    addSparseValue(sparseOutput, kPhaseB, kAngleColumn, polar(kPhaseB, kAngleColumn), result.sparseOutputNonZeroCount);
    addSparseValue(sparseOutput, kPhaseC, kMagnitudeColumn, polar(kPhaseC, kMagnitudeColumn), result.sparseOutputNonZeroCount);
    addSparseValue(sparseOutput, kPhaseC, kAngleColumn, polar(kPhaseC, kAngleColumn), result.sparseOutputNonZeroCount);

    result.pTotal = input(kPhaseA, kPColumn) + input(kPhaseB, kPColumn) + input(kPhaseC, kPColumn);
    result.qTotal = input(kPhaseA, kQColumn) + input(kPhaseB, kQColumn) + input(kPhaseC, kQColumn);
    result.totalPolarPower = _converter.convertPhase({result.pTotal, result.qTotal});

    return result;
}
}
