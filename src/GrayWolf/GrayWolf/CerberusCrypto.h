#pragma once

#include <cstdint>
#include <string>

// Cerberus GSC/CSC script crypto, ported from Cerberus.Logic (CRC32.cs + Salsa20.cs). Black Ops 3
// script assets (scriptparsetree) are Salsa20-encrypted and CRC32-checked; these are the primitives
// the decompiler needs before anything else. Both are standard algorithms, so the port is verified
// byte-for-byte against published test vectors (see CerberusCryptoSelfTest).
namespace Cerberus
{
    // Standard (zlib) CRC32, matching Cerberus.Logic.CRC32. Table generated from polynomial
    // 0xEDB88320 (identical to the hardcoded C# table). Value() returns the finalised checksum.
    class Crc32
    {
    public:
        Crc32();
        void Update(uint8_t Byte);
        void Update(const uint8_t* Data, size_t Size);
        uint32_t Value() const { return ~m_Value; }
        static uint32_t Compute(const uint8_t* Data, size_t Size);

    private:
        uint32_t m_Value;
    };

    // Salsa20 stream cipher (Bernstein), ported from Cerberus.Logic.Salsa20 with the same state
    // layout, round count, and little-endian word order. Encryption == decryption (XOR keystream).
    class Salsa20
    {
    public:
        // key must be 16 or 32 bytes, iv 8 bytes; rounds defaults to 20 (Salsa20/20).
        Salsa20(const uint8_t* Key, size_t KeyLen, const uint8_t* Iv, int Rounds = 20);

        // XORs the keystream over Input into Output (same length). Processes in 64-byte blocks,
        // advancing the 64-bit block counter exactly as the reference does.
        void Transform(const uint8_t* Input, size_t Count, uint8_t* Output);

    private:
        void Hash(uint8_t Output[64]);
        uint32_t m_State[16];
        int m_Rounds;
    };

    // Runs both primitives against known-answer vectors. Returns true iff both match exactly.
    bool CerberusCryptoSelfTest(std::string& Report);
}
