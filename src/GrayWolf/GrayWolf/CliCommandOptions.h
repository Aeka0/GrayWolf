#pragma once

#include <string>

// Per-command options, one struct each.
// CliOptions is one struct shared by every subcommand: new commands declare their own struct here,
// built from CliOptions (CLI11 binds to its members) by a From() factory.

struct CliOptions;

// coverage <ff>: what a zone holds and which mechanism can extract each type.
struct CoverageCommandOptions
{
    std::string FastFilePath;
    std::string ReportPath;

    static CoverageCommandOptions From(const CliOptions& Options);
};
