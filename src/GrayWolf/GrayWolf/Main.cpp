#include "stdafx.h"

#include <stdio.h>
#include <cmath>
#include "AppLog.h"
#include <CLI/CLI.hpp>

// Must be included before additional includes
#include "WraithX.h"
#include "Updater.h"


#include "resource.h"

#include "SettingsManager.h"
#include "CoDAssets.h"
#include "GameCommand.h"
#include "DiskGameCli.h"
#include "ImGuiShell.h"

#include <vector>
#include "FileSystems.h"

#include <string>

#include "CliOptions.h"
#include "CliCommands.h"
#include "CliSupport.h"

#include <dbghelp.h>
#include <filesystem>
#pragma comment(lib, "dbghelp.lib")

// Allow modern GUI controls
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

// Cleanup old patch files before launch...
static void CleanupFilesystem()
{
    std::error_code Ec;
    auto CurrentPath = FileSystems::GetApplicationPath();

    std::filesystem::remove((std::filesystem::path(CurrentPath) / "exhalelib.dll").string(), Ec);
    std::filesystem::remove((std::filesystem::path(CurrentPath) / "latest_log.txt").string(), Ec);
    std::filesystem::remove((std::filesystem::path(CurrentPath) / "settings.dat").string(), Ec);
    std::filesystem::remove((std::filesystem::path(CurrentPath) / "Wraith.exe").string(), Ec);

    // Old cache
    std::filesystem::remove_all((std::filesystem::path(CurrentPath) / "data").string(), Ec);
    // Old update cache
    std::filesystem::remove_all((std::filesystem::path(CurrentPath) / "Temp").string(), Ec);
}

// Set once a CLI subcommand runs: no one is there to click a dialog, and a modal box on a crashed
// worker thread froze whole exports instead of ending them.
static volatile bool g_CliMode = false;

LONG WINAPI MyUnhandledExceptionFilter(EXCEPTION_POINTERS* ExceptionInfo)
{
    if (!g_CliMode)
    MessageBoxA(NULL, "GrayWolf has encountered a fatal error and must close.\n\nA dump file will be written to where GrayWolf's exe is located, please provide this and as much information as you can when reporting this crash.", "GrayWolf", MB_OK | MB_ICONERROR);
    
    HANDLE hFile = CreateFile(
        L"crash_dump.dmp",
        GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL,
        CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );
    MINIDUMP_EXCEPTION_INFORMATION mei{};
    mei.ThreadId = GetCurrentThreadId();
    mei.ClientPointers = TRUE;
    mei.ExceptionPointers = ExceptionInfo;
    MiniDumpWriteDump(
        GetCurrentProcess(),
        GetCurrentProcessId(),
        hFile,
        MiniDumpWithFullMemory,
        &mei,
        NULL,
        NULL);
    if (hFile != INVALID_HANDLE_VALUE)
    {
        CloseHandle(hFile);
    }

    // headless: the dump is written, end with a failing code rather than waiting on anything
    if (g_CliMode) TerminateProcess(GetCurrentProcess(), 0xC0DE);

    return EXCEPTION_EXECUTE_HANDLER;
}


