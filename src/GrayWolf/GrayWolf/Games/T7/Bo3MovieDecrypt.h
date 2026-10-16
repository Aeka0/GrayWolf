#pragma once

#include <string>

// Decrypts a BO3 zombies cinematic (SEE ending, or any other keyed lui::play_movie call). Algorithm
// No key is hardcoded: it is extracted, every call, from the map's own compiled GSC via acts.
//
// ZoneName's *_patch.ff (falling back to plain .ff) under InstallDir/zone/ is decompiled; every keyed
// lui::prime_movie/play_movie call found is tried against InstallDir/video/<name>.mkv until one
// resolves and decrypts. ActsHandler defaults to "bo3" when empty. OutPath empty defaults to
// "<movie name>_decrypted.mkv" in the working directory. Report gets the full trail.
bool Bo3DecryptMovie(const std::string& InstallDir, const std::string& ZoneName,
                     const std::string& ActsExe, const std::string& ActsHandler,
                     const std::string& OutPath, std::string& Report);
