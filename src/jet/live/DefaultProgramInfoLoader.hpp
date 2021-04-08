
#pragma once

#ifdef __linux__
#include "jet/live/_linux/ElfProgramInfoLoader.hpp"
namespace jet
{
    using DefaultProgramInfoLoader = ElfProgramInfoLoader;
}

#elif __APPLE__
#include "jet/live/_macos/MachoProgramInfoLoader.hpp"
namespace jet
{
    using DefaultProgramInfoLoader = MachoProgramInfoLoader;
}

#elif defined(__WIN32) || defined(WIN32) || defined(__WIN32__)
#include "jet/live/_win/CoffProgramInfoLoader.hpp"
namespace jet
{
    using DefaultProgramInfoLoader = CoffProgramInfoLoader;
}
#else
#error "Platform is not supported"
#endif
