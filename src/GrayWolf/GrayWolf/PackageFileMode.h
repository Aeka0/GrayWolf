#pragma once

// The `file` subcommand's use case: one package file (.xpak, .ipak, .iwd, .sabs/.sabl) loaded
// into CoDAssets' pool, read through the live readers of the game it belongs to (ICoDGame.h,
// PackageMark), and its images or sounds written under exported_files/<game folder>/.

#include <cstdint>
#include <string>
#include <vector>

enum class LoadGameFileResult
{
    Success,
    InvalidFile,
    UnknownError
};

namespace PackageFileMode
{
    // Releases whatever was loaded, then loads the file's assets into CoDAssets::GameAssets
    LoadGameFileResult Load(const std::string& FilePath);
    // Writes the pool's assets (Selection: those indices, nullptr: all) on Threads threads (0: the
    // cores minus 2); returns how many it went through, placeholders included
    uint32_t Export(const std::vector<uint32_t>* Selection, uint32_t Threads);
    // Where the last export wrote: the asset's folder, or its kind's for several
    const std::string& LatestExportPath();
}
