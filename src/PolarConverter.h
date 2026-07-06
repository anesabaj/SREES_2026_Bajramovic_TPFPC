#pragma once

#include "PowerFlowTypes.h"

namespace tpfpc
{
class PolarConverter
{
public:
    PolarPower convertPhase(const RectangularPower& power) const;
    ThreePhasePolarPower convertThreePhase(const ThreePhaseRectangularPower& power) const;
};
}
