#include <windows.h>

#include <cnt/ListSL.h>
#include <sc/IPlugin.h>

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

namespace
{
using GetPluginInterface = sc::IPlugin* (*)();

class TestLog
{
    std::ofstream _file;

public:
    TestLog()
        : _file("SREES_2026_Bajramovic_TPFPC_dTwinPluginLoaderTest.log", std::ios::binary)
    {
    }

    template <typename T>
    TestLog& operator<<(const T& value)
    {
        std::cout << value;
        if (_file)
            _file << value;
        return *this;
    }
};

std::wstring toWideString(const char* text)
{
    const int size = MultiByteToWideChar(CP_UTF8, 0, text, -1, nullptr, 0);
    if (size <= 0)
        return {};

    std::wstring result(size - 1, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, text, -1, result.data(), size);
    return result;
}

std::string getLastLibraryError()
{
    const DWORD errorCode = GetLastError();
    LPSTR message = nullptr;
    const DWORD size = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                                      nullptr,
                                      errorCode,
                                      MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                                      reinterpret_cast<LPSTR>(&message),
                                      0,
                                      nullptr);

    std::ostringstream out;
    out << "Windows error " << errorCode;
    if (size != 0 && message != nullptr)
        out << ": " << message;

    if (message != nullptr)
        LocalFree(message);

    return out.str();
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

bool fileExists(const std::string& fileName)
{
    std::ifstream input(fileName, std::ios::binary);
    return input.good();
}

std::streamoff fileSize(const std::string& fileName)
{
    std::ifstream input(fileName, std::ios::binary | std::ios::ate);
    if (!input)
        return 0;

    return input.tellg();
}
}

int main()
{
    TestLog log;
    const char* pluginPath = TPFPC_DTWIN_PLUGIN_PATH;
    log << "Plugin DLL: " << pluginPath << '\n';
    log << "Dependency DLL folder: " << TPFPC_OTHER_BIN_PATH << '\n';
    log << "GTK DLL folder: " << TPFPC_OTHER_BIN_GTK_PATH << '\n';

    SetDefaultDllDirectories(LOAD_LIBRARY_SEARCH_DEFAULT_DIRS | LOAD_LIBRARY_SEARCH_USER_DIRS | LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR);
    const auto otherBinPath = toWideString(TPFPC_OTHER_BIN_PATH);
    if (!otherBinPath.empty())
        AddDllDirectory(otherBinPath.c_str());
    const auto gtkPath = toWideString(TPFPC_OTHER_BIN_GTK_PATH);
    if (!gtkPath.empty())
        AddDllDirectory(gtkPath.c_str());

    HMODULE library = LoadLibraryExA(pluginPath, nullptr, LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR | LOAD_LIBRARY_SEARCH_DEFAULT_DIRS | LOAD_LIBRARY_SEARCH_USER_DIRS);
    if (library == nullptr)
    {
        log << "LoadLibrary: FAILED\n";
        log << getLastLibraryError() << '\n';
        return 1;
    }
    log << "LoadLibrary: OK\n";

    auto getPluginInterface = reinterpret_cast<GetPluginInterface>(GetProcAddress(library, "getPluginInterface"));
    if (getPluginInterface == nullptr)
    {
        log << "getPluginInterface: FAILED\n";
        FreeLibrary(library);
        return 2;
    }
    log << "getPluginInterface: OK\n";

    sc::IPlugin* plugin = getPluginInterface();
    if (plugin == nullptr)
    {
        log << "sc::IPlugin pointer: FAILED\n";
        FreeLibrary(library);
        return 3;
    }
    log << "sc::IPlugin pointer: OK\n";

    log << "getMenuName: " << plugin->getMenuName().c_str() << '\n';
    log << "getModelType: " << modelTypeName(plugin->getModelType()) << '\n';
    log << "getOutFileName before show: " << plugin->getOutFileName().c_str() << '\n';
    log << "getMaxSupportedArchiveParts: " << plugin->getMaxSupportedArchiveParts() << '\n';

    sc::IPlugin::MemoryArchiveContainer archives;
    for (size_t i = 0; i < size_t(sc::IPlugin::ArchType::NA); ++i)
        archives[i] = nullptr;

    log << "Memory archives: skipped for local file-only workflow\n";

    bool cleanerCalled = false;
    auto cleaner = [&cleanerCalled]() {
        cleanerCalled = true;
    };

    bool callbackCalled = false;
    std::string generatedFileName;
    auto onComplete = [&callbackCalled, &generatedFileName](sc::IPlugin* completedPlugin) {
        callbackCalled = true;
        if (completedPlugin != nullptr)
            generatedFileName = completedPlugin->getOutFileName().c_str();
    };

    plugin->show(nullptr, archives, 1, cleaner, onComplete);

    log << "show workflow: OK\n";
    log << "onComplete callback: " << (callbackCalled ? "OK" : "NOT CALLED") << '\n';
    log << "cleaner callback: " << (cleanerCalled ? "OK" : "NOT CALLED") << '\n';

    const std::string outFileName = generatedFileName.empty()
        ? std::string(plugin->getOutFileName().c_str())
        : generatedFileName;

    log << "getOutFileName after show: " << outFileName << '\n';

    if (outFileName.empty())
    {
        log << "Generated dmodl path: FAILED\n";
        FreeLibrary(library);
        return 6;
    }

    if (!fileExists(outFileName))
    {
        log << "Generated dmodl file exists: FAILED\n";
        FreeLibrary(library);
        return 7;
    }

    log << "Generated dmodl file exists: OK\n";
    log << "Generated dmodl size: " << fileSize(outFileName) << " bytes\n";

    FreeLibrary(library);
    return callbackCalled && cleanerCalled ? 0 : 8;
}
