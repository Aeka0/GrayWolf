#include "stdafx.h"

#include "MemoryReader.h"


MemoryReader::MemoryReader()
{
    DataPointer = nullptr;
    DataLength = 0;
    CurrentPosition = 0;
    KeepAlive = false;
}

MemoryReader::MemoryReader(int8_t* Stream, uint64_t StreamLength, bool KeepStreamAlive)
{
    DataPointer = Stream;
    DataLength = StreamLength;
    CurrentPosition = 0;
    KeepAlive = KeepStreamAlive;
}

MemoryReader::~MemoryReader()
{
    Close();
}

void MemoryReader::Setup(int8_t* Stream, uint64_t StreamLength, bool KeepStreamAlive)
{
    if (DataPointer != nullptr)
    {
        Close();
    }
    DataPointer = Stream;
    DataLength = StreamLength;
    CurrentPosition = 0;
    KeepAlive = KeepStreamAlive;
}

void MemoryReader::Close()
{
    if (DataPointer != nullptr)
    {
        if (!KeepAlive)
        {
            delete[] DataPointer;
        }
        DataPointer = nullptr;
    }
}

uint64_t MemoryReader::GetLength() const
{
    return DataLength;
}

uint64_t MemoryReader::GetPosition() const
{
    return CurrentPosition;
}

int8_t* MemoryReader::GetCurrentStream() const
{
    return DataPointer;
}

int8_t* MemoryReader::GetCurrentStream(const size_t size)
{

    if ((size + CurrentPosition) <= DataLength)
    {
        auto result = DataPointer + CurrentPosition;
        CurrentPosition += size;
        return result;
    }

    return nullptr;
}

void MemoryReader::SetPosition(uint64_t Offset)
{
    if (Offset <= DataLength)
    {
        CurrentPosition = Offset;
    }
}

void MemoryReader::Advance(uint64_t Length)
{
    if ((Length + CurrentPosition) <= DataLength)
    {
        CurrentPosition += Length;
    }
}

void MemoryReader::Read(uint64_t Length, int8_t* Result)
{
    if (DataPointer != nullptr && Result != nullptr)
    {
        if ((CurrentPosition + Length) <= DataLength)
        {
            std::memcpy(Result, (DataPointer + CurrentPosition), (size_t)Length);
            CurrentPosition += Length;
        }
    }
}

std::string MemoryReader::ReadNullTerminatedString()
{
    if (DataPointer != nullptr)
    {
        // Up to the null, found at once (a name index reads millions of these)
        const char* Start = reinterpret_cast<const char*>(DataPointer) + CurrentPosition;
        const size_t Left = CurrentPosition < DataLength ? (size_t)(DataLength - CurrentPosition) : 0;
        const char* End = static_cast<const char*>(std::memchr(Start, 0, Left));
        if (End != nullptr)
        {
            CurrentPosition += (uint64_t)(End - Start) + 1;
            return std::string(Start, End);
        }
        // no null before the end: consumed, then the failed read Read<char> would throw
        CurrentPosition = (std::max)(CurrentPosition, DataLength);
        return std::string(1, Read<char>());
    }
    return "";
}
