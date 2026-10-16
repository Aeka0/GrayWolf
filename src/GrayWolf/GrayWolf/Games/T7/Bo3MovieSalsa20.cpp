#include "stdafx.h"

#include "Bo3MovieSalsa20.h"

#include <cstring>

namespace
{
    inline uint32_t ReadLE32(const uint8_t* P)
    {
        return (uint32_t)P[0] | ((uint32_t)P[1] << 8) | ((uint32_t)P[2] << 16) | ((uint32_t)P[3] << 24);
    }

    inline void WriteLE32(uint8_t* P, uint32_t V)
    {
        P[0] = (uint8_t)(V);
        P[1] = (uint8_t)(V >> 8);
        P[2] = (uint8_t)(V >> 16);
        P[3] = (uint8_t)(V >> 24);
    }

    inline uint32_t RotL(uint32_t X, int N)
    {
        return (X << N) | (X >> (32 - N));
    }

    // Generates one 64-byte Salsa20 keystream block for the given key/nonce/block-counter, per
    // Bernstein's Salsa20 specification (the standard 4x4 word layout, 10 double-rounds = 20 rounds).
    void Salsa20Block(const uint8_t Key[32], const uint8_t Nonce[8], uint64_t BlockCounter, uint8_t Out[64])
    {
        uint32_t X[16];
        X[0] = 0x61707865;                 // "expa"
        X[1] = ReadLE32(Key + 0);
        X[2] = ReadLE32(Key + 4);
        X[3] = ReadLE32(Key + 8);
        X[4] = ReadLE32(Key + 12);
        X[5] = 0x3320646e;                 // "nd 3"
        X[6] = ReadLE32(Nonce + 0);
        X[7] = ReadLE32(Nonce + 4);
        X[8] = (uint32_t)(BlockCounter & 0xFFFFFFFFu);
        X[9] = (uint32_t)(BlockCounter >> 32);
        X[10] = 0x79622d32;                // "2-by"
        X[11] = ReadLE32(Key + 16);
        X[12] = ReadLE32(Key + 20);
        X[13] = ReadLE32(Key + 24);
        X[14] = ReadLE32(Key + 28);
        X[15] = 0x6b206574;                // "te k"

        uint32_t Y[16];
        std::memcpy(Y, X, sizeof(Y));

        for (int i = 0; i < 10; i++)
        {
            // Column round
            Y[4] ^= RotL(Y[0] + Y[12], 7);
            Y[8] ^= RotL(Y[4] + Y[0], 9);
            Y[12] ^= RotL(Y[8] + Y[4], 13);
            Y[0] ^= RotL(Y[12] + Y[8], 18);

            Y[9] ^= RotL(Y[5] + Y[1], 7);
            Y[13] ^= RotL(Y[9] + Y[5], 9);
            Y[1] ^= RotL(Y[13] + Y[9], 13);
            Y[5] ^= RotL(Y[1] + Y[13], 18);

            Y[14] ^= RotL(Y[10] + Y[6], 7);
            Y[2] ^= RotL(Y[14] + Y[10], 9);
            Y[6] ^= RotL(Y[2] + Y[14], 13);
            Y[10] ^= RotL(Y[6] + Y[2], 18);

            Y[3] ^= RotL(Y[15] + Y[11], 7);
            Y[7] ^= RotL(Y[3] + Y[15], 9);
            Y[11] ^= RotL(Y[7] + Y[3], 13);
            Y[15] ^= RotL(Y[11] + Y[7], 18);

            // Row round
            Y[1] ^= RotL(Y[0] + Y[3], 7);
            Y[2] ^= RotL(Y[1] + Y[0], 9);
            Y[3] ^= RotL(Y[2] + Y[1], 13);
            Y[0] ^= RotL(Y[3] + Y[2], 18);

            Y[6] ^= RotL(Y[5] + Y[4], 7);
            Y[7] ^= RotL(Y[6] + Y[5], 9);
            Y[4] ^= RotL(Y[7] + Y[6], 13);
            Y[5] ^= RotL(Y[4] + Y[7], 18);

            Y[11] ^= RotL(Y[10] + Y[9], 7);
            Y[8] ^= RotL(Y[11] + Y[10], 9);
            Y[9] ^= RotL(Y[8] + Y[11], 13);
            Y[10] ^= RotL(Y[9] + Y[8], 18);

            Y[12] ^= RotL(Y[15] + Y[14], 7);
            Y[13] ^= RotL(Y[12] + Y[15], 9);
            Y[14] ^= RotL(Y[13] + Y[12], 13);
            Y[15] ^= RotL(Y[14] + Y[13], 18);
        }

        for (int i = 0; i < 16; i++)
            WriteLE32(Out + i * 4, Y[i] + X[i]);
    }
}

