#include "PolarConverter.h"

#include <cmath>

namespace
{
constexpr double pi = 3.14159265358979323846;
}

namespace tpfpc
{
PolarPower PolarConverter::convertPhase(const RectangularPower& power) const
{
    PolarPower polarPower;
    polarPower.magnitude = std::sqrt(power.p * power.p + power.q * power.q);
    polarPower.angleRad = std::atan2(power.q, power.p);
    polarPower.angleDeg = polarPower.angleRad * 180.0 / pi;
    return polarPower;
}

ThreePhasePolarPower PolarConverter::convertThreePhase(const ThreePhaseRectangularPower& power) const
{
    ThreePhasePolarPower polarPower;
    polarPower.phaseA = convertPhase(power.phaseA);
    polarPower.phaseB = convertPhase(power.phaseB);
    polarPower.phaseC = convertPhase(power.phaseC);
    return polarPower;
}
}
