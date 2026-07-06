#include "MatricesView.h"

#include <gui/GridComposer.h>

MatricesView::MatricesView()
    : gui::View(8, 8, 8, 8)
    , _titleLabel("Matrices")
    , _grid(1, 1)
{
    _titleLabel.setResizable(20);

    gui::GridComposer gridComposer(_grid);
    gridComposer.appendRow(_titleLabel);

    setLayout(&_grid);
}
