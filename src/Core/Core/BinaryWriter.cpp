#include "stdafx.h"

#include "BinaryWriter.h"

#include <stdexcept>

bool BinaryWriter::Create(const std::string& FileName)
{
    Close();
    Stream.open(FileName, std::ios::binary | std::ios::trunc);
    return Stream.is_open();
}

bool BinaryWriter::Open(const std::string& FileName)
{
    Close();
    Stream.open(FileName, std::ios::binary | std::ios::app);
    return Stream.is_open();
}

void BinaryWriter::Close()
{
    if (Stream.is_open())
    {
        Stream.close();
    }
    Stream.clear();
}

uint64_t BinaryWriter::GetPosition()
{
    return Stream.is_open() ? (uint64_t)Stream.tellp() : 0;
}

uint64_t BinaryWriter::GetLength()
{
    if (!Stream.is_open())
    {
        return 0;
    }
    const auto Position = Stream.tellp();
    Stream.seekp(0, std::ios::end);
    const auto Length = (uint64_t)Stream.tellp();
    Stream.seekp(Position);
    return Length;
}

void BinaryWriter::SetPosition(uint64_t Offset)
{
    if (Stream.is_open())
    {
        Stream.seekp((std::streamoff)Offset, std::ios::beg);
    }
}

void BinaryWriter::WriteNullTerminatedString(const std::string& Value)
{
    WriteRaw(Value.c_str(), 1, Value.size() + 1);
}

void BinaryWriter::Write(const int8_t* Buffer, uint32_t Size)
{
    WriteRaw(Buffer, 1, Size);
}

void BinaryWriter::Write(const uint8_t* Buffer, uint32_t Size)
{
    WriteRaw(Buffer, 1, Size);
}

void BinaryWriter::WriteRaw(const void* Data, size_t ElementSize, size_t Count)
{
    if (!Stream.is_open())
    {
        throw std::runtime_error("BinaryWriter: no file is open");
    }
    Stream.write(static_cast<const char*>(Data), (std::streamsize)(ElementSize * Count));
}
