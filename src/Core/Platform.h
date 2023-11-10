// Copyright (C) 2023 Goncalo Soares Roque 

#pragma once

#ifdef GRAPE_PLATFORM_WINDOWS
#include <shellapi.h> // ShellExecuteA
#include <Windows.h> // GetModuleFileNameA
#else
#include <unistd.h> // readlink
#endif

#include <filesystem>

namespace GRAPE {
    /**
    * @brief Calls the OS library to open or start a path.
    * @param Path The path to the file or executable to be opened.
    */
    inline void platformOpen(const char* Path) {
#ifdef GRAPE_PLATFORM_WINDOWS
        ShellExecuteA(nullptr, "open", Path, nullptr, nullptr, SW_SHOWDEFAULT);
#else
#if defined(GRAPE_PLATFORM_MACOS)
        const char* openExec = "open";
#elif defined(GRAPE_PLATFORM_LINUX)
        const char* openExec = "xdg-open";
#endif
        std::system(std::format("{} \"{}\"", openExec, Path));
#endif
    }

    inline std::filesystem::path getExecutableDir() {
#ifdef GRAPE_PLATFORM_WINDOWS
        char rawPathName[MAX_PATH];
        GetModuleFileNameA(nullptr, rawPathName, MAX_PATH);
        return std::filesystem::path(rawPathName).parent_path();
#else
        char result[PATH_MAX];
        ssize_t count = readlink("/proc/self/exe", result, PATH_MAX);
        return std::string(result, (count > 0) ? count : 0);
#endif
    }

    inline std::filesystem::path getResolvedPath(std::string_view GrapePath) {
        return weakly_canonical(getExecutableDir() /= GrapePath);
    }
}
