#pragma once

#include <chrono>
#include <cstdint>
#include <string>
#include <vector>

#include "WraithAsset.h" // WraithAssetType

struct CliOptions;

// Writes the report to the parent console when there is one, and to --report when asked.
void CliReport(const CliOptions& Options, const std::string& Report);

// Applies the per-run export CLI flags by overriding the persisted settings the exporters read.
void ApplyExportOverrides(const CliOptions& Options);

// A comma-separated list of case-insensitive substrings, lowered once so matching is cheap.
std::vector<std::string> SplitNameFilter(const std::string& Filter);

bool MatchesNameFilter(const std::string& Name, const std::vector<std::string>& Needles);

// Maps a --type value onto an asset type, so a run can measure one kind at a time.
bool ParseAssetType(const std::string& Name, WraithAssetType& TypeOut);

// How long loading took, for the CLI's reports.
using CliClock = std::chrono::steady_clock;
int64_t CliElapsedMs(CliClock::time_point A, CliClock::time_point B);
