#include "MatpowerPowerFlowView.h"

#include <cstdio>
#include <filesystem>

#include <gui/FileDialog.h>
#include <gui/GridComposer.h>
#include <gui/Thread.h>
#include <thread/Thread.h>

#ifndef TPFPC_SOURCE_ROOT
#define TPFPC_SOURCE_ROOT "."
#endif

namespace
{
std::filesystem::path defaultCasePath()
{
    return std::filesystem::path(TPFPC_SOURCE_ROOT) / "res" / "t_case3p_a.m";
}

std::filesystem::path defaultOutputPath()
{
    return std::filesystem::path(TPFPC_SOURCE_ROOT) / "res" / "t_case3p_a_gui_output.dmodl";
}

const char* progressTextForPercent(int percent)
{
    if (percent < 15)
        return "Start MATPOWER konverzije:";
    if (percent < 30)
        return "MATPOWER fajl se parsira:";
    if (percent < 50)
        return "PQ/PV/slack klasifikacija:";
    if (percent < 70)
        return "Formiranje Ybus matrice:";
    if (percent < 90)
        return "Polarni Ybus i NLE model:";
    return "Generisanje .dmodl:";
}
}

MatpowerPowerFlowView::MatpowerPowerFlowView()
    : gui::View(8, 8, 8, 8)
    , _titleLabel("MATPOWER-to-dTwin polarni power-flow converter")
    , _inputLabel("MATPOWER .m fajl")
    , _inputBrowseButton("...")
    , _outputLabel("Izlazni .dmodl")
    , _outputBrowseButton("...")
    , _convertButton("Convert MATPOWER to Polar dTwin Model")
    , _statusLabel("Status: spreman")
    , _baseMvaLabel("baseMVA: -")
    , _countsLabel("Buses: -, Generators: -, Branches: -")
    , _busTypesLabel("PQ: -, PV: -, Slack: -")
    , _ybusLabel("Ybus sparse nnz: -")
    , _outputResultLabel("Output .dmodl: -")
    , _progressLabel("Progress: 0%")
    , _grid(13, 6)
    , _operationInProgress(false)
    , _progressThreadRunning(false)
    , _progressPercent(0)
    , _viewAlive(std::make_shared<std::atomic_bool>(true))
{
    _titleLabel.setResizable(80);
    _statusLabel.setResizable(100);
    _baseMvaLabel.setResizable(60);
    _countsLabel.setResizable(90);
    _busTypesLabel.setResizable(90);
    _ybusLabel.setResizable(60);
    _outputResultLabel.setResizable(110);
    _progressLabel.setResizable(28);
    _progressIndicator.setValue(0.0);
    _convertButton.setType(gui::Button::Type::Default);

    _inputPathEdit.setAsReadOnly();
    _outputPathEdit.setAsReadOnly();
    setDefaultPaths();

    _inputBrowseButton.onClick([this]()
    {
        chooseInputPath();
    });

    _outputBrowseButton.onClick([this]()
    {
        chooseOutputPath();
    });

    _convertButton.onClick([this]()
    {
        convert();
    });

    _grid.setSpaceBetweenCells(6, 10);
    gui::GridComposer gridComposer(_grid);
    gridComposer.appendRow(_titleLabel, 0);
    gridComposer.appendRow(_inputLabel);
    gridComposer.appendCol(_inputPathEdit, 4);
    gridComposer.appendCol(_inputBrowseButton);
    gridComposer.appendRow(_outputLabel);
    gridComposer.appendCol(_outputPathEdit, 4);
    gridComposer.appendCol(_outputBrowseButton);
    gridComposer.appendRow(_convertButton, 0, td::HAlignment::Right);
    gridComposer.appendRow(_statusLabel, 0);
    gridComposer.appendRow(_baseMvaLabel, 0);
    gridComposer.appendRow(_countsLabel, 0);
    gridComposer.appendRow(_busTypesLabel, 0);
    gridComposer.appendRow(_ybusLabel, 0);
    gridComposer.appendRow(_outputResultLabel, 0);
    gridComposer.appendRow(_progressLabel);
    gridComposer.appendCol(_progressIndicator, 5);

    setLayout(&_grid);
}

MatpowerPowerFlowView::~MatpowerPowerFlowView()
{
    *_viewAlive = false;
    _progressThreadRunning = false;
    _operationInProgress = false;

    if (_progressThread.joinable())
        _progressThread.join();

    if (_worker.joinable())
        _worker.join();
}

void MatpowerPowerFlowView::setDefaultPaths()
{
    _inputPathEdit = defaultCasePath().string().c_str();
    _outputPathEdit = defaultOutputPath().string().c_str();
}

void MatpowerPowerFlowView::chooseInputPath()
{
    gui::OpenFileDialog::show(this, "Odaberi MATPOWER .m fajl", "*.m", 2001, [this](gui::FileDialog* pFileDlg)
    {
        if (pFileDlg->getStatus() != gui::FileDialog::Status::OK)
            return;

        const std::filesystem::path inputPath(pFileDlg->getFileName().c_str());
        _inputPathEdit = inputPath.string().c_str();

        std::filesystem::path outputPath = inputPath;
        outputPath.replace_extension(".dmodl");
        _outputPathEdit = outputPath.string().c_str();
    });
}

void MatpowerPowerFlowView::chooseOutputPath()
{
    gui::SaveFileDialog::show(this, "Odaberi izlazni .dmodl", "*.dmodl", 2002, [this](gui::FileDialog* pFileDlg)
    {
        if (pFileDlg->getStatus() != gui::FileDialog::Status::OK)
            return;

        std::filesystem::path outputPath(pFileDlg->getFileName().c_str());
        if (outputPath.extension().empty())
            outputPath += ".dmodl";

        _outputPathEdit = outputPath.string().c_str();
    });
}

