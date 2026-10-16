#pragma once

#include <cstdint>

struct Salsa20Key
{
    uint8_t* Key;
    // The length of the key, either 128 or 256 bits
    uint32_t KeyLength;
    uint8_t* IV;

    Salsa20Key()
    {
        Key = nullptr;
        KeyLength = 0;
        IV = nullptr;
    }
};

class Encryption
{
public:

    static uint32_t Salsa20Block(int8_t* Buffer, uint32_t BufferSize, const Salsa20Key& Key);
};