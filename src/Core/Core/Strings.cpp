#include "stdafx.h"

#include "Strings.h"

#include <stdexcept>

#include <Windows.h>

namespace
{
    constexpr const char* Blanks = " \t\n\v\f\r";
}

std::string Strings::Trim(std::string Value)
{
    const auto First = Value.find_first_not_of(Blanks);
    if (First == std::string::npos)
    {
        return std::string();
    }
    return Value.substr(First, Value.find_last_not_of(Blanks) - First + 1);
}

bool Strings::IsNullOrWhiteSpace(const std::string& Value)
{
    return Value.find_first_not_of(Blanks) == std::string::npos;
}

std::vector<uint8_t> Strings::FromBase64String(const std::string& Value)
{
    std::vector<uint8_t> ByteBuffer;
    
    uint8_t* InputPtr = (uint8_t*)Value.c_str();
    uint8_t* EndPtr = InputPtr + Value.size();

    // For this to work, value's size must be divisible by 4
    if (Value.size() % 4 != 0) { return ByteBuffer; }

    const uint32_t IntA = (uint32_t)'A';
    const uint32_t Inta = (uint32_t)'a';
    const uint32_t Int0 = (uint32_t)'0';
    const uint32_t IntEq = (uint32_t)'=';
    const uint32_t IntPlus = (uint32_t)'+';
    const uint32_t IntSlash = (uint32_t)'/';
    const uint32_t IntSpace = (uint32_t)' ';
    const uint32_t IntTab = (uint32_t)'\t';
    const uint32_t IntNLn = (uint32_t)'\n';
    const uint32_t IntCRt = (uint32_t)'\r';
    const uint32_t IntAtoZ = (uint32_t)('Z' - 'A');
    const uint32_t Int0to9 = (uint32_t)('9' - '0');

    uint32_t CurrentBlock = 0xFF;

    uint32_t CurrentCode = 0;

    while (true)
    {
        if (InputPtr >= EndPtr) { goto AllInputConsumed; }

        CurrentCode = (uint32_t)(*InputPtr);
        InputPtr++;

        if (CurrentCode - IntA <= IntAtoZ) { CurrentCode -= IntA; }

        else if (CurrentCode - Inta <= IntAtoZ) { CurrentCode -= (Inta - 26); }

        else if (CurrentCode - Int0 <= Int0to9) { CurrentCode -= (Int0 - 52); }

        else
        {
            switch (CurrentCode)
            {
            case IntPlus:
                CurrentCode = 62;
                break;
            case IntSlash:
                CurrentCode = 63;
                break;
            case IntEq:
                goto EqualityReached;
            }
        }

        CurrentBlock = (CurrentBlock << 6) | CurrentCode;

        if ((CurrentBlock & 0x80000000) != 0)
        {
            ByteBuffer.push_back((uint8_t)(CurrentBlock >> 16));
            ByteBuffer.push_back((uint8_t)(CurrentBlock >> 8));
            ByteBuffer.push_back((uint8_t)(CurrentBlock));
            CurrentBlock = 0xFF;
        }
    }

    // Only hit if we make it to the '=' early
EqualityReached:
    
    // '=' can only be in the last pos
    if (InputPtr == EndPtr)
    {
        // Code is 0 for trailing
        CurrentBlock <<= 6;

        ByteBuffer.push_back((uint8_t)(CurrentBlock >> 16));
        ByteBuffer.push_back((uint8_t)(CurrentBlock >> 8));

        CurrentBlock = 0xFF;
    }
    else
    {
        if (InputPtr == (EndPtr - 1) && *(InputPtr) == '=')
        {
            // Code is 0 for each trailing
            CurrentBlock <<= 12;

            ByteBuffer.push_back((uint8_t)(CurrentBlock >> 16));

            CurrentBlock = 0xFF;
        }
    }

AllInputConsumed:


    return ByteBuffer;
}

std::string Strings::Replace(std::string Subject, const std::string& Search, const std::string& Replace)
{
    size_t CurrentPosition = 0;
    while ((CurrentPosition = Subject.find(Search, CurrentPosition)) != std::string::npos)
    {
        Subject.replace(CurrentPosition, Search.length(), Replace);
        CurrentPosition += Replace.length();
    }
    return Subject;
}

std::vector<std::string> Strings::SplitString(const std::string& Search, const char Split, bool StripBlank)
{
    std::vector<std::string> Result;

    size_t Start = 0;
    size_t End = Search.find_first_of(Split);

    while (End <= std::string::npos)
    {
        if (StripBlank)
        {
            auto Str = Search.substr(Start, End - Start);
            if (Str != "") { Result.emplace_back(Str); }
        }
        else
        {
            Result.emplace_back(Search.substr(Start, End - Start));
        }

        if (End == std::string::npos) { break; }

        Start = End + 1;
        End = Search.find_first_of(Split, Start);
    }

    return Result;
}

std::string Strings::ToLower(std::string Value)
{
    for (auto& Char : Value)
    {
        if (Char >= 'A' && Char <= 'Z') { Char = (char)(Char - 'A' + 'a'); }
    }
    return Value;
}

std::string Strings::ToUpper(std::string Value)
{
    for (auto& Char : Value)
    {
        if (Char >= 'a' && Char <= 'z') { Char = (char)(Char - 'a' + 'A'); }
    }
    return Value;
}

std::string Strings::ToNormalString(const std::wstring& Value)
{
    if (Value.empty())
    {
        return std::string();
    }
    const int Size = WideCharToMultiByte(CP_UTF8, 0, Value.data(), (int)Value.size(), nullptr, 0, nullptr, nullptr);
    std::string Result((size_t)Size, '\0');
    WideCharToMultiByte(CP_UTF8, 0, Value.data(), (int)Value.size(), Result.data(), Size, nullptr, nullptr);
    return Result;
}

std::wstring Strings::ToUnicodeString(const std::string& Value)
{
    if (Value.empty())
    {
        return std::wstring();
    }
    const int Size = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, Value.data(), (int)Value.size(), nullptr, 0);
    if (Size <= 0)
    {
        throw std::range_error("not UTF-8: " + Value);
    }
    std::wstring Result((size_t)Size, L'\0');
    MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, Value.data(), (int)Value.size(), Result.data(), Size);
    return Result;
}
