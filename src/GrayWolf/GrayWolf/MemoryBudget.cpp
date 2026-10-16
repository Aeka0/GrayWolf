#include "stdafx.h"

#include "MemoryBudget.h"

#include <algorithm>
#include <cstdio>

namespace MemoryBudget
{
uint64_t Available()
{
    MEMORYSTATUSEX S{};
    S.dwLength = sizeof(S);
    if (!GlobalMemoryStatusEx(&S)) return 0;
    return std::min<uint64_t>(S.ullAvailPhys, S.ullAvailPageFile);
}

uint64_t Reserve()
{
    MEMORYSTATUSEX S{};
    S.dwLength = sizeof(S);
    const uint64_t Third = GlobalMemoryStatusEx(&S) ? S.ullTotalPhys / 3 : 0;
    return std::max<uint64_t>(Third, 4ull << 30);
}

uint64_t For(double Fraction, uint64_t Largest)
{
    const uint64_t Free = Available(), Keep = Reserve();
    const uint64_t Room = Free > Keep ? Free - Keep : 0;
    return std::max<uint64_t>(static_cast<uint64_t>(Room * Fraction), Largest);
}

std::string Gb(uint64_t Bytes)
{
    char B[32];
    snprintf(B, sizeof(B), "%.2f GB", Bytes / double(1ull << 30));
    return B;
}

std::string CheckFits(uint64_t Largest, const std::string& What)
{
    const uint64_t Free = Available();
    if (Largest <= Free) return {};
    return What + " needs about " + Gb(Largest) + ", " + Gb(Free) + " is free (" + Gb(Largest - Free) +
           " short): close other programs or leave that zone out";
}
}
