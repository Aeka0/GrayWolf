#pragma once

// <map>_import.py, the Blender script written beside every map .glb: it reads the .glb itself
// (not through Blender's glTF importer) so that a model placed many times becomes one collection
// placed by Geometry Nodes instances (or collection instances, a setting in the script). Its body
// is tools/mapimport/map_import.py, embedded by tools/mapimport/embed.py; this fills its parameters.

#include <string>

namespace Maps
{
    struct ImportScriptParams
    {
        std::string Glb;         // the .glb's file name, beside the script
        std::string Materials;   // the materials sidecar's file name ("" when none)
        std::string Sky;         // a sky texture relative to the script ("" when none)
        const float* Sun = nullptr;   // unit vector toward the sun, game space (null: unknown)
        bool Atmosphere = false;      // the sky is the game's atmosphere: a physical sky in Blender
    };

    std::string ImportScript(const ImportScriptParams& P);
}
