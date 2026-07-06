#pragma once

#include "MainTabbedView.h"

#include <gui/Window.h>

class MainWindow : public gui::Window
{
protected:
    MainTabbedView _mainView;

public:
    MainWindow()
        : gui::Window(gui::Size(1180, 960))
    {
        setTitle("SREES 2026 Bajramovic TPFPC");
        setCentralView(&_mainView, gui::Frame::FixSizes::No);
    }
};
