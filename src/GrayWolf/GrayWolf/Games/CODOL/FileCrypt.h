#pragma once

#include <cstdint>
#include <string>
#include <vector>

// The game's own layer over the files of its .ifs archives (images, sounds, model data; not the
// fast files): zlib, then AES-192 in counter mode (tomcrypt's big-endian counter) per 0x8000-byte
// block, the block's counter {nonce, unpacked size, block offset, block size}, the nonce a CRC-32 of
// the file's base name seeded with its length; the unpacked size follows the data (u32). The
// game's code: 0x76ab00 (key), 0x76abc0 (nonce), 0x76ac80 (a block).
namespace Codol
{
    // Data unwrapped in place when it is wrapped; false (Data untouched) when it is not
    bool UnwrapFile(const std::string& Name, std::vector<uint8_t>& Data);
}
