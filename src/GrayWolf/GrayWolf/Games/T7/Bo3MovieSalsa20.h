#pragma once

#include <cstdint>

// Standard (unmodified) Salsa20 stream cipher, 20 rounds, 256-bit key: BO3's cinematic-file cipher.
namespace Salsa20
{
    // XORs Data in place with the Salsa20 keystream (encrypt and decrypt are the same operation for
    // a stream cipher). Starts at keystream block 0 (byte offset 0 of the stream) - matches how BO3
    // decrypts a movie file from its start. Key must point to 32 bytes, Nonce to 8 bytes.
    void CryptInPlace(uint8_t* Data, size_t Size, const uint8_t Key[32], const uint8_t Nonce[8]);

    // Checks this implementation against an independent Salsa20 test vector. True iff byte-exact.
    bool SelfTest();
}
