#pragma once

#include <cstdint>
#include <fstream>
#include <string>

// A file written as a sequence of typed values. Writing to a file that did not open throws.
class BinaryWriter
{
private:
    std::ofstream Stream;

    // Count elements of ElementSize bytes to the file, which must be open
    void WriteRaw(const void* Data, size_t ElementSize, size_t Count);

public:
    // Creates or empties the file
    bool Create(const std::string& FileName);
    // Opens the file (created when missing) to append to it: every write goes to its end
    bool Open(const std::string& FileName);

    void Close();

    uint64_t GetPosition();
    uint64_t GetLength();
    // Where the next write goes, to rewrite what is there
    void SetPosition(uint64_t Offset);

    void WriteNullTerminatedString(const std::string& Value);

    template<class T>
    void Write(T Value)
    {
        WriteRaw(&Value, sizeof(T), 1);
    }

    template<uint32_t Size>
    void Write(const char(&Value)[Size])
    {
        WriteRaw(&Value, 1, Size);
    }

    template<uint32_t Size>
    void Write(const uint8_t(&Value)[Size])
    {
        WriteRaw(&Value, 1, Size);
    }

    void Write(const int8_t* Buffer, uint32_t Size);
    void Write(const uint8_t* Buffer, uint32_t Size);
};
