#pragma once

#include <string>
#include <vector>
#include <cstdint>

// .xpak entries are keyed by an opaque hash with no known name->hash algorithm, so this dumps
// every sufficiently-large entry from the given .xpak file(s) to find one by content, not by name.
// Returns how many entries were written to <OutDir>/<hex CacheID>.bin; fills Report with a summary.
size_t Bo3DumpXpakEntries(const std::vector<std::string>& XpakFiles, const std::string& OutDir,
                           uint64_t MinSizeBytes, std::string& Report);
