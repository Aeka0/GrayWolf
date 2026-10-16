#pragma once

#include <cstdint>
#include <string>
#include <vector>

// String helpers the standard library lacks. Blanks are the C locale's: space, \t, \n, \v, \f, \r;
// case is ASCII only, other bytes are left as they are.
namespace Strings
{
    // Value without its leading and trailing blanks
    [[nodiscard]] std::string Trim(std::string Value);
    // Empty, or blanks only
    bool IsNullOrWhiteSpace(const std::string& Value);

    std::vector<uint8_t> FromBase64String(const std::string& Value);

    // Subject with every Search replaced, left to right
    [[nodiscard]] std::string Replace(std::string Subject, const std::string& Search, const std::string& Replace);
    // Search cut at each Split, empty parts left out when StripBlank
    std::vector<std::string> SplitString(const std::string& Search, const char Split, bool StripBlank = false);

    [[nodiscard]] std::string ToLower(std::string Value);
    [[nodiscard]] std::string ToUpper(std::string Value);

    // UTF-16 <-> UTF-8; ToUnicodeString throws std::range_error on bytes that are not UTF-8
    std::string ToNormalString(const std::wstring& Value);
    std::wstring ToUnicodeString(const std::string& Value);
}
