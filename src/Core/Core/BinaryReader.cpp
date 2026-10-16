#include "stdafx.h"

#include "BinaryReader.h"

#include <stdexcept>

bool BinaryReader::Open(const std::string& FileName)
{
    Close();

    Stream.open(FileName, std::ios::binary);
    if (!Stream.is_open())
    {
        return false;
    }
    Stream.seekg(0, std::ios::end);
    FileLength = (uint64_t)Stream.tellg();
    Stream.seekg(0, std::ios::beg);
    return true;
}

void BinaryReader::Close()
{
    if (Stream.is_open())
    {
        Stream.close();
    }
    Stream.clear();
    FileLength = 0;
}

uint64_t BinaryReader::GetLength() const
{
    return FileLength;
}

uint64_t BinaryReader::GetPosition() const
{
    if (!Stream.is_open())
    {
        return 0;
    }
    // A read past the end leaves the stream failed, its position still the end
    Stream.clear();
    return (uint64_t)Stream.tellg();
}

void BinaryReader::SetPosition(uint64_t Offset)
{
    if (Stream.is_open())
    {
        Stream.clear();
        Stream.seekg((std::streamoff)Offset, std::ios::beg);
    }
}

void BinaryReader::Advance(uint64_t Length)
{
    if (Stream.is_open())
    {
        Stream.clear();
        Stream.seekg((std::streamoff)Length, std::ios::cur);
    }
}

uint64_t BinaryReader::ReadRaw(void* Data, uint64_t Count)
{
    if (!Stream.is_open())
    {
        throw std::runtime_error("BinaryReader: no file is open");
    }
    Stream.read(static_cast<char*>(Data), (std::streamsize)Count);
    return (uint64_t)Stream.gcount();
}

std::string BinaryReader::ReadNullTerminatedString()
{
    std::string Result;
    for (char Char = Read<char>(); Char != 0; Char = Read<char>())
    {
        Result += Char;
    }
    return Result;
}

int8_t* BinaryReader::Read(uint64_t Length, uint64_t& Result)
{
    if (!Stream.is_open())
    {
        return nullptr;
    }
    int8_t* ResultBlock = new int8_t[(size_t)Length];
    std::memset(ResultBlock, 0, (size_t)Length);
    Result = ReadRaw(ResultBlock, Length);
    return ResultBlock;
}

void BinaryReader::Read(uint8_t* Buffer, uint64_t Length, uint64_t& Result)
{
    if (Stream.is_open())
    {
        Result = ReadRaw(Buffer, Length);
    }
}
