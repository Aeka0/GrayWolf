#include "stdafx.h"

#include <CLI/CLI.hpp>

#include "CliCommands.h"
#include "CliOptions.h"
#include "GameCommand.h"

namespace
{
    void AddOptions(CLI::App& Sub, CliOptions& Options)
    {
        Sub.add_option("--profile", Options.ZoneWalkProfile, "zonekit profile (default bo4)");
    }

    const GameCommand Command{
        "bo4-zone-diag",
        "BO4 zones walked with the zonekit tables, for diagnosis: per-zone summary, and with --out --type strings|matimages|refs|imagekeys (assets: disk-export --game bo4)",
        "A zone (.ff/.bin), a folder of zones, a .txt list, or casc:<pattern> (from --install, e.g. casc:*.ff)",
        RunBo4ZoneDiag,
        AddOptions,
    };
    const GameCommandRegistration Registered(Command);
}
