#pragma once

#include <cstdint>
#include <memory>

struct XImageDDS;

// A class that handles converting IWI files to DDS files in memory
class CoDIWITranslator
{
public:
    static std::unique_ptr<XImageDDS> TranslateIWI(const std::unique_ptr<uint8_t[]>& IWIBuffer, uint32_t IWIBufferSize);
};