// Loads a package file and optionally exports it, without ever creating a window.
// Everything below runs the exact same asset pipeline the UI drives, the only
// difference being that no progress callback is installed.
static int RunCli(const CliOptions& Options)
{
    if (Options.SelfTest) return RunSelfTest(Options);
    if (Options.ListInstalls) return RunListInstalls(Options);


    if (!Options.Identify.empty()) return RunIdentify(Options);

    if (!Options.Bo3Images.empty()) return RunBo3Images(Options);

    if (!Options.Bo3DumpXpakEntries.empty()) return RunBo3DumpXpakEntries(Options);

    if (Options.Salsa20SelfTest) return RunSalsa20SelfTest(Options);

    if (!Options.Bo3DecryptMovie.empty()) return RunBo3DecryptMovie(Options);

    if (!Options.DecompressFF.empty()) return RunDecompressFF(Options);

    if (!Options.VerifyOpus.empty()) return RunVerifyOpus(Options);

    if (!Options.IndexAssets.empty() || !Options.ModelsDir.empty()) return RunIndexAssets(Options);

    if (!Options.QueryAssets.empty()) return RunQueryAssets(Options);

    if (!Options.AudioProbe.empty()) return RunAudioProbe(Options);

    if (!Options.CascList.empty()) return RunCascList(Options);
    if (!Options.XpakIndex.empty()) return RunXpakIndex(Options);
    if (!Options.XsubIndex.empty()) return RunXsubIndex(Options);
    if (!Options.XsubExtract.empty()) return RunXsubExtract(Options);

    if (!Options.CascExtractDir.empty()) return RunCascExtractDir(Options);

    if (!Options.ZoneDecompressIn.empty()) return RunZoneDecompress(Options);

    if (!Options.Bo4ExportLocalizeDiskIn.empty()) return RunBo4ExportLocalizeDisk(Options);

    if (!Options.Bo4ExportRawStringsDiskIn.empty()) return RunBo4ExportRawStringsDisk(Options);

    if (!Options.ZoneWalkList.empty()) return RunZoneWalk(Options);

    for (const GameCommand* Command : GameCommands())
    {
        auto It = Options.DiskExportIn.find(Command->Name);
        if (It != Options.DiskExportIn.end() && !It->second.empty()) return Command->Run(Options, It->second);
    }
    // disk-export <input> [--game id], or a game's own alias: the one driver (DiskGame.h)
    for (const auto& [Command, Input] : Options.DiskExportIn)
    {
        if (Input.empty()) continue;
        const DiskGame::IDiskGame* Game = nullptr;
        if (Command != "disk-export") Game = DiskGame::Find(Command);
        else if (!Options.DiskGameId.empty()) Game = DiskGame::Find(Options.DiskGameId);
        else Game = DiskGame::Detect(Input);
        if (Game == nullptr)
        {
            CliReport(Options, "result=error\nreason=no disk reader for " + (Options.DiskGameId.empty() ? Input : Options.DiskGameId) + "\n");
            return 1;
        }
        return DiskGame::Run(*Game, Options, Input);
    }

    if (!Options.DecodeIwd.empty()) return RunDecodeIwd(Options);

    if (!Options.GltfSelfTest.empty()) return RunGltfSelfTest(Options);

    if (!Options.ExportMapGlb.empty()) return RunExportMapGlb(Options);

    if (!Options.OpenMap.empty()) return RunOpenMap(Options);

    if (Options.CerberusSelfTest) return RunCerberusSelfTest(Options);

    if (Options.SeAnimSelfTest) return RunSeAnimSelfTest(Options);
    if (Options.CastSelfTest) return RunCastSelfTest(Options);

    if (!Options.MakeTestSeAnim.empty()) return RunMakeTestSeAnim(Options);

    if (!Options.RenderAnimModel.empty()) return RunRenderAnimModel(Options);

    if (!Options.Coverage.empty()) return RunCoverage(Options);
    if (!Options.OatExtract.empty()) return RunOatExtract(Options);

    if (!Options.XAnimBinDump.empty()) return RunXAnimBinDump(Options);

    if (!Options.XAnimBinSelfTest.empty()) return RunXAnimBinSelfTest(Options);

    if (!Options.XAnimBinToSeAnimIn.empty()) return RunXAnimBinToSeAnimIn(Options);

    if (!Options.XAnimBinExportDirIn.empty()) return RunXAnimBinExportDirIn(Options);

    if (!Options.OatFx.empty()) return RunOatFx(Options);

    if (!Options.OatFxExport.empty()) return RunOatFxExport(Options);

    if (!Options.OatXModelMtl.empty()) return RunOatXModelMtl(Options);

    if (!Options.OatWeapon.empty()) return RunOatWeapon(Options);

    if (!Options.OatRawfiles.empty()) return RunOatRawfiles(Options);

    if (!Options.GenDisc.empty()) return RunGenDisc(Options);

    if (!Options.ActsExtract.empty()) return RunActsExtract(Options);

    if (!Options.DumpModel.empty()) return RunDumpModel(Options);

    if (!Options.DumpGlb.empty()) return RunDumpGlb(Options);

    if (!Options.RenderGlbPng.empty()) return RunRenderGlbPng(Options);

    if (!Options.Ui.empty()) return RunUiMode(Options);

    if (!Options.PreviewWindow.empty()) return RunPreviewWindow(Options);

    if (!Options.RenderModel.empty()) return RunRenderModelCli(Options);

    if (!Options.SabProbe.empty()) return RunSabProbe(Options);

    if (Options.UpdateNameDb) return RunUpdateNameDb(Options);

    if (Options.Update) return RunUpdate(Options);

    if (!Options.ApplyUpdate.empty()) return RunApplyUpdate(Options);

    if (!Options.ImportHashCsv.empty()) return RunImportHashCsv(Options);


    if (!Options.Bo4DecryptHex.empty() || !Options.Bo4DecryptDiskHex.empty()) return RunBo4Decrypt(Options);

    if (Options.File.empty())
    {
        CliReport(Options, "result=error\nreason=missing subcommand\n");
        return 2;
    }

    if (!WraithX::InitializeAPI())
    {
        CliReport(Options, "result=error\nreason=api init failed\n");
        return 1;
    }

    return RunLoadFileMode(Options);
}