void Salsa20::CryptInPlace(uint8_t* Data, size_t Size, const uint8_t Key[32], const uint8_t Nonce[8])
{
    uint8_t Block[64];
    uint64_t Counter = 0;
    size_t Offset = 0;

    while (Offset < Size)
    {
        Salsa20Block(Key, Nonce, Counter, Block);
        const size_t Chunk = (Size - Offset < 64) ? (Size - Offset) : 64;
        for (size_t i = 0; i < Chunk; i++)
            Data[Offset + i] ^= Block[i];
        Offset += Chunk;
        Counter++;
    }
}

bool Salsa20::SelfTest()
{
    // Test vector generated with pycryptodome's Salsa20, not derived from any BO3 file or key.
    // Key/nonce are the byte sequences 0x00..0x1f / 0x00..0x07; the 127-byte plaintext spans a
    // block boundary, so the partial-final-block path runs too.
    uint8_t Key[32];
    for (int i = 0; i < 32; i++) Key[i] = (uint8_t)i;
    uint8_t Nonce[8];
    for (int i = 0; i < 8; i++) Nonce[i] = (uint8_t)i;

    const uint8_t Plaintext[127] = {
        0x54,0x68,0x65,0x20,0x71,0x75,0x69,0x63,0x6b,0x20,0x62,0x72,0x6f,0x77,0x6e,0x20,
        0x66,0x6f,0x78,0x20,0x6a,0x75,0x6d,0x70,0x73,0x20,0x6f,0x76,0x65,0x72,0x20,0x74,
        0x68,0x65,0x20,0x6c,0x61,0x7a,0x79,0x20,0x64,0x6f,0x67,0x21,0x21,0x21,0x21,0x00,
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    };

    const uint8_t ExpectedCiphertext[127] = {
        0x7a,0xc5,0x6a,0x7f,0x69,0x22,0x40,0xad,0xbd,0x52,0xd1,0xdb,0x47,0x93,0x3a,0xd7,
        0x49,0xb4,0x3c,0x88,0x11,0xe9,0xb5,0xa2,0x6a,0xc4,0x83,0x62,0xcb,0x8b,0xe6,0xc8,
        0x1f,0xda,0x25,0x13,0x37,0x23,0xae,0x55,0x5c,0x27,0x9f,0xf2,0xdf,0x57,0xbd,0xa5,
        0xfd,0xd8,0x05,0x7d,0x46,0x32,0x69,0x90,0xe5,0xf1,0x36,0xe2,0xfc,0xb7,0xbb,0x7c,
        0xa1,0x3a,0x2b,0x59,0xd9,0x04,0x7b,0x8d,0xbe,0xb9,0x3e,0xc4,0xb7,0x8c,0xe1,0xa5,
        0x9b,0xc2,0x10,0x64,0x13,0x18,0xcc,0xce,0x69,0x4d,0x30,0xff,0x81,0xd2,0xaf,0xe7,
        0xbd,0xf5,0xa3,0xd5,0x8c,0xb4,0xf9,0xa4,0xed,0x52,0x47,0x82,0x3e,0x14,0x61,0x8f,
        0x06,0xdc,0x61,0xcf,0xe4,0xb8,0xe2,0xba,0x83,0x67,0x83,0xb2,0x80,0xef,0xed,
    };

    uint8_t Scratch[127];
    std::memcpy(Scratch, Plaintext, sizeof(Scratch));
    CryptInPlace(Scratch, sizeof(Scratch), Key, Nonce);

    return std::memcmp(Scratch, ExpectedCiphertext, sizeof(Scratch)) == 0;
}
