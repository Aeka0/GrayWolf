#pragma once

// The end of every map export, whatever read the level: the scene's images named from the files
// the image kind writes, the copies the map settings ask for (map_png, map_textures) scheduled
// after the run's dependencies, and the scene written by each enabled ISceneWriter (the .glb
// always, Cast with map_cast, Radiant with map_radiant). A map reader fills a MapScene, writes its textures through its
// image kind, then calls this.

#include "DiskGame.h"
#include "MapScene.h"

#include <functional>
#include <map>
#include <set>
#include <string>

namespace Materials { class IAdapter; }

namespace Maps
{
    // Names: the images the scene's materials (and sky) name, as the image kind names them;
    // FileOf(Name) the absolute path the image kind wrote it to (with Ext). Stem: the map's
    // path without extension. True when the .glb was written; failures go to T under Zone.
    bool WriteMapFiles(MapScene& Scene, const std::set<std::string>& Names,
                       const std::function<std::string(const std::string&)>& FileOf, const std::string& Ext,
                       const std::string& Stem, const Materials::IAdapter& Adapter, const std::string& Zone,
                       const DiskGame::Context& C, DiskGame::Totals& T, std::map<std::string, size_t>& Counters);
}
