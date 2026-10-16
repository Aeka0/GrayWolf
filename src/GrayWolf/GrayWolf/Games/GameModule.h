#pragma once

#include <cstdint>
#include <initializer_list>
#include <string>
#include <vector>

// One entry per supported title: what a title is and how far its support has got. What can
// read it is found through IZoneSource.

enum class GameSupportStage
{
    // The container decompresses; nothing reads its asset list yet.
    Container,
    // The asset list is read, but no type can be extracted yet.
    List,
    // The asset list is read and assets can be extracted.
    Assets,
};

struct GameModule
{
    // Short engine name the readers use: "IW3".."IW6", "T4".."T8".
    const char* Engine;
    const char* Title;
    // The executable that identifies an install on disk, empty when several titles share one.
    const char* InstallMarker;
    // Magic plus version, because several titles share a magic and differ only by version.
    const char* FastFileMagic;
    uint32_t    FastFileVersion;
    GameSupportStage Stage;

    // When several rows share a magic and version (Black Ops 6 and 7, a campaign and a multiplayer
    // build), what tells their fast files apart, in this order: the nearest folder above the file
    // named InstallFolder ("cod24"), then ClaimsHeader, which reads the file's header (a loose
    // file). The claims of rows sharing a version must exclude each other.
    const char* InstallFolder = nullptr;
    bool (*ClaimsHeader)(const std::string& FastFilePath) = nullptr;
    // The zonekit profile reading this row's zones, when its title has several (nullptr: the
    // title's only one)
    const char* Profile = nullptr;
};

// A static instance in Games/<game>/Module.cpp adds that title's rows, so a new title (or a
// stage change) edits its own file only.
struct GameModuleRegistration
{
    GameModuleRegistration(std::initializer_list<GameModule> Rows);
};

// Every title this build can read from disk, newest reader last.
const std::vector<GameModule>& RegisteredGameModules();

// The module matching a fastfile's magic and version, or nullptr when no reader claims it. When
// several rows share both, FastFilePath decides (InstallFolder, then ClaimsHeader); without a
// path, or when none claims it, the row with neither, or nullptr.
const GameModule* FindGameModule(const std::string& Magic, uint32_t Version, const std::string& FastFilePath = {});

// The module of a fast file on disk, by its header and its path
const GameModule* FindGameModuleForFile(const std::string& FastFilePath);
