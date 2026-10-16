#pragma once

#include <string>

class Hashing
{
public:

    static std::string HashCRC32String(const std::string& Value, uint32_t Initial = 0);

    static uint64_t HashXXHashString(const std::string& Value);
};