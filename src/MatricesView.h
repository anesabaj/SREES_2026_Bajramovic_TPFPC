#pragma once

#include <gui/GridLayout.h>
#include <gui/Label.h>
#include <gui/View.h>

class MatricesView : public gui::View
{
protected:
    gui::Label _titleLabel;
    gui::GridLayout _grid;

public:
    MatricesView();
};
