#include "stdafx.h"

#include "CerberusCrypto.h"

#include <cstring>
#include <cstdio>
#include <string>

namespace Cerberus
{
    // CRC32 (zlib) - ported from Cerberus.Logic.CRC32.

    namespace
    {
        struct Crc32Table
        {
            uint32_t T[256];
            Crc32Table()
            {
                for (uint32_t i = 0; i < 256; i++)
                {
                    uint32_t c = i;
                    for (int k = 0; k < 8; k++)
                        c = (c & 1) ? (0xEDB88320u ^ (c >> 1)) : (c >> 1);
                    T[i] = c;
                }
            }
        };
        const Crc32Table g_Crc;
    }

    Crc32::Crc32() : m_Value(0xFFFFFFFFu) {}

    void Crc32::Update(uint8_t Byte)
    {
        m_Value = g_Crc.T[(m_Value ^ Byte) & 0xFF] ^ (m_Value >> 8);
    }

    void Crc32::Update(const uint8_t* Data, size_t Size)
    {
        for (size_t i = 0; i < Size; i++) Update(Data[i]);
    }

    uint32_t Crc32::Compute(const uint8_t* Data, size_t Size)
    {
        Crc32 C;
        C.Update(Data, Size);
        return C.Value();
    }

    // Salsa20 - ported from Cerberus.Logic.Salsa20 (Salsa20CryptoTransform).

    namespace
    {
        inline uint32_t Rotate(uint32_t v, int c) { return (v << c) | (v >> (32 - c)); }
        inline uint32_t ToU32(const uint8_t* p) { return (uint32_t)(p[0] | (p[1] << 8) | (p[2] << 16) | ((uint32_t)p[3] << 24)); }
        inline void ToBytes(uint32_t v, uint8_t* out) { out[0] = (uint8_t)v; out[1] = (uint8_t)(v >> 8); out[2] = (uint8_t)(v >> 16); out[3] = (uint8_t)(v >> 24); }

        // "expand 32-byte k" / "expand 16-byte k" as little-endian words.
        const uint8_t c_sigma[16] = { 'e','x','p','a', 'n','d',' ','3', '2','-','b','y', 't','e',' ','k' };
        const uint8_t c_tau[16]   = { 'e','x','p','a', 'n','d',' ','1', '6','-','b','y', 't','e',' ','k' };
    }

    Salsa20::Salsa20(const uint8_t* Key, size_t KeyLen, const uint8_t* Iv, int Rounds)
        : m_Rounds(Rounds)
    {
        std::memset(m_State, 0, sizeof(m_State));
        m_State[1] = ToU32(Key + 0);
        m_State[2] = ToU32(Key + 4);
        m_State[3] = ToU32(Key + 8);
        m_State[4] = ToU32(Key + 12);
        const uint8_t* Constants = (KeyLen == 32) ? c_sigma : c_tau;
        const size_t KeyIndex = KeyLen - 16;
        m_State[11] = ToU32(Key + KeyIndex + 0);
        m_State[12] = ToU32(Key + KeyIndex + 4);
        m_State[13] = ToU32(Key + KeyIndex + 8);
        m_State[14] = ToU32(Key + KeyIndex + 12);
        m_State[0]  = ToU32(Constants + 0);
        m_State[5]  = ToU32(Constants + 4);
        m_State[10] = ToU32(Constants + 8);
        m_State[15] = ToU32(Constants + 12);
        m_State[6]  = ToU32(Iv + 0);
        m_State[7]  = ToU32(Iv + 4);
        m_State[8]  = 0;
        m_State[9]  = 0;
    }

