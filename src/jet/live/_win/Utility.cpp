#include "jet/live/Utility.hpp"
#include <fstream>
#include <iomanip>
#include <sstream>
#include <iterator>

namespace jet
{
    std::vector<MemoryRegion> getMemoryRegions()
    {
        std::vector<MemoryRegion> res;
        return res;
    }

    std::string relToString(uint32_t /*relocType*/)
    {
        return "UNKNOWN";
    }
}
