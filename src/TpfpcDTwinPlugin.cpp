#include "TpfpcDTwinPlugin.h"

#include "MatpowerPowerFlow.h"

#include <arch/MemoryOut.h>
#include <gui/Button.h>
#include <gui/FileDialog.h>
#include <gui/GridComposer.h>
#include <gui/GridLayout.h>
#include <gui/Label.h>
#include <gui/LineEdit.h>
#include <gui/View.h>
#include <gui/Window.h>

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>

#ifndef TPFPC_SOURCE_ROOT
#define TPFPC_SOURCE_ROOT "."
#endif

namespace
{
std::filesystem::path getDefaultInputPath()
{
    return std::filesystem::path(TPFPC_SOURCE_ROOT) / "res" / "t_case3p_a.m";
}

std::filesystem::path getDefaultOutputPath()
{
    const char* tempPath = std::getenv("TEMP");
    if (tempPath == nullptr || *tempPath == '\0')
        tempPath = std::getenv("TMP");

    const std::filesystem::path outputFolder = (tempPath != nullptr && *tempPath != '\0')
        ? std::filesystem::path(tempPath)
        : std::filesystem::current_path();

    return outputFolder / "SREES_2026_Bajramovic_TPFPC_3P_MATPOWER_PF.dmodl";
}

std::string makeShortSummary(const tpfpc::matpower::PowerFlowModel& model)
{
    std::ostringstream out;
    out << "baseMVA=" << model.inputCase.baseMVA
        << ", buses=" << model.inputCase.buses.size()
        << ", gen=" << model.inputCase.generators.size()
        << ", branch=" << model.inputCase.branches.size()
        << ", PQ=" << model.pqBuses.size()
        << ", PV=" << model.pvBuses.size()
        << ", Slack=" << model.slackBuses.size()
        << ", Ybus nnz=" << model.ybusSparse.size();
    return out.str();
}

void writeArchive(arch::MemoryOut* archive, const std::string& content)
{
    if (archive == nullptr)
        return;

    archive->put(content.c_str(), content.length());
}

class TpfpcDTwinPluginWindow;

class TpfpcDTwinPlugin : public sc::IPlugin
{
    MemoryArchiveContainer _outArchives;
    TpfpcDTwinPluginWindow* _pWnd = nullptr;
    td::String _outFileName;

public:
    TpfpcDTwinPlugin();

    void show(gui::Window* parentWnd, MemoryArchiveContainer& archives, td::UINT4 wndID, const Cleaner& cleaner, const CallBack& onComplete) override final;
    td::String getMenuName() const override final;
    MemoryArchiveContainer& getArchives() override final;
    arch::MemoryOut* getArchive(IPlugin::ArchType type) override final;
    td::String getOutFileName() const override final;
    size_t getMaxSupportedArchiveParts() const override final;
    ModelType getModelType() const override final;

    bool createMatpowerModel(const td::String& inputFileName, const td::String& outFileName, td::String& status, std::string& summary);
    void onClosedPluginWindow();
};

class TpfpcDTwinPluginView : public gui::View
{
    TpfpcDTwinPlugin* _pPlugin;
    sc::IPlugin::CallBack _onComplete;

