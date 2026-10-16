#include "stdafx.h"

// The registry of the games' live readers (ICoDGame.h): each game registers its own from its
// folder, the common code finds one by engine or by the package files that are its.

#include "ICoDGame.h"

#include <map>
#include <stdexcept>

namespace
{
    // By engine, so a lookup that two games could answer always answers the same
    std::map<std::string, const ICoDGame*>& Registry()
    {
        static std::map<std::string, const ICoDGame*> R;
        return R;
    }
}

LiveGameRegistration::LiveGameRegistration(const ICoDGame& Game)
{
    Registry()[Game.Engine()] = &Game;
}

const ICoDGame& LiveGames::Get(const std::string& Engine)
{
    const auto It = Registry().find(Engine);
    if (It == Registry().end())
        throw std::out_of_range("no live game registered for engine " + Engine);
    return *It->second;
}

const ICoDGame* LiveGames::OwnerOf(PackageMark::Container Kind, uint32_t Version, uint32_t EntrySize, const std::string& Folder)
{
    // A mark naming an entry size or a folder first, then one that names neither
    for (const bool Specific : { true, false })
        for (const auto& [Engine, Game] : Registry())
            for (const auto& Mark : Game->Packages())
            {
                if (Mark.Kind != Kind || Mark.Version != Version) continue;
                const bool Named = Mark.EntrySize != 0 || Mark.Folder[0] != '\0';
                if (Named != Specific) continue;
                if (Mark.EntrySize != 0 && Mark.EntrySize != EntrySize) continue;
                if (Mark.Folder[0] != '\0' && Folder.find(Mark.Folder) == std::string::npos) continue;
                return Game;
            }
    return nullptr;
}
