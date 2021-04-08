#pragma once

#include "CoffProgramInfoLoader.hpp"
#include "jet/live/LiveContext.hpp"
#include "scoped_handle.hpp"
#include "jet/live/IProgramInfoLoader.hpp"

#include <Windows.h>
#include <psapi.h> // EnumProcessModules
#include <processthreadsapi.h> // GetCurrentProcessId

#include <type_traits>
#include <iostream>

namespace jet
{
std::vector<std::string>
CoffProgramInfoLoader::getAllLoadedProgramsPaths(const LiveContext*) const
{
    std::vector<std::string> paths;
    scoped_handle hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ,
        FALSE, GetCurrentProcessId());
    if (NULL == hProcess)
        return paths;

    HMODULE hMods[1024];
    DWORD cbNeeded;
    if (EnumProcessModules(hProcess, hMods, sizeof(hMods), &cbNeeded)) {
        auto size = size_t(cbNeeded / sizeof(HMODULE));
        paths.reserve(size);
        for (size_t i = 0; i < size; ++i) {
            char name[MAX_PATH];
            if (GetModuleFileNameEx(hProcess, hMods[i], name, sizeof(name)))
                paths.emplace_back(name);
        }
    }

    return paths;
}

Symbols
CoffProgramInfoLoader::getProgramSymbols(const LiveContext* context,
                                         const std::string& filepath) const
{
    Symbols symbols;

    // copy-pasted from Linux code, may be invalid:
    std::string realFilePath = filepath.empty() ? context->thisExecutablePath : filepath;
    std::cerr << realFilePath << std::endl;

    return symbols;
}

std::vector<Relocation>
CoffProgramInfoLoader::getLinkTimeRelocations(const LiveContext* context,
                                              const std::vector<std::string>& objFilePaths)
{
    return {};
}

std::vector<std::string>
CoffProgramInfoLoader::getUndefinedSymbolNames(const LiveContext* context,
                                               const std::string filepath)
{
    return {};
}

std::vector<std::string>
CoffProgramInfoLoader::getExportedSymbolNames(const LiveContext* context,
                                              const std::string filepath)
{
    return {};
}

} // namespace jet