    gui::Label _titleLabel;
    gui::Label _inputPathLabel;
    gui::LineEdit _inputPathEdit;
    gui::Button _inputBrowseButton;
    gui::Label _outputPathLabel;
    gui::LineEdit _outputPathEdit;
    gui::Button _outputBrowseButton;
    gui::Label _summaryLabel;
    gui::Label _statusLabel;
    gui::LineEdit _statusEdit;
    gui::Button _convertButton;
    gui::GridLayout _grid;

public:
    TpfpcDTwinPluginView(TpfpcDTwinPlugin* pPlugin, const sc::IPlugin::CallBack& onComplete)
        : gui::View(8, 8, 8, 8)
        , _pPlugin(pPlugin)
        , _onComplete(onComplete)
        , _titleLabel("MATPOWER-to-dTwin polarni power-flow converter")
        , _inputPathLabel("MATPOWER .m")
        , _inputBrowseButton("...")
        , _outputPathLabel("Izlazni .dmodl")
        , _outputBrowseButton("...")
        , _summaryLabel("Summary: -")
        , _statusLabel("Status")
        , _convertButton("Konvertuj MATPOWER")
        , _grid(8, 6)
    {
        _inputPathEdit.setAsReadOnly();
        _outputPathEdit.setAsReadOnly();
        _statusEdit.setAsReadOnly();
        _inputPathEdit = getDefaultInputPath().string().c_str();
        _outputPathEdit = getDefaultOutputPath().string().c_str();
        _statusEdit = "Spreman";

        _titleLabel.setResizable(80);
        _summaryLabel.setResizable(110);
        _convertButton.setType(gui::Button::Type::Default);

        gui::GridComposer gridComposer(_grid);
        gridComposer.appendRow(_titleLabel, 0);
        gridComposer.appendRow(_inputPathLabel);
        gridComposer.appendCol(_inputPathEdit, 4);
        gridComposer.appendCol(_inputBrowseButton);
        gridComposer.appendRow(_outputPathLabel);
        gridComposer.appendCol(_outputPathEdit, 4);
        gridComposer.appendCol(_outputBrowseButton);
        gridComposer.appendRow(_summaryLabel, 0);
        gridComposer.appendRow(_statusLabel);
        gridComposer.appendCol(_statusEdit, 5);
        gridComposer.appendRow(_convertButton, 0, td::HAlignment::Right);

        setLayout(&_grid);

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
    }

