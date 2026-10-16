#pragma once

// Black Ops Cold War's string cipher: the text of a zone's strings (bone and notetrack names...)
// is encrypted, [id 0x80-0xBF][b1][n][n - 1 bytes]. The game's DecryptString is, for every cipher
// the zones use, a byte map per position (rol(c ^ k, r)), tabulated from the decrypted exe into
// the t9 profile's strings.cipher.txt.

#include <string>

namespace ColdWar
{
    // The table in Dir (the t9 profile's folder), read once; false (and why) when it cannot be
    bool LoadStringCipher(const std::string& Dir, std::string& Error);
    // A string's text: decrypted when it starts with a cipher the table knows, as it is when it is
    // not encrypted; false when it is encrypted with no cipher known (Text left as it was)
    bool DecryptString(const std::string& Raw, std::string& Text);
}
