#pragma once

#include <string>
#include <vector>

struct AssetEntry;

// Decodes an indexed IWD sound (block MS-ADPCM) and writes it to disk as either a PCM16 WAV or,
// when AsOpus is set, an Ogg/Opus file (~1/5 the size, reusing the project's Sound encoder). Returns
// false if the source cannot be located, decoded, or encoded.
bool RenderAudioToFile(const AssetEntry& Entry, const std::string& InstallDir, const std::string& OutPath, bool AsOpus);

// Same, keyed by the IPAK's own 64-bit key rather than by a data-segment offset. A BO2 image's key
// is (GfxImage.hash << 32) | streamedParts[0].hash - both fields sit in the fastfile, where the
// offset and ipakIndex do NOT: the engine fills those in when it loads the package, so a zone read
// offline has them at zero and only the hashes to go on.
size_t ExtractIpakImagesByKey(const std::string& IpakDir,
                              const std::vector<std::pair<uint64_t, std::string>>& Jobs);

// Extracts a single image by name (e.g. "ch_rubble01_col") from any .iwd under InstallDir - looking
// for the entry "images/<ImageName>.iwi" - and decodes it to a PNG. Used to texture models that were
// loaded in-process via OAT (their material colour-map names match the IWD IWi names). Returns false
// if the image is not found in any package or cannot be decoded. Requires WraithX::InitializeAPI (WIC).
bool ExtractIwiToPng(const std::string& InstallDir, const std::string& ImageName, const std::string& OutPng);

// Decodes ONE IWD sound by its package entry name (e.g. "sound/.../foo.wav") - searches every .iwd
// under InstallDir, decodes the first match (BO1 block MS-ADPCM, or passes a straight RIFF through)
// and writes it to OutWavPath. Used by the per-zone sound pass for STREAMED aliases (their audio is
// in the IWDs, referenced by filename). Returns false if not found or undecodable.
bool DecodeIwdSoundByName(const std::string& InstallDir, const std::string& EntryName, const std::string& OutWavPath);

// Extracts EVERY sound out of the install's IWD packages: walks all .iwd under InstallDir, decodes
// each "*.wav" entry (BO1's block MS-ADPCM) to a PCM16 WAV under OutDir, preserving the entry's
// sound/... subpath; already-RIFF entries are passed through. Deduplicates by entry name across
// packages. Returns how many were written. OAT loads T5/BO1 sounds but has no dumper.
// Requires WraithX::InitializeAPI.
size_t ExtractIwdSounds(const std::string& InstallDir, const std::string& OutDir);

// Decodes an indexed image (its IWi, pulled from the install's IWD packages) to a PNG on disk.
// Reuses CoDIWITranslator (IWi -> DDS) + Image::ConvertImageMemory (DDS -> PNG). Returns false
// if the source cannot be located or decoded. Requires WraithX::InitializeAPI first (WIC).
bool RenderImageToPNG(const AssetEntry& Entry, const std::string& InstallDir, const std::string& OutPngPath);

// True if the asset name looks like a normal map (CoD suffixes: _n / _nml / _normal).
bool IsNormalMapName(const std::string& Name);

// Inverts the green channel of a PNG in place - the DirectX->OpenGL normal-map convention flip
// (what Blender and most DCC tools expect). Returns false if the file can't be read or written.
// Requires WraithX::InitializeAPI first (WIC).
bool FlipNormalMapGreen(const std::string& PngPath);

// Composites square thumbnails of the given images into one PNG "contact sheet" - the headless
// way to visually review a filter's matches (e.g. every HUD icon of a map at a glance). Returns
// false if nothing could be decoded. Requires WraithX::InitializeAPI first (WIC).
bool BuildContactSheet(const std::vector<const AssetEntry*>& Images, const std::string& InstallDir,
                       const std::string& OutPngPath, int ThumbSize = 64, int MaxImages = 1024);