    td::String getOutFileName() const
    {
        return _outputPathEdit.getText();
    }

private:
    void chooseInputPath()
    {
        gui::OpenFileDialog::show(this, "Odaberi MATPOWER .m fajl", "*.m", 3001, [this](gui::FileDialog* pFileDlg)
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

    void chooseOutputPath()
    {
        gui::SaveFileDialog::show(this, "Odaberi izlazni .dmodl", "*.dmodl", 3002, [this](gui::FileDialog* pFileDlg)
        {
            if (pFileDlg->getStatus() != gui::FileDialog::Status::OK)
                return;

            std::filesystem::path outputPath(pFileDlg->getFileName().c_str());
            if (outputPath.extension().empty())
                outputPath += ".dmodl";

            _outputPathEdit = outputPath.string().c_str();
        });
    }

    void convert()
    {
        if (_pPlugin == nullptr)
            return;

        td::String status;
        std::string summary;
        if (!_pPlugin->createMatpowerModel(_inputPathEdit.getText(), _outputPathEdit.getText(), status, summary))
        {
            _statusEdit = status;
            return;
        }

        _summaryLabel.setTitle(summary.c_str());
        _statusEdit = status;

        if (_onComplete)
            _onComplete(_pPlugin);

        gui::Window* pWnd = getParentWindow();
        if (pWnd != nullptr)
            pWnd->close();
    }
};

class TpfpcDTwinPluginWindow : public gui::Window
{
    TpfpcDTwinPlugin* _pPlugin;
    TpfpcDTwinPluginView _view;
    sc::IPlugin::Cleaner _cleaner;

public:
    TpfpcDTwinPluginWindow(gui::Window* parentWnd, TpfpcDTwinPlugin* pPlugin, const sc::IPlugin::CallBack& onComplete, const sc::IPlugin::Cleaner& cleaner, td::UINT4 wndID)
        : gui::Window(gui::Size(980, 420), parentWnd, wndID)
        , _pPlugin(pPlugin)
        , _view(pPlugin, onComplete)
        , _cleaner(cleaner)
    {
        setTitle("TPFPC MATPOWER Polar PF Converter");
        setCentralView(&_view);
    }

    td::String getOutFileName() const
    {
        return _view.getOutFileName();
    }

protected:
    void onClose() override final
    {
        if (_cleaner)
            _cleaner();

        if (_pPlugin != nullptr)
            _pPlugin->onClosedPluginWindow();
    }
};

TpfpcDTwinPlugin::TpfpcDTwinPlugin()
{
    for (size_t i = 0; i < size_t(ArchType::NA); ++i)
        _outArchives[i] = nullptr;
}

void TpfpcDTwinPlugin::show(gui::Window* parentWnd, MemoryArchiveContainer& archives, td::UINT4 wndID, const Cleaner& cleaner, const CallBack& onComplete)
{
    for (size_t i = 0; i < size_t(ArchType::NA); ++i)
        _outArchives[i] = archives[i];

    if (_pWnd != nullptr)
    {
        _pWnd->setFocus();
        return;
    }

    _pWnd = new TpfpcDTwinPluginWindow(parentWnd, this, onComplete, cleaner, wndID);
    _pWnd->open();
}

td::String TpfpcDTwinPlugin::getMenuName() const
{
    return "TPFPC MATPOWER Polar PF Converter";
}

TpfpcDTwinPlugin::MemoryArchiveContainer& TpfpcDTwinPlugin::getArchives()
{
    return _outArchives;
}

arch::MemoryOut* TpfpcDTwinPlugin::getArchive(IPlugin::ArchType type)
{
    const auto iType = size_t(type);
    if (iType >= getMaxSupportedArchiveParts())
        return nullptr;

    return _outArchives[iType];
}

td::String TpfpcDTwinPlugin::getOutFileName() const
{
    if (_pWnd != nullptr)
        return _pWnd->getOutFileName();

    return _outFileName;
}

size_t TpfpcDTwinPlugin::getMaxSupportedArchiveParts() const
{
    return size_t(ArchType::NA);
}

TpfpcDTwinPlugin::ModelType TpfpcDTwinPlugin::getModelType() const
{
    return ModelType::NL;
}

bool TpfpcDTwinPlugin::createMatpowerModel(const td::String& inputFileName, const td::String& outFileName, td::String& status, std::string& summary)
{
    if (inputFileName.isEmpty())
    {
        status = "GRESKA! Prazna putanja MATPOWER .m fajla";
        return false;
    }

    if (outFileName.isEmpty())
    {
        status = "GRESKA! Prazna putanja za izlazni .dmodl";
        return false;
    }

    std::string error;
    tpfpc::matpower::MatpowerCase matpowerCase;
    const std::filesystem::path inputPath(inputFileName.c_str());
    const std::filesystem::path outputPath(outFileName.c_str());

    if (!tpfpc::matpower::loadCase(inputPath, matpowerCase, error))
    {
        status = ("GRESKA! MATPOWER parser: " + error).c_str();
        return false;
    }

    tpfpc::matpower::PowerFlowModel model;
    if (!tpfpc::matpower::buildPowerFlowModel(matpowerCase, model, error))
    {
        status = ("GRESKA! Power-flow model: " + error).c_str();
        return false;
    }

    const auto dmodlContent = tpfpc::matpower::createDmodl(model, inputPath);
    writeArchive(getArchive(ArchType::DigitalModel), dmodlContent);

    const auto visualModelContent = tpfpc::matpower::createVoltagePhaseVisualModel(model);
    if (!visualModelContent.empty())
        writeArchive(getArchive(ArchType::VisualModel), visualModelContent);

    std::ofstream fileOut(outputPath, std::ios::binary);
    if (!fileOut)
    {
        status = "GRESKA! Nije moguce kreirati izlazni .dmodl fajl";
        return false;
    }

    fileOut << dmodlContent;

    if (!visualModelContent.empty())
    {
        std::filesystem::path visualOutputPath = outputPath;
        visualOutputPath.replace_extension(".vmodl");

        std::ofstream visualFileOut(visualOutputPath, std::ios::binary);
        if (visualFileOut)
            visualFileOut << visualModelContent;
    }

    _outFileName = outFileName;
    summary = makeShortSummary(model);
    status = "OK! MATPOWER polarni .dmodl je generisan";
    return true;
}

void TpfpcDTwinPlugin::onClosedPluginWindow()
{
    _pWnd = nullptr;
    for (size_t i = 0; i < size_t(ArchType::NA); ++i)
        _outArchives[i] = nullptr;
}

TpfpcDTwinPlugin s_plugin;
}

extern "C"
{
PLUGIN_API sc::IPlugin* getPluginInterface()
{
    return &s_plugin;
}
}
