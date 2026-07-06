#pragma once

#include "PolarConverter.h"

#include <dense/Matrix.h>

#include <cstddef>

namespace tpfpc
{
struct MatrixConversionResult
{
    MatrixConversionResult();

    dense::DblMatrix inputMatrix;
    dense::DblMatrix polarMatrix;
    double pTotal = 0.0;
    double qTotal = 0.0;
    PolarPower totalPolarPower;
    std::size_t sparseInputNonZeroCount = 0;
    std::size_t sparseOutputNonZeroCount = 0;
};

class PowerFlowMatrixModel
{
public:
    MatrixConversionResult convert(const ThreePhaseRectangularPower& rectangularPower) const;

private:
    PolarConverter _converter;
};
}