// CLI11 front end: a subcommand alone means "run headless". Shared options
// (--install/--out/--cache/--threads/--limit/--type/--report/--name-filter/format overrides/
// --acts-*/--oat-*) live on the parent App with fallthrough(). Fills CliOptions for RunCli.
static int RunHeadlessCli(int argc, char** argv, bool& CliInvoked)
{
    CliOptions Options;
    CliInvoked = false;


    // Let CLI11-printed help/error text reach an invoking console (a no-op without one).
    if (AttachConsole(ATTACH_PARENT_PROCESS))
    {
        FILE* OutF = nullptr;
        FILE* ErrF = nullptr;
        freopen_s(&OutF, "CONOUT$", "w", stdout);
        freopen_s(&ErrF, "CONOUT$", "w", stderr);
    }

    // The bare "verifiedhashes" token (no leading dashes) is scanned for directly in main()
    // before this function ever runs, and is not part of CLI11's grammar - drop it here so it
    // does not turn into an "unrecognized argument" parse error.
    std::vector<std::string> ArgStorage;
    for (int i = 1; i < argc; i++)
    {
        if (std::string(argv[i]) != "verifiedhashes") ArgStorage.push_back(argv[i]);
    }

    std::vector<char*> ArgvPtrs;
    ArgvPtrs.push_back(const_cast<char*>("GrayWolf.exe"));
    for (auto& A : ArgStorage) ArgvPtrs.push_back(const_cast<char*>(A.c_str()));

    CLI::App App{"GrayWolf - Call of Duty asset extractor (headless mode)", "GrayWolf"};
    App.fallthrough();
    App.require_subcommand(0, 1);

    // Shared options, usable by any subcommand
    App.add_option("--install", Options.Install, "Game install / asset source directory")
        ->check(CLI::ExistingDirectory)->group("Shared options");
    App.add_option("--out", Options.Out, "Output file or directory (meaning depends on the command)")->group("Shared options");
    App.add_option("--cache", Options.Cache, "Persistent decompression cache directory")->group("Shared options");
    App.add_option("--threads", Options.Threads, "Worker thread count")->group("Shared options");
    App.add_option("--limit", Options.Limit, "Cap the number of assets processed")->group("Shared options");
    App.add_option("--type", Options.Type, "Asset type filter (model, image, sound, ...)")->group("Shared options");
    App.add_option("--report", Options.Report, "Write the result report to this file")->group("Shared options");
    App.add_flag("--export-all", Options.ExportAll, "Export every asset matching --type/--limit/--name-filter")->group("Shared options");
    App.add_option("--name-filter", Options.NameFilter,
        "Comma separated substrings, matches any (case-insensitive)")->group("Shared options");
    App.add_option("--model-format", Options.ModelFormat,
        "cast|semodel|obj|ma|xna|xmodel_export|xmodel_bin|smd|gltf|glb|none, or a comma list (cast,semodel)")->group("Shared options");
    App.add_option("--anim-format", Options.AnimFormat,
        "cast|seanim|xanim_export|none, or a comma list (cast,seanim)")->group("Shared options");
    App.add_option("--image-format", Options.ImageFormat, "png|tga|dds")->group("Shared options");
    App.add_option("--ui-res", Options.UiRes,
                   "4k|1080|all: BO4 UI texture zones to read, 4K (default) or 1080p (the same images at half size)")
        ->group("Shared options");
    App.add_option("--map-textures", Options.MapTextures,
                   "full|2k|1k: map textures larger than that copied at that size with mips (maps/textures/), "
                   "which the import script reads (full: none)")
        ->group("Shared options");
    App.add_option("--map-png", Options.MapPng,
                   "on|off: also write PNG copies of map colour maps (maps/textures/) for glTF tools other than "
                   "Blender (off: the .glb names the DDS files, Blender's import script reads them)")
        ->group("Shared options");
    App.add_option("--map-cast", Options.MapCast,
                   "on|off: also write the map as a Cast scene (<map>.cast placing maps/models/<model>.cast), "
                   "for Maya and the Cast plugins (off: the .glb only)")
        ->group("Shared options");
    App.add_option("--map-radiant", Options.MapRadiant,
                   "on|off: also write the map as a Radiant source (<map>.map: the clip map's brushes, the entities, "
                   "misc_model placements; <map>_radiant/: the world and models as XMODEL_EXPORT, a GDT), for the mod tools")
        ->group("Shared options");
    App.add_option("--material-gdt", Options.MaterialGdt,
                   "on|off: also write Black Ops III Mod Tools material and image GDT entries for the exported materials "
                   "(models: source_data/graywolf_<out folder>.gdt, their XMODEL_EXPORT files naming them; maps: in the "
                   "Radiant GDT with --map-radiant on)")
        ->group("Shared options");
    App.add_option("--sound-aliases", Options.SoundAliases,
                   "on|off: the sound kind also writes each zone's sound aliases as a Black Ops III mod tools alias CSV "
                   "(sounds/<zone>.csv, FileSpec relative to sounds/), to port sounds with their behaviour (off: audio only)")
        ->group("Shared options");
    App.add_option("--map-collision", Options.MapCollision,
                   "on|off: also write the map's collision as <map>_collision.json (the clip map's brushes, brush "
                   "models, triggers and triangles), for navigation and top-down tools")
        ->group("Shared options");
    App.add_option("--map-content", Options.MapContent,
                   "all|terrain: the whole map, or its terrain alone (meshes and their materials; no models, "
                   "lights or entities)")
        ->group("Shared options");
    App.add_flag("--decompile{on}", Options.Decompile,
                 "--type script: also decompile the compiled scripts (acts or gsc-tool, by the game's VM); "
                 "a script the tool does not decompile keeps its bytecode file (--decompile=off: bytecode only)")
        ->group("Shared options");
    App.add_option("--language", Options.Language,
                   "english|french|german|italian|spanish|russian|polish: CoD1/CoD2, the localized archives that "
                   "replace the base ones' files (english: none, the base wins)")
        ->group("Shared options");
    App.add_option("--audio-format", Options.AudioFormat,
                   "wav|flac|opus: sounds (wav: as the game keeps them, or decoded; flac: lossless; opus: the game's own "
                   "Opus packets kept as they are, any other sound encoded)")
        ->transform(CLI::IsMember({ "wav", "flac", "opus" }, CLI::ignore_case))
        ->group("Shared options");
    App.add_option("--unit", Options.Unit, "Centimeter|Meter|Inch (model export scale)")->group("Shared options");
    App.add_flag("--lods", Options.Lods, "Export every available model LOD")->group("Shared options");
    App.add_option("--normal-map", Options.NormalMap, "none|opengl (flip normal-map green for Blender/DCC)")->group("Shared options");
    App.add_option("--acts-exe", Options.ActsExe, "Path to acts.exe (else acts.exe on PATH)")->group("Shared options");
    App.add_option("--gsc-tool-exe", Options.GscToolExe, "Path to gsc-tool.exe (else gsc-tool.exe on PATH)")->group("Shared options");
    App.add_option("--game-exe", Options.GameExe,
                   "--decompile: the game's dumped (unpacked) exe acts decrypts the scripts' encrypted strings with, "
                   "for a game that encrypts them (default: the game's dump in acts' deps folder; not found: strings left encrypted)")
        ->group("Shared options");
    App.add_option("--acts-handler", Options.ActsHandler, "Force a game handler for acts (bo4/cw/mwiii/bo6/...)")->group("Shared options");
    App.add_option("--oat-model-format", Options.OatModelFmt, "gltf|glb|obj|xmodel_export|xmodel_bin")->group("Shared options");
    App.add_option("--oat-image-format", Options.OatImageFmt, "dds|iwi")->group("Shared options");
    App.add_flag("--oat-gdt", Options.OatGdt, "Also emit a source_data/<zone>.gdt")->group("Shared options");
    App.add_flag("--with-sounds", Options.WithSounds, "Also extract sounds OAT can't dump (T5/BO1)")->group("Shared options");

    auto* SubSelfTest = App.add_subcommand("self-test", "Run built-in assertions that need no game files or window");

    auto* SubListInstalls = App.add_subcommand("list-installs", "The Call of Duty installs found on this machine, as the Load Game panel lists them");

    auto* SubVerifyOpus = App.add_subcommand("verify-opus", "Decode an Opus file and report duration/rate/peaks");
    SubVerifyOpus->add_option("file", Options.VerifyOpus, "Opus file to decode")->required()->check(CLI::ExistingFile);

    auto* SubDecompressFF = App.add_subcommand("decompress-ff", "Decompress a fast file and report its header/zone info");
    SubDecompressFF->add_option("file", Options.DecompressFF, "Fast file (.ff) to decompress")->required()->check(CLI::ExistingFile);
    SubDecompressFF->add_flag("--strings", Options.Strings, "Also dump the zone's string table");


    auto* SubIdentify = App.add_subcommand("identify", "Identify a game/asset by name or hash");
    SubIdentify->add_option("name", Options.Identify, "Name or hash to identify")->required();

    auto* SubBo3Images = App.add_subcommand("bo3-images", "BO3 zone images -> PNG, pixels sourced from the .xpaks");
    SubBo3Images->add_option("file", Options.Bo3Images, "Fast file (.ff) to read image assets from")->required()->check(CLI::ExistingFile);
    SubBo3Images->add_option("--bo3-xpaks", Options.Bo3Xpaks, "Where the packages are (default: beside the ff)");

    auto* SubBo3DumpXpakEntries = App.add_subcommand("bo3-dump-xpak-entries",
        "Diagnostic: dump every entry >= --bo3-xpak-min-mb to --out (cinematic-encryption brute-force search)");
    SubBo3DumpXpakEntries->add_option("files", Options.Bo3DumpXpakEntries, "Comma separated .xpak file list")->required();
    SubBo3DumpXpakEntries->add_option("--bo3-xpak-min-mb", Options.Bo3XpakMinMB, "Size floor in MB (default 2)");

    auto* SubSalsa20SelfTest = App.add_subcommand("salsa20-selftest", "Verify the hand-written Salsa20 cipher vs a test vector");

    auto* SubBo3DecryptMovie = App.add_subcommand("bo3-decrypt-movie",
        "Decrypt a BO3 cinematic (Salsa20, key comes from the zone's own GSC, never hardcoded)");
    SubBo3DecryptMovie->add_option("zone", Options.Bo3DecryptMovie, "Zone name whose cinematic to decrypt")->required();

    auto* SubIndexAssets = App.add_subcommand("index-assets", "Build a searchable asset index (assets_index.json)");
    SubIndexAssets->add_option("iwd-dir", Options.IndexAssets, "IWD package directory to index");
    SubIndexAssets->add_option("--models-dir", Options.ModelsDir, "Model directory to index");

    auto* SubQueryAssets = App.add_subcommand("query-assets", "Apply a saved filter to an asset index and list the matches");
    SubQueryAssets->add_option("query", Options.QueryAssets, "Asset index (.json) to query")->required();
    SubQueryAssets->add_option("--filter", Options.FilterFile, "Saved filter file to apply");
    SubQueryAssets->add_option("--contact-sheet", Options.ContactSheet, "Also render matches into one contact sheet image");
    SubQueryAssets->add_option("--preview-dir", Options.PreviewDir, "Also render each match to a PNG/WAV in this directory");

    auto* SubAudioProbe = App.add_subcommand("audio-probe", "Decode via AudioPlayer and print duration/rate/peaks");
    SubAudioProbe->add_option("file", Options.AudioProbe, "Audio file to decode")->required()->check(CLI::ExistingFile);

    auto* SubCascList = App.add_subcommand("casc-list", "List every file in a CASC storage matching --casc-pattern");
    SubCascList->add_option("dir", Options.CascList, "CASC storage directory")->required()->check(CLI::ExistingDirectory);
    SubCascList->add_option("--casc-pattern", Options.CascPattern, "Glob pattern (default: *.sab*)");

    auto* SubXpakIndex = App.add_subcommand("xpak-index", "Write every .xpak's index (package, key, offset, stored size) as TSV to --out");
    SubXpakIndex->add_option("dir", Options.XpakIndex, "CASC storage directory")->required()->check(CLI::ExistingDirectory);
    (void)SubXpakIndex;

    auto* SubXsubIndex = App.add_subcommand("xsub-index", "Write every .xsub's hash table (package, type, key, offset, stored size; version 23 adds the size) as TSV to --out");
    SubXsubIndex->add_option("dir", Options.XsubIndex, "Install: CASC storage or plain folder")->required()->check(CLI::ExistingDirectory);
    SubXsubIndex->add_option("--casc-pattern", Options.CascPattern, "Packages to read (default: *.xsub); --type probe also reads each object's block headers and first block");

    auto* SubXsubExtract = App.add_subcommand("xsub-extract", "Write .xsub objects (Cold War, Modern Warfare III, Black Ops 6 and 7), decompressed, as <out>/<key>.bin (a streamkey's key at its 8)");
    SubXsubExtract->add_option("dir", Options.XsubExtract, "Install (CASC storage or plain folder)")->required()->check(CLI::ExistingDirectory);
    SubXsubExtract->add_option("--keys", Options.XsubKeys, "Object keys, hex, comma separated")->required();

    auto* SubCascExtractDir = App.add_subcommand("casc-extract", "Pull one file out of CASC storage as-is");
    SubCascExtractDir->add_option("dir", Options.CascExtractDir, "CASC storage directory")->required()->check(CLI::ExistingDirectory);
    SubCascExtractDir->add_option("name", Options.CascExtractName, "File name inside CASC storage")->required();
    SubCascExtractDir->add_option("outfile", Options.CascExtractOut, "Where to write the extracted file")->required();

    auto* SubZoneDecompress = App.add_subcommand("zone-decompress", "Decompress a local fastfile to raw zone bytes (FastFile::Decompress)");
    SubZoneDecompress->add_option("in", Options.ZoneDecompressIn, "Local .ff fastfile path (already extracted via casc-extract)")->required()->check(CLI::ExistingFile);
    SubZoneDecompress->add_option("outfile", Options.ZoneDecompressOut, "Where to write the raw decompressed zone bytes")->required();

    auto* SubBo4LocalizeDisk = App.add_subcommand("bo4-export-localize-disk",
        "Fully offline BO4 Localize export from an already-decompressed zone (zone-decompress output)");
    SubBo4LocalizeDisk->add_option("in", Options.Bo4ExportLocalizeDiskIn, "Decompressed zone bytes (from zone-decompress)")->required()->check(CLI::ExistingFile);
    SubBo4LocalizeDisk->add_option("outfile", Options.Bo4ExportLocalizeDiskOut, "Where to write the JSONL output")->required();

    auto* SubBo4RawStringsDisk = App.add_subcommand("bo4-export-rawstrings-disk",
        "Fully offline BO4 dialogue/rawstring export (Value+Hash+Pad header shape) from a decompressed zone");
    SubBo4RawStringsDisk->add_option("in", Options.Bo4ExportRawStringsDiskIn, "Decompressed zone bytes (from zone-decompress)")->required()->check(CLI::ExistingFile);
    SubBo4RawStringsDisk->add_option("outfile", Options.Bo4ExportRawStringsDiskOut, "Where to write the JSONL output")->required();

    // bo4-walk: the same command, profile bo4 by default
    for (const char* Name : {"zone-walk", "bo4-walk"})
    {
        auto* SubZoneWalk = App.add_subcommand(Name,
            "Walk decompressed zones with a zonekit profile's reader tables and write every read");
        SubZoneWalk->add_option("zone-list", Options.ZoneWalkList, "Text file: one decompressed zone path per line")->required()->check(CLI::ExistingFile);
        SubZoneWalk->add_option("--profile", Options.ZoneWalkProfile, "zonekit profile: a name (bo4) or a profile directory");
        SubZoneWalk->add_option("--tables", Options.ZoneWalkTables, "Reader table file(s) instead of the profile's; later ones override");
        SubZoneWalk->add_option("--types", Options.ZoneWalkTypes, "Text file: `<type id> <table name>` per line, instead of the profile's")->check(CLI::ExistingFile);
        SubZoneWalk->add_flag("--layout", Options.ZoneWalkLayout, "Also write each read's block and offset in the stream memory (output version 2)");
        SubZoneWalk->add_option("--assets-out", Options.ZoneWalkAssetsOut,
            "Also write <dir>/<zone>.assets.tsv: every asset instance walked (type, parent, start, end, u64 at start) and each image's mip records (layout walk)");
    }

    // A game's own commands (GameCommand.h, e.g. Games/T8/ZoneDiag.cpp)
    for (const GameCommand* Command : GameCommands())
    {
        auto* Sub = App.add_subcommand(Command->Name, Command->Help);
        Sub->add_option("zones", Options.DiskExportIn[Command->Name], Command->InputHelp)->required();
        if (Command->AddOptions) Command->AddOptions(*Sub, Options);
    }
    // disk-export <input> [--game id]: every game on the disk template, plus their aliases
    {
        std::string Ids, KindNames;
        for (const DiskGame::IDiskGame* G : DiskGame::Games()) Ids += std::string(Ids.empty() ? "" : ", ") + G->Id();
        for (const DiskGame::KindInfo& K : DiskGame::Kinds) KindNames += std::string(KindNames.empty() ? "" : ", ") + K.Folder;
        auto* Sub = App.add_subcommand("disk-export",
            "Read zones from disk and export " + KindNames + " to <out>/<game>/<kind>/ (--type; games: " + Ids + ")");
        Sub->add_option("zones", Options.DiskExportIn["disk-export"], "A zone, or an install / folder of zones")->required();
        Sub->add_option("--game", Options.DiskGameId, "The game (" + Ids + "); detected from the zones when omitted");
        for (const DiskGame::IDiskGame* G : DiskGame::Games())
            for (const auto& Alias : G->Aliases())
            {
                auto* A = App.add_subcommand(Alias, std::string(G->Title()) + " from disk (same as disk-export --game " + G->Id() + ")");
                A->add_option("zones", Options.DiskExportIn[Alias], "A zone, or an install / folder of zones")->required();
            }
    }

    auto* SubDecodeIwd = App.add_subcommand("decode-iwd", "BO1 IWD sound -> PCM WAV (--audio-format opus for opus)");
    SubDecodeIwd->add_option("file", Options.DecodeIwd, "IWD sound entry to decode")->required()->check(CLI::ExistingFile);

    auto* SubGltfSelfTest = App.add_subcommand("gltf-selftest", "Write a tiny GLB scene to verify the writer");
    SubGltfSelfTest->add_option("outfile", Options.GltfSelfTest, "Where to write the test .glb")->required();

    auto* SubExportMapGlb = App.add_subcommand("export-map-glb", "Export a T5 map's GfxWorld to a Blender .glb");
    SubExportMapGlb->add_option("file", Options.ExportMapGlb, "Fast file (.ff) holding the GfxWorld")->required()->check(CLI::ExistingFile);
    SubExportMapGlb->add_flag("--map-no-tex", Options.MapNoTex, "Skip embedding textures (much smaller, geometry preview)");

    auto* SubOpenMap = App.add_subcommand("open-map", "Open a map export in Blender: the first time its import script runs and "
                                                      "<map>.blend is saved beside it, later that .blend opens");
    SubOpenMap->add_option("map", Options.OpenMap, "The map's .glb, its _import.py, its .blend, or a folder holding one map")
        ->required()->check(CLI::ExistingPath);
    SubOpenMap->add_flag("--reimport", Options.OpenMapReimport, "Run the import script again, even when the .blend exists");
    SubOpenMap->add_flag("--background", Options.OpenMapBackground, "Import and save the .blend without opening Blender's window");

    auto* SubCerberusSelfTest = App.add_subcommand("cerberus-selftest", "Verify ported CRC32/Salsa20 (Cerberus crypto)");

    auto* SubSeAnimSelfTest = App.add_subcommand("seanim-selftest", "Round-trip a synthetic .seanim + verify pose evaluation");
    auto* SubCastSelfTest = App.add_subcommand("cast-selftest", "Write a synthetic model/anim as .cast and SE, read both back (--out keeps them)");

    auto* SubMakeTestSeAnim = App.add_subcommand("make-test-seanim", "Write a spinning test .seanim (--out sets the path)");
    SubMakeTestSeAnim->add_option("bonename", Options.MakeTestSeAnim, "Bone to animate")->required();

    auto* SubRenderAnimModel = App.add_subcommand("render-anim", "Skinned PNG at one frame (--out sets the path)");
    SubRenderAnimModel->add_option("parts", Options.RenderAnimModel, "\"<model>;<anim>;<frame>\"")->required();

    auto* SubCoverage = App.add_subcommand("coverage", "Per-type disk-extraction coverage for one fastfile (what comes out, and through which mechanism)");
    SubCoverage->add_option("file", Options.Coverage, "Fast file to report on")->required()->check(CLI::ExistingFile);

    auto* SubOatExtract = App.add_subcommand("oat-extract", "Extract a CoD4/WaW/BO1/BO2/MW2/MW3 fastfile via linked OAT libs (in-process)");
    SubOatExtract->add_option("file", Options.OatExtract, "Fast file to extract")->required()->check(CLI::ExistingFile);
    SubOatExtract->add_flag("--oat-list", Options.OatList, "List the fastfile's assets instead of extracting");

    auto* SubXAnimBinDump = App.add_subcommand("xanimbin-dump", "Parse a Mod Tools .xanim_bin, print the resolved record stream");
    SubXAnimBinDump->add_option("file", Options.XAnimBinDump, ".xanim_bin file")->required()->check(CLI::ExistingFile);

    auto* SubXAnimBinSelfTest = App.add_subcommand("xanimbin-selftest", "Assemble Layer 2, report bone/frame/notify counts");
    SubXAnimBinSelfTest->add_option("file", Options.XAnimBinSelfTest, ".xanim_bin file")->required()->check(CLI::ExistingFile);

    auto* SubXAnimBinToSeAnim = App.add_subcommand("xanimbin-to-seanim", "Convert one .xanim_bin to .seanim");
    SubXAnimBinToSeAnim->add_option("infile", Options.XAnimBinToSeAnimIn, ".xanim_bin input")->required()->check(CLI::ExistingFile);
    SubXAnimBinToSeAnim->add_option("outfile", Options.XAnimBinToSeAnimOut, ".seanim output")->required();

    auto* SubXAnimBinExportDir = App.add_subcommand("xanimbin-export-dir",
        "Bulk .xanim_bin -> .seanim, mirroring the source tree so the result is browsable with `ui`");
    SubXAnimBinExportDir->add_option("indir", Options.XAnimBinExportDirIn, "Source directory of .xanim_bin files")
        ->required()->check(CLI::ExistingDirectory);
    SubXAnimBinExportDir->add_option("outdir", Options.XAnimBinExportDirOut, "Destination directory")->required();

    auto* SubOatFx = App.add_subcommand("oat-fx", "Per-element FX/material/blend detail, T5+T6, no game needed");
    SubOatFx->add_option("file", Options.OatFx, "Fast file to read")->required()->check(CLI::ExistingFile);

    auto* SubOatFxExport = App.add_subcommand("oat-fx-export", "Real per-effect JSON export to --out, T5+T6");
    SubOatFxExport->add_option("file", Options.OatFxExport, "Fast file to read")->required()->check(CLI::ExistingFile);

    auto* SubOatXModelMtl = App.add_subcommand("oat-xmodel-materials", "Per-xmodel-surface material/blend, T5+T6");
    SubOatXModelMtl->add_option("file", Options.OatXModelMtl, "Fast file to read")->required()->check(CLI::ExistingFile);

    auto* SubOatWeapon = App.add_subcommand("oat-weapon", "Compiled WeaponDef splash/damage-falloff fields, T5+T6");
    SubOatWeapon->add_option("file", Options.OatWeapon, "Fast file to read")->required()->check(CLI::ExistingFile);

    auto* SubOatRawfiles = App.add_subcommand("oat-rawfiles", "Dump .gsc/.csc (T5) / plain rawfiles+.atr (T6) source to --out");
    SubOatRawfiles->add_option("file", Options.OatRawfiles, "Fast file to read")->required()->check(CLI::ExistingFile);

    auto* SubGenDisc = App.add_subcommand("gen-disc", "Write a flat N-gon disc (or ring) XMODEL_EXPORT");
    SubGenDisc->add_option("outfile", Options.GenDisc, "Where to write the .XMODEL_EXPORT")->required();
    SubGenDisc->add_option("--gen-disc-radius", Options.GenDiscRadius, "Outer radius in units (default 50)");
    SubGenDisc->add_option("--gen-disc-inner-radius", Options.GenDiscInnerRadius, ">0 makes an annulus (ring) instead of a filled disc");
    SubGenDisc->add_option("--gen-disc-segments", Options.GenDiscSegments, "Segment count (default 32)");
    SubGenDisc->add_option("--gen-disc-material", Options.GenDiscMaterial, "Existing alpha material to reference, no new asset needed");

    auto* SubActsExtract = App.add_subcommand("acts-extract", "Extract a modern-CoD fastfile via ate47's acts");
    SubActsExtract->add_option("file", Options.ActsExtract, "Fast file to extract")->required()->check(CLI::ExistingFile);

    auto* SubDumpModel = App.add_subcommand("dump-model", "Print a .semodel's bones/bbox");
    SubDumpModel->add_option("file", Options.DumpModel, ".semodel file")->required()->check(CLI::ExistingFile);

    auto* SubDumpGlb = App.add_subcommand("dump-glb", "Print a .glb's PreviewGeometry stats");
    SubDumpGlb->add_option("file", Options.DumpGlb, ".glb file")->required()->check(CLI::ExistingFile);

    auto* SubRenderGlbPng = App.add_subcommand("render-glb-png", "Headless PNG render of a .glb (--out sets the path)");
    SubRenderGlbPng->add_option("file", Options.RenderGlbPng, "Input .glb")->required()->check(CLI::ExistingFile);

    auto* SubUi = App.add_subcommand("ui", "Open the ImGui + D3D11 asset browser shell on a folder of exported assets");
    SubUi->add_option("dir", Options.Ui, "Folder of exported assets (empty starts with nothing loaded)");

    auto* SubPreviewWindow = App.add_subcommand("preview-window", "Open an interactive D3D11 orbit viewer");
    SubPreviewWindow->add_option("parts", Options.PreviewWindow, "Semicolon separated model part paths")->required();

    auto* SubRenderModel = App.add_subcommand("render-model", "Render a model to PNG (software, --gpu, or --turntable N views)");
    SubRenderModel->add_option("file", Options.RenderModel, "Model file to render")->required()->check(CLI::ExistingFile);
    SubRenderModel->add_flag("--gpu", Options.Gpu, "Use the offscreen D3D11 renderer");
    SubRenderModel->add_option("--turntable", Options.Turntable, "Render N views around the model into one contact sheet");

    auto* SubSabProbe = App.add_subcommand("sab-probe", "Run SABSupport on a sound bank");
    SubSabProbe->add_option("file", Options.SabProbe, "Sound bank (.sab) file, or a folder of .sabl banks")->required()->check(CLI::ExistingPath);

    auto* SubUpdateNameDb = App.add_subcommand("update-name-db", "Fetch echo000/cod-name-db's CSVs into package_index\\*.wni");
    SubUpdateNameDb->add_option("--name-db-dir", Options.NameDbDir, "Override the package_index output directory");

    auto* SubUpdate = App.add_subcommand("update", "Update the hash packages (package_index) from the latest GitHub release; report or apply a newer GrayWolf");
    SubUpdate->add_flag("--check", Options.UpdateCheck, "Only report what is newer, download nothing");
    SubUpdate->add_flag("--app", Options.UpdateApp, "Also download a newer GrayWolf and replace this one once it exits");
    SubUpdate->add_flag("--force", Options.UpdateForce, "Install the packages even when not newer, hand-built or linked");
    SubUpdate->add_option("--package-dir", Options.UpdatePackageDir, "Override the package_index directory");
    SubUpdate->add_option("--repo", Options.UpdateRepo, "owner/name (default: the update_repo setting)");

    // Run by `update --app` / the GUI's Restart now, from a copy of the exe: not for users.
    auto* SubApplyUpdate = App.add_subcommand("apply-update", "(internal) Replace GrayWolf's files with a verified update");
    SubApplyUpdate->group("");
    SubApplyUpdate->add_option("stage", Options.ApplyUpdate, "Verified update folder")->required();
    SubApplyUpdate->add_option("--target", Options.ApplyTarget, "GrayWolf's folder")->required();
    SubApplyUpdate->add_option("--wait-pid", Options.ApplyWaitPid, "Wait for this process to exit first");
    SubApplyUpdate->add_flag("--relaunch", Options.ApplyRelaunch, "Start the new GrayWolf.exe afterwards");

    auto* SubImportHashCsv = App.add_subcommand("import-hash-csv", "Convert a local \"hash,name\" CSV to .wni");
    SubImportHashCsv->add_option("file", Options.ImportHashCsv, "Local hash,name CSV")->required()->check(CLI::ExistingFile);
    SubImportHashCsv->add_option("--import-hash-name", Options.ImportHashName, "Output .wni stem (no extension)");
    SubImportHashCsv->add_option("--name-db-dir", Options.NameDbDir, "Override the package_index output directory");

    auto* SubBo4Decrypt = App.add_subcommand("bo4-decrypt", "Decrypt one BO4 string given as hex bytes [EncryptionID][StringSize][ciphertext][NUL]");
    SubBo4Decrypt->add_option("--hex", Options.Bo4DecryptHex, "Through the zone's string cipher (DecryptString)");
    SubBo4Decrypt->add_option("--disk-hex", Options.Bo4DecryptDiskHex, "Through the on-disk Localize cipher");

    auto* SubFile = App.add_subcommand("file", "Load a package file straight off disk and export it");
    SubFile->add_option("path", Options.File, "Package/fast file to load")->required()->check(CLI::ExistingFile);

    try
    {
        App.parse((int)ArgvPtrs.size(), ArgvPtrs.data());
    }
    catch (const CLI::ParseError& e)
    {
        CliInvoked = true;
        return App.exit(e);
    }


    if (App.get_subcommands().empty())
    {
        // No subcommand was invoked (a plain launch) - fall through to the classic GUI.
        return 0;
    }

    // Resolve the trigger-only bools (subcommands with no options of their own) and the
    // optional-trailing-value flags that could not bind straight to their CliOptions field.
    if (*SubSelfTest) Options.SelfTest = true;
    if (*SubListInstalls) Options.ListInstalls = true;
    if (*SubSalsa20SelfTest) Options.Salsa20SelfTest = true;
    if (*SubCerberusSelfTest) Options.CerberusSelfTest = true;
    if (*SubSeAnimSelfTest) Options.SeAnimSelfTest = true;
    if (*SubCastSelfTest) Options.CastSelfTest = true;
    if (*SubUpdateNameDb) Options.UpdateNameDb = true;
    if (*SubUpdate) Options.Update = true;


    (void)SubVerifyOpus; (void)SubDecompressFF; (void)SubIdentify;
    (void)SubBo3Images; (void)SubBo3DumpXpakEntries; (void)SubBo3DecryptMovie; (void)SubIndexAssets;
    (void)SubQueryAssets; (void)SubAudioProbe; (void)SubCascList; (void)SubCascExtractDir; (void)SubDecodeIwd;
    (void)SubZoneDecompress;
    (void)SubGltfSelfTest; (void)SubExportMapGlb; (void)SubOpenMap; (void)SubMakeTestSeAnim; (void)SubRenderAnimModel;
    (void)SubCoverage; (void)SubOatExtract; (void)SubXAnimBinDump; (void)SubXAnimBinSelfTest; (void)SubXAnimBinToSeAnim;
    (void)SubXAnimBinExportDir; (void)SubOatFx; (void)SubOatFxExport;
    (void)SubOatXModelMtl; (void)SubOatWeapon; (void)SubOatRawfiles; (void)SubGenDisc;
    (void)SubActsExtract; (void)SubDumpModel; (void)SubDumpGlb; (void)SubRenderGlbPng; (void)SubUi;
    (void)SubPreviewWindow; (void)SubRenderModel; (void)SubSabProbe; (void)SubImportHashCsv; (void)SubApplyUpdate;
    (void)SubFile; (void)SubBo4Decrypt;

    CliInvoked = true;
    g_CliMode = true;
    return RunCli(Options);
}

