#pragma once

#include <cstdint>
#include <cstring>
#include <fstream>
#include <string>

// A file read as a cursor of typed values. Past the end, reads come back zeroed (and the position
// stays at the end); reading a file that did not open throws.
class BinaryReader
{
private:
    mutable std::ifstream Stream;
    uint64_t FileLength = 0;

    // Count bytes into Data; the number read
    uint64_t ReadRaw(void* Data, uint64_t Count);

public:
    bool Open(const std::string& FileName);

    void Close();

    uint64_t GetLength() const;
    uint64_t GetPosition() const;

    void SetPosition(uint64_t Offset);
    void Advance(uint64_t Length);

    template <class T>
    T Read()
    {
        T ResultValue;
        std::memset(&ResultValue, 0, sizeof(ResultValue));
        ReadRaw(&ResultValue, sizeof(ResultValue));
        return ResultValue;
    }
    template <class T>
    T Read(uint64_t Offset)
    {
        SetPosition(Offset);
        return Read<T>();
    }
    std::string ReadNullTerminatedString();
    // Length bytes in a new[] block (zeroed past what was read), Result: the number read
    int8_t* Read(uint64_t Length, uint64_t& Result);
    void Read(uint8_t* Buffer, uint64_t Length, uint64_t& Result);
};
