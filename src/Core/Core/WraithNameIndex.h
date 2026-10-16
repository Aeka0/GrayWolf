#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>

class WraithNameIndex
{
public:
    WraithNameIndex();
    WraithNameIndex(const std::string& FilePath);

    std::unordered_map<uint64_t, std::string> NameDatabase;

    void SaveIndex(const std::string& FilePath);
    // KeyMask defaults to the 60-bit XAssetName hash space. BO4's sound-hash ids use 63 bits
    // (bits 60-62 carry data): pass 0x7FFFFFFFFFFFFFFF for that index.
    void LoadIndex(const std::string& LoadIndex, uint64_t KeyMask = 0xFFFFFFFFFFFFFFFull);
};