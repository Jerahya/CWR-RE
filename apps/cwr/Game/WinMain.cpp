#ifdef _WIN32

#include <windows.h>
#include <cstring>
#include "GameApplication.hpp"
#include <Poseidon/Core/ProgressSystem.hpp> // Needed for complete type in Application
#include <Poseidon/Foundation/Common/ConsoleUtils.hpp>
#include <Poseidon/Foundation/Platform/CppRuntimeWarning.hpp>
#include <Poseidon/Foundation/Platform/CrashHandler.hpp>

int PASCAL WinMain(HINSTANCE hInst, HINSTANCE hPrev, LPSTR szCmdLine, int sw)
{
    Poseidon::Foundation::WarnIfCppRuntimeIsOlder();
    Poseidon::Foundation::InstallCrashHandler(nullptr);
    if (!strstr(szCmdLine, "--check"))
        Poseidon::Foundation::attachParentConsole();
    GameApplication app;
    return app.Run(hInst, szCmdLine, sw);
}

#else // Linux

#include "GameApplication.hpp"
#include <Poseidon/Foundation/Platform/CrashHandler.hpp>
#ifdef __SWITCH__
#include <Poseidon/Foundation/Platform/PlatformSwitch.hpp>
#endif

int main(int argc, char* argv[])
{
#ifdef __SWITCH__
    Poseidon::Foundation::SwitchStartup(argc, argv);
#endif
    Poseidon::Foundation::InstallCrashHandler(nullptr);
    int result = 0;
    {
        GameApplication app;
        result = app.Run(argc, argv);
    }
#ifdef __SWITCH__
    Poseidon::Foundation::SwitchShutdown();
#endif
    return result;
}

#endif
