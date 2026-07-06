#pragma once

#include "MatpowerPowerFlowView.h"
#include "PowerFlowView.h"

#include <gui/StandardTabView.h>

class MainTabbedView : public gui::StandardTabView
{
protected:
    MatpowerPowerFlowView _matpowerView;
    PowerFlowView _calculationsView;

public:
    MainTabbedView();
};
