#include <cnt/ListSL.h>

#include <arch/MemoryOut.h>
#include <gui/Application.h>
#include <gui/GridComposer.h>
#include <gui/GridLayout.h>
#include <gui/Label.h>
#include <gui/LineEdit.h>
#include <gui/View.h>
#include <gui/Window.h>
#include <gui/WinMain.h>
#include <sc/IPlugin.h>
#include <syst/LibraryLoader.h>

#include <fstream>
#include <string>

namespace
{
using GetPluginInterface = sc::IPlugin* (*)();

const char* testLogFileName()
{
    return "SREES_2026_Bajramovic_TPFPC_dTwinPluginGuiTest.log";
}

void appendLog(const std::string& line)
{
    std::ofstream logFile(testLogFileName(), std::ios::app | std::ios::binary);
    if (logFile)
        logFile << line << '\n';
}

const char* modelTypeName(sc::IPlugin::ModelType modelType)
{
    switch (modelType)
    {
    case sc::IPlugin::ModelType::NL:
        return "NL";
    case sc::IPlugin::ModelType::LSE:
        return "LSE";
    case sc::IPlugin::ModelType::MLE:
        return "MLE";
    case sc::IPlugin::ModelType::OPTIM:
        return "OPTIM";
    case sc::IPlugin::ModelType::ODE:
        return "ODE";
    case sc::IPlugin::ModelType::DAE:
        return "DAE";
    default:
        return "Unknown";
    }
}

class GuiTestView : public gui::View
{
    gui::Label _titleLabel;
    gui::Label _dllLabel;
    gui::LineEdit _dllValue;
    gui::Label _statusLabel;
    gui::LineEdit _statusValue;
    gui::Label _pluginLabel;
    gui::LineEdit _pluginValue;
    gui::Label _outputLabel;
    gui::LineEdit _outputValue;
    gui::GridLayout _grid;

public:
    GuiTestView()
        : gui::View(8, 8, 8, 8)
        , _titleLabel("Local dTwin plugin GUI test")
        , _dllLabel("DLL")
        , _statusLabel("Status")
        , _pluginLabel("Plugin")
        , _outputLabel("Output")
        , _grid(5, 2)
    {
        _dllValue.setAsReadOnly();
        _dllValue = TPFPC_DTWIN_PLUGIN_PATH;

        _statusValue.setAsReadOnly();
        _statusValue = "Starting";

        _pluginValue.setAsReadOnly();
        _pluginValue = "-";

        _outputValue.setAsReadOnly();
        _outputValue = "-";

        gui::GridComposer gridComposer(_grid);
        gridComposer.appendRow(_titleLabel, 0);
        gridComposer.appendRow(_dllLabel);
        gridComposer.appendCol(_dllValue);
        gridComposer.appendRow(_pluginLabel);
        gridComposer.appendCol(_pluginValue);
        gridComposer.appendRow(_statusLabel);
        gridComposer.appendCol(_statusValue);
        gridComposer.appendRow(_outputLabel);
        gridComposer.appendCol(_outputValue);

        setLayout(&_grid);
    }

    void setStatus(const char* status)
    {
        _statusValue = status;
        appendLog(std::string("Status: ") + status);
    }

    void setPlugin(sc::IPlugin* pPlugin)
    {
        if (pPlugin == nullptr)
        {
            _pluginValue = "-";
            return;
        }

        td::String pluginText;
        pluginText.format("%s (%s)", pPlugin->getMenuName().c_str(), modelTypeName(pPlugin->getModelType()));
        _pluginValue = pluginText;
        appendLog(std::string("Plugin: ") + pluginText.c_str());
    }

    void setOutput(const td::String& outputPath)
    {
        _outputValue = outputPath;
        appendLog(std::string("Output: ") + outputPath.c_str());
    }
};

class GuiTestWindow : public gui::Window
{
    GuiTestView _view;
    syst::LibraryLoader _library;
    sc::IPlugin* _pPlugin = nullptr;
    sc::IPlugin::MemoryArchiveContainer _archives;
    bool _cleanerCalled = false;
    bool _onCompleteCalled = false;

public:
    GuiTestWindow()
        : gui::Window(gui::Size(900, 260))
    {
        setTitle("TPFPC dTwin Plugin GUI Test");
        setCentralView(&_view);

        for (size_t i = 0; i < size_t(sc::IPlugin::ArchType::NA); ++i)
            _archives[i] = nullptr;
    }

    ~GuiTestWindow()
    {
        releaseArchives();
    }

protected:
    void onInitialAppearance() override
    {
        openPlugin();
    }

    void onClose() override
    {
        releaseArchives();
    }

private:
    void openPlugin()
    {
        appendLog("GUI test started");
        _view.setStatus("Loading DLL");

        const td::String pluginPath = TPFPC_DTWIN_PLUGIN_PATH;
        if (!_library.load(pluginPath))
        {
            _view.setStatus("ERROR! Cannot load plugin DLL");
            return;
        }
        _view.setStatus("DLL loaded");

        auto getPluginInterface = _library.getFunctionPtr<GetPluginInterface>("getPluginInterface");
        if (getPluginInterface == nullptr)
        {
            _view.setStatus("ERROR! getPluginInterface missing");
            return;
        }
        _view.setStatus("getPluginInterface OK");

        _pPlugin = getPluginInterface();
        if (_pPlugin == nullptr)
        {
            _view.setStatus("ERROR! Plugin interface is null");
            return;
        }
        _view.setPlugin(_pPlugin);

        if (!allocateArchives())
        {
            _view.setStatus("ERROR! Cannot allocate archives");
            return;
        }
        _view.setStatus("Archives ready; opening plugin window");

        auto cleaner = [this]()
        {
            releaseArchives();
            _cleanerCalled = true;
            appendLog("Cleaner: OK");

            if (_onCompleteCalled)
                _view.setStatus("Completed; cleaner OK");
            else
                _view.setStatus("Plugin window closed before Convert");
        };

        auto onComplete = [this](sc::IPlugin* pPlugin)
        {
            _onCompleteCalled = true;
            appendLog("onComplete: OK");

            if (pPlugin != nullptr)
                _view.setOutput(pPlugin->getOutFileName());

            _view.setStatus("onComplete OK");
        };

        _pPlugin->show(this, _archives, 7000, cleaner, onComplete);
    }

    bool allocateArchives()
    {
        releaseArchives();

        _archives[0] = arch::MemoryOut::allocate(arch::MemoryOut::PageSize::Normal);
        _archives[1] = arch::MemoryOut::allocate(arch::MemoryOut::PageSize::Small);

        if (_archives[0] == nullptr || _archives[1] == nullptr)
            return false;

        return _archives[0]->open(nullptr) && _archives[1]->open(nullptr);
    }

    void releaseArchives()
    {
        for (size_t i = 0; i < size_t(sc::IPlugin::ArchType::NA); ++i)
        {
            if (_archives[i] != nullptr)
            {
                _archives[i]->release();
                _archives[i] = nullptr;
            }
        }
    }
};

class GuiTestApplication : public gui::Application
{
protected:
    gui::Window* createInitialWindow() override
    {
        return new GuiTestWindow();
    }

public:
    GuiTestApplication(int argc, const char** argv)
        : gui::Application(argc, argv)
    {
    }
};
}

int main(int argc, const char* argv[])
{
    GuiTestApplication app(argc, argv);
    app.init("BA");
    return app.run();
}
