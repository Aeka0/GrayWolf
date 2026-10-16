#pragma once

#include <cstdint>
#include <string>
#include <vector>

// Typed values appended to a buffer in memory (the XMB writer builds its file in one)
class MemoryWriter
{
private:
    std::vector<int8_t> Data;

public:
    explicit MemoryWriter(size_t Capacity) { Data.reserve(Capacity); }

    // The bytes written so far
    uint64_t GetPosition() const { return Data.size(); }
    int8_t* GetCurrentStream() { return Data.data(); }

    void WriteNullTerminatedString(const std::string& Value)
    {
        Write(reinterpret_cast<const uint8_t*>(Value.c_str()), (uint32_t)Value.size() + 1);
    }

    template<class T>
    void Write(T Value)
    {
        Write(reinterpret_cast<const uint8_t*>(&Value), (uint32_t)sizeof(T));
    }

    void Write(const uint8_t* Buffer, uint32_t Size)
    {
        Data.insert(Data.end(), reinterpret_cast<const int8_t*>(Buffer), reinterpret_cast<const int8_t*>(Buffer) + Size);
    }
};
