// Copyright (C) 2023 Goncalo Soares Roque 

#include "Application.h"

namespace GRAPE {
    int main(int ArgCount, char** ArgValues) {
        Application grape(CommandLineArgs(ArgCount, ArgValues));
        grape.run();
        return 0;
    }
}

#ifdef GRAPE_PLATFORM_WINDOWS
#ifdef GRAPE_DISTRIBUTION

#include "windows.h"

static bool attachOutputToConsole() {
    if (AttachConsole(ATTACH_PARENT_PROCESS))
    {
        const HANDLE consoleHandleOut = GetStdHandle(STD_OUTPUT_HANDLE);
        if (consoleHandleOut != INVALID_HANDLE_VALUE)
        {
            FILE* unused;
            [[maybe_unused]] const errno_t errOpen = freopen_s(&unused, "CONOUT$", "w", stdout);
            [[maybe_unused]] const int errBuffer = setvbuf(stdout, nullptr, _IONBF, 0);
        }
        else { return FALSE; }

        return TRUE;
    }

    return FALSE;
}

static void sendEnterKey() {
    INPUT ip;

    ip.type = INPUT_KEYBOARD;
    ip.ki.wScan = 0;
    ip.ki.time = 0;
    ip.ki.dwExtraInfo = 0;

    ip.ki.wVk = 0x0D;
    ip.ki.dwFlags = 0;
    SendInput(1, &ip, sizeof(INPUT));

    ip.ki.dwFlags = KEYEVENTF_KEYUP;
    SendInput(1, &ip, sizeof(INPUT));
}

int WINAPI WinMain(HINSTANCE, HINSTANCE, PSTR, INT) {
    const bool runAsConsole = attachOutputToConsole();
    const int ret = GRAPE::main(__argc, __argv);

    if (runAsConsole && GetConsoleWindow() == GetForegroundWindow())
        sendEnterKey();

    return ret;
}

#else

int main(int ArgCount, char** ArgValues) {
    return GRAPE::main(ArgCount, ArgValues);
}

#endif
#endif
