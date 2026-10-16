// open-map <map or folder> [--reimport] [--background]: a map export opened in Blender
// (BlenderLauncher.h), the command line's face of the GUI's "Open in Blender".

#include "stdafx.h"

#include "BlenderLauncher.h"
#include "CliCommands.h"
#include "CliSupport.h"

#include <filesystem>

int RunOpenMap(const CliOptions& Options)
{
    namespace fs = std::filesystem;
    std::string Error;
    Blender::Map M;
    std::error_code Ec;
    bool Found = false;
    if (fs::is_directory(fs::path(Options.OpenMap), Ec))   // argv: the ANSI code page
    {
        const std::vector<Blender::Map> Maps = Blender::MapsIn(Options.OpenMap);
        if (Maps.size() == 1) { M = Maps[0]; Found = true; }
        else if (Maps.empty()) Error = "no map in " + Options.OpenMap + " (no *_import.py there or in maps\\)";
        else
        {
            Error = std::to_string(Maps.size()) + " maps in " + Options.OpenMap + ", pass one's .glb:";
            for (const auto& Each : Maps) Error += "\n  " + Each.Glb;
        }
    }
    else Found = Blender::MapOf(Options.OpenMap, M, Error);

    std::string Did;
    const bool Ok = Found && Blender::Open(M, { Options.OpenMapReimport, Options.OpenMapBackground }, Did, Error);
    const Blender::Exe B = Blender::Find();
    CliReport(Options, Ok ? "result=ok\ndid=" + Did + "\nblender=" + B.Path + " (" + B.From + ")\n"
                          : "result=error\nreason=" + Error + "\n");
    // dropped on the exe from Explorer: no console to read the error in
    if (!Ok && GetConsoleWindow() == nullptr && Options.Report.empty())
        MessageBoxA(nullptr, Error.c_str(), "GrayWolf: open-map", MB_OK | MB_ICONEXCLAMATION);
    return Ok ? 0 : 1;
}