#ifdef _DEBUG
int main(int argc, char** argv)
#else
int APIENTRY WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nCmdShow)
#endif
{
    SetUnhandledExceptionFilter(MyUnhandledExceptionFilter);


    AppLog::Init();

    {
        SettingsManager::LoadSettings("graywolf", "greyhound",
        {
            { "exportimg", "PNG" },
            { "exportsnd", "WAV" },
            // The game folder picked in the shell, so it survives a restart.
            { "game_folder", "" },
            // blender.exe for Open in Blender (BlenderLauncher.h); empty: detected
            { "blender_path", "" },
            // Updates (Updater.h): at GUI start, hash packages installed and a newer app announced
            { "auto_update", "true" },
            { "update_repo", "Kenshin9977/GrayWolf" },
            { "update_use_gh_token", "false" },
            { "keepsndpath", "true" },
            { "skipblankaudio", "false" },
            { "usesabindexbo4", "false" },
            { "exportmodelimg", "true" },
            { "exportalllods", "false" },
            { "exporthitbox", "false" },
            { "exportvtxcolor", "false" },
            { "exportimgnames", "false"},
            { "mdlmtlfolders", "true"},
            { "skipprevmodel", "true"},
            { "skipprevimg", "true"},
            { "skipprevsound", "true"},
            { "skipprevanim", "true"},
            { "export_ma", "false" },
            { "export_obj", "false" },
            { "export_xna", "false" },
            { "export_smd", "false" },
            { "export_xmexport", "false" },
            { "export_xmbin", "false" },
            // Models and anims in Cast (github.com/dtzxporter/cast) by default; SEModel/SEAnim stay
            // available. A graywolf.json that already has these keys keeps its choice.
            { "export_seanim", "false" },
            { "export_semodel", "false" },
            { "export_castanim", "true" },
            { "export_castmdl", "true" },
            { "export_gltf", "false" },
            { "export_glb", "false" },
            { "export_directxanim", "false" },
            { "directxanim_ver", "17" },
            { "global_images", "false" },
            { "patchnormals", "true" },
            { "patchcolor", "true" },
            { "showxanim", "true" },
            { "showxmodel", "true" },
            { "showximage", "false" },
            { "showxrawfiles", "false" },
            { "showxsounds", "false" },
            { "showxmtl", "false" },
            { "match_game_lod_index", "false" },
            { "remove_mdl_basename", "false" },
            { "unit", "Centimeter" }
        });

#ifndef _DEBUG
        int argc = __argc;
        char** argv = __argv;
#endif

        // Headless mode, never touches the UI - a subcommand was invoked (or --help/a parse
        // error was already handled) if CliInvoked comes back true.
        bool CliInvoked = false;
        const int CliResult = RunHeadlessCli(argc, argv, CliInvoked);
        if (CliInvoked)
        {
            return CliResult;
        }

        CleanupFilesystem();
        // GUI only (never a CLI run, so never an export or a regress); off with auto_update=false
        // or GRAYWOLF_NO_UPDATE. Runs on a worker thread.
        Updater::Background::Start(true);

        // A plain double-click (no CLI args) opens the Dear ImGui + D3D11 shell, same as `ui ""`
        // (empty asset list, filled from its own panels). The CLI still works without a D3D11 device.
        const bool Started = WraithX::InitializeAPI() && RunImGuiShell("");
        if (Started)
            WraithX::ShutdownAPI();
        else
            MessageBoxA(NULL, "GrayWolf could not start its window (it needs a Direct3D 11 device).\n"
                "The command line still works: GrayWolf.exe --help", "GrayWolf", MB_OK | MB_ICONEXCLAMATION);

        return Started ? 0 : -1;
    }

    return 0;
}