    void Salsa20::Hash(uint8_t Output[64])
    {
        uint32_t s[16];
        std::memcpy(s, m_State, sizeof(s));
        for (int round = m_Rounds; round > 0; round -= 2)
        {
            s[4]  ^= Rotate(s[0]  + s[12], 7);  s[8]  ^= Rotate(s[4]  + s[0],  9);
            s[12] ^= Rotate(s[8]  + s[4],  13); s[0]  ^= Rotate(s[12] + s[8],  18);
            s[9]  ^= Rotate(s[5]  + s[1],  7);  s[13] ^= Rotate(s[9]  + s[5],  9);
            s[1]  ^= Rotate(s[13] + s[9],  13); s[5]  ^= Rotate(s[1]  + s[13], 18);
            s[14] ^= Rotate(s[10] + s[6],  7);  s[2]  ^= Rotate(s[14] + s[10], 9);
            s[6]  ^= Rotate(s[2]  + s[14], 13); s[10] ^= Rotate(s[6]  + s[2],  18);
            s[3]  ^= Rotate(s[15] + s[11], 7);  s[7]  ^= Rotate(s[3]  + s[15], 9);
            s[11] ^= Rotate(s[7]  + s[3],  13); s[15] ^= Rotate(s[11] + s[7],  18);
            s[1]  ^= Rotate(s[0]  + s[3],  7);  s[2]  ^= Rotate(s[1]  + s[0],  9);
            s[3]  ^= Rotate(s[2]  + s[1],  13); s[0]  ^= Rotate(s[3]  + s[2],  18);
            s[6]  ^= Rotate(s[5]  + s[4],  7);  s[7]  ^= Rotate(s[6]  + s[5],  9);
            s[4]  ^= Rotate(s[7]  + s[6],  13); s[5]  ^= Rotate(s[4]  + s[7],  18);
            s[11] ^= Rotate(s[10] + s[9],  7);  s[8]  ^= Rotate(s[11] + s[10], 9);
            s[9]  ^= Rotate(s[8]  + s[11], 13); s[10] ^= Rotate(s[9]  + s[8],  18);
            s[12] ^= Rotate(s[15] + s[14], 7);  s[13] ^= Rotate(s[12] + s[15], 9);
            s[14] ^= Rotate(s[13] + s[12], 13); s[15] ^= Rotate(s[14] + s[13], 18);
        }
        for (int i = 0; i < 16; i++) ToBytes(s[i] + m_State[i], Output + 4 * i);
    }

    void Salsa20::Transform(const uint8_t* Input, size_t Count, uint8_t* Output)
    {
        uint8_t Block[64];
        size_t In = 0, Out = 0;
        while ((int64_t)Count > 0)
        {
            Hash(Block);
            m_State[8] += 1;
            if (m_State[8] == 0) m_State[9] += 1;
            const size_t BlockSize = Count < 64 ? Count : 64;
            for (size_t i = 0; i < BlockSize; i++)
                Output[Out + i] = (uint8_t)(Input[In + i] ^ Block[i]);
            In += 64; Out += 64;
            Count -= 64;
        }
    }

    bool CerberusCryptoSelfTest(std::string& Report)
    {
        char Buf[128];
        bool Ok = true;

        // CRC32("123456789") == 0xCBF43926 (classic check value).
        const char* Check = "123456789";
        uint32_t Crc = Crc32::Compute((const uint8_t*)Check, 9);
        bool CrcOk = (Crc == 0xCBF43926u);
        std::snprintf(Buf, sizeof(Buf), "CRC32 \"123456789\" = 0x%08X (expect 0xCBF43926) -> %s\n", Crc, CrcOk ? "OK" : "FAIL");
        Report += Buf; Ok &= CrcOk;

        // Salsa20/20, eSTREAM Set 1 vector 0: 256-bit key = 80 00..00, IV = 00..00.
        // Keystream[0..63] known-answer.
        uint8_t Key[32] = { 0x80 };
        uint8_t Iv[8] = { 0 };
        uint8_t Zero[64] = { 0 };
        uint8_t Ks[64];
        Salsa20 S(Key, 32, Iv, 20);
        S.Transform(Zero, 64, Ks);

        static const uint8_t Expected[64] = {
            0xE3,0xBE,0x8F,0xDD,0x8B,0xEC,0xA2,0xE3,0xEA,0x8E,0xF9,0x47,0x5B,0x29,0xA6,0xE7,
            0x00,0x39,0x51,0xE1,0x09,0x7A,0x5C,0x38,0xD2,0x3B,0x7A,0x5F,0xAD,0x9F,0x68,0x44,
            0xB2,0x2C,0x97,0x55,0x9E,0x27,0x23,0xC7,0xCB,0xBD,0x3F,0xE4,0xFC,0x8D,0x9A,0x07,
            0x44,0x65,0x2A,0x83,0xE7,0x2A,0x9C,0x46,0x18,0x76,0xAF,0x4D,0x7E,0xF1,0xA1,0x17,
        };
        bool SalsaOk = (std::memcmp(Ks, Expected, 64) == 0);
        Report += "Salsa20/20 eSTREAM Set1 Vec0 keystream -> ";
        Report += SalsaOk ? "OK\n" : "FAIL\n";
        Ok &= SalsaOk;

        return Ok;
    }
}
