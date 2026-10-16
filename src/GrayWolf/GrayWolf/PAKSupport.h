#pragma once

#include <string>
#include <cstdint>
#include <memory>

class PAKSupport
{
public:

    // Extracts a resource from an image package (Ghosts)
    static std::unique_ptr<uint8_t[]> GhostsExtractImagePackage(const std::string& PackageName, uint64_t AssetOffset, uint64_t AssetSize, uint32_t& ResultSize);
    // Extracts a resource from an image package (AW / MWR)
    static std::unique_ptr<uint8_t[]> AWExtractImagePackage(const std::string& PackageName, uint64_t AssetOffset, uint64_t AssetSize, uint32_t& ResultSize);
    // Extracts a resource from an image package (IW)
    static std::unique_ptr<uint8_t[]> IWExtractImagePackage(const std::string& PackageName, uint64_t AssetOffset, uint64_t AssetSize, uint32_t& ResultSize);
};