void MatpowerPowerFlowView::setStatus(const char* status)
{
    _statusLabel.setTitle(status);
}

void MatpowerPowerFlowView::setProgress(double value, const char* status)
{
    _progressIndicator.setValue(value);

    char text[96];
    std::snprintf(text, sizeof(text), "%s %.0f%%", status, value * 100.0);
    _progressLabel.setTitle(text);
}

bool MatpowerPowerFlowView::beginOperation()
{
    bool expected = false;
    if (!_operationInProgress.compare_exchange_strong(expected, true))
        return false;

    if (_worker.joinable())
        _worker.join();

    if (_progressThread.joinable())
        _progressThread.join();

    _convertButton.disable();
    _progressPercent = 0;
    setProgress(0.0, "Start MATPOWER konverzije:");
    setStatus("Status: konverzija pokrenuta");

    _progressThreadRunning = true;
    _progressThread = std::thread(&MatpowerPowerFlowView::progressMethod, this);
    return true;
}

void MatpowerPowerFlowView::finishOperation()
{
    _progressThreadRunning = false;
    _operationInProgress = false;
    _progressPercent = 100;
    setProgress(1.0, "Gotovo:");
    _convertButton.enable();

    if (_progressThread.joinable())
        _progressThread.join();

    if (_worker.joinable())
        _worker.join();
}

void MatpowerPowerFlowView::convert()
{
    if (_operationInProgress.load())
        return;

    const td::String inputPath = _inputPathEdit.getText();
    const td::String outputPath = _outputPathEdit.getText();

    if (inputPath.isEmpty())
    {
        setStatus("GRESKA! MATPOWER input putanja je prazna");
        return;
    }

    if (outputPath.isEmpty())
    {
        setStatus("GRESKA! Izlazna .dmodl putanja je prazna");
        return;
    }

    if (!beginOperation())
        return;

    _worker = std::thread(&MatpowerPowerFlowView::workerMethod, this, inputPath, outputPath);
}

void MatpowerPowerFlowView::workerMethod(td::String inputPath, td::String outputPath)
{
    ConversionDisplayData data;
    _progressPercent = 15;
    thread::sleepMilliSeconds(80);
    _progressPercent = 30;

    data.success = tpfpcConvertMatpowerToDmodl(inputPath.c_str(), outputPath.c_str(), &data.result) != 0;
    _progressPercent = data.success ? 90 : 100;

    auto viewAlive = _viewAlive;
    gui::thread::asyncExecInMainThread([this, data, viewAlive]()
    {
        if (!*viewAlive)
            return;

        finishConversion(data);
    });
}

void MatpowerPowerFlowView::progressMethod()
{
    int localPercent = 0;
    while (_progressThreadRunning && _operationInProgress)
    {
        const int target = _progressPercent.load();
        if (localPercent < target)
            localPercent += 5;
        else if (localPercent < 90)
            localPercent += 2;

        if (localPercent > 90)
            localPercent = 90;

        postProgress(static_cast<double>(localPercent) / 100.0, progressTextForPercent(localPercent));
        thread::sleepMilliSeconds(100);
    }
}

void MatpowerPowerFlowView::postProgress(double value, const char* status)
{
    auto viewAlive = _viewAlive;
    gui::thread::asyncExecInMainThread([this, value, status, viewAlive]()
    {
        if (!*viewAlive)
            return;

        if (!_operationInProgress)
            return;

        setProgress(value, status);
    });
}

void MatpowerPowerFlowView::finishConversion(const ConversionDisplayData& data)
{
    if (!data.success)
    {
        _baseMvaLabel.setTitle("baseMVA: -");
        _countsLabel.setTitle("Buses: -, Generators: -, Branches: -");
        _busTypesLabel.setTitle("PQ: -, PV: -, Slack: -");
        _ybusLabel.setTitle("Ybus sparse nnz: -");
        _outputResultLabel.setTitle("Output .dmodl: -");

        char text[620];
        std::snprintf(text, sizeof(text), "Status: %s", data.result.message);
        _statusLabel.setTitle(text);
        finishOperation();
        return;
    }

    char text[620];
    std::snprintf(text, sizeof(text), "baseMVA: %.2f", data.result.baseMVA);
    _baseMvaLabel.setTitle(text);

    std::snprintf(
        text,
        sizeof(text),
        "Buses: %u, Generators: %u, Branches: %u",
        data.result.busCount,
        data.result.generatorCount,
        data.result.branchCount);
    _countsLabel.setTitle(text);

    std::snprintf(
        text,
        sizeof(text),
        "PQ: %u [%s], PV: %u [%s], Slack: %u [%s]",
        data.result.pqCount,
        data.result.pqBuses,
        data.result.pvCount,
        data.result.pvBuses,
        data.result.slackCount,
        data.result.slackBuses);
    _busTypesLabel.setTitle(text);

    std::snprintf(text, sizeof(text), "Ybus sparse nnz: %u", data.result.ybusNonZeroCount);
    _ybusLabel.setTitle(text);

    std::snprintf(text, sizeof(text), "Output .dmodl: %s", data.result.outputPath);
    _outputResultLabel.setTitle(text);

    std::snprintf(text, sizeof(text), "Status: %s", data.result.message);
    _statusLabel.setTitle(text);

    finishOperation();
}
