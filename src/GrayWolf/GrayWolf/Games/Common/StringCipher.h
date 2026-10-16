#pragma once

// The string cipher of Cold War and Modern Warfare 2019: a zone string's text encrypted as
// [id 0x80-0xBF][b1][n][n - 1 bytes]. The game's DecryptString is, for every cipher the zones use, a
// byte map per position (rol(c ^ k, r)), tabulated from the decrypted exe into the profile's
// strings.cipher.txt.

#include <string>

namespace StringCipher
{
    struct Table;
    // The table in Dir (a zonekit profile's folder), read once per folder; nullptr (and why) when
    // it cannot be
    const Table* Load(const std::string& Dir, std::string& Error);
    // A string's text: decrypted when it starts with a cipher the table knows, as it is when it is
    // not encrypted; false when it is encrypted with no cipher known (Text left as it was)
    bool Decrypt(const Table* Cipher, const std::string& Raw, std::string& Text);
}
