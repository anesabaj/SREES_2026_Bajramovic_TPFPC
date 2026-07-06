#include "MainTabbedView.h"

MainTabbedView::MainTabbedView()
{
    addView(&_matpowerView, "MATPOWER PF Converter");
    addView(&_calculationsView, "P/Q Demo");
}
