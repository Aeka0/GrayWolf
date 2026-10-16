#pragma once

// How much memory a batch of zones may hold at once, from what the system has free now: the
// smaller of free RAM and the room left before the commit limit (RAM + page file), which is what
// runs out first when several large zones are loaded together.

#include <cstdint>
#include <string>

namespace MemoryBudget
{
    uint64_t Available();
    // What a batch leaves free for the rest of the machine: a third of the RAM, at least 4 GB.
    uint64_t Reserve();
    // Fraction of what Available() has above Reserve(), never below the largest single item
    // (one always proceeds).
    uint64_t For(double Fraction, uint64_t Largest);
    // Empty when the largest item fits; otherwise why not, in words, with the shortfall.
    std::string CheckFits(uint64_t Largest, const std::string& What);
    std::string Gb(uint64_t Bytes);
}
