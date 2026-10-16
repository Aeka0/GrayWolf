#include "stdafx.h"

#include "StringCipher.h"

#include "Games/Common/StringCipher.h"

namespace
{
    const StringCipher::Table* Cipher = nullptr;
}

bool ColdWar::LoadStringCipher(const std::string& Dir, std::string& Error)
{
    Cipher = StringCipher::Load(Dir, Error);
    return Cipher != nullptr;
}

bool ColdWar::DecryptString(const std::string& Raw, std::string& Text)
{
    return StringCipher::Decrypt(Cipher, Raw, Text);
}
