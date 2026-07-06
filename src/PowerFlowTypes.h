#pragma once

namespace tpfpc
{
enum class Phase
{
    A,
    B,
    C
};

struct RectangularPower
{
    double p = 0.0;
    double q = 0.0;
};

struct PolarPower
{
    double magnitude = 0.0;
    double angleRad = 0.0;
    double angleDeg = 0.0;
};

struct ThreePhaseRectangularPower
{
    RectangularPower phaseA;
    RectangularPower phaseB;
    RectangularPower phaseC;
};

struct ThreePhasePolarPower
{
    PolarPower phaseA;
    PolarPower phaseB;
    PolarPower phaseC;
};
}
