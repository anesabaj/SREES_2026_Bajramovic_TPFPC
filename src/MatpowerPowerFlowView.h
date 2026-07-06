#pragma once

#include "TpfpcPluginApi.h"

#include <atomic>
#include <memory>
#include <thread>

#include <gui/Button.h>
#include <gui/GridLayout.h>
#include <gui/Label.h>
#include <gui/LineEdit.h>
#include <gui/ProgressIndicator.h>
#include <gui/View.h>

class MatpowerPowerFlowView : public gui::View
{
    gui::Label _titleLabel;
    gui::Label _inputLabel;
    gui::LineEdit _inputPathEdit;
    gui::Button _inputBrowseButton;
    gui::Label _outputLabel;
    gui::LineEdit _outputPathEdit;
    gui::Button _outputBrowseButton;
    gui::Button _convertButton;
    gui::Label _statusLabel;
    gui::Label _baseMvaLabel;
    gui::Label _countsLabel;
    gui::Label _busTypesLabel;
    gui::Label _ybusLabel;
    gui::Label _outputResultLabel;
    gui::Label _progressLabel;
    gui::ProgressIndicator _progressIndicator;
    gui::GridLayout _grid;

    std::thread _worker;
    std::thread _progressThread;
    std::atomic_bool _operationInProgress;
    std::atomic_bool _progressThreadRunning;
    std::atomic_int _progressPercent;
    std::shared_ptr<std::atomic_bool> _viewAlive;

public:
    MatpowerPowerFlowView();
    ~MatpowerPowerFlowView();

private:
    struct ConversionDisplayData
    {
        bool success = false;
        TpfpcMatpowerResult result = {};
    };

    void chooseInputPath();
    void chooseOutputPath();
    void setDefaultPaths();
    void setStatus(const char* status);
    void setProgress(double value, const char* status);
    bool beginOperation();
    void finishOperation();
    void convert();
    void workerMethod(td::String inputPath, td::String outputPath);
    void progressMethod();
    void postProgress(double value, const char* status);
    void postResult(const ConversionDisplayData& data);
    void finishConversion(const ConversionDisplayData& data);
};
