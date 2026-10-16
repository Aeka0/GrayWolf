#pragma once
#include <vector>
#include <stdint.h>
#include <string>
#include <memory>
#include <functional>

// The pool and string addresses a dumped game's asset pools are walked from, read by the live
// readers of MW2019, MWII, MWIII, Vanguard and MW2CR (GameModernWarfare4..6, GameVanguard,
// GameModernWarfare2RM). Nothing sets state any more: those readers wait for an offline bring-up.
namespace ps
{
    class State
    {
    public:
        std::string GameDirectory;
        uint64_t PoolsAddress;
        uint64_t StringsAddress;
        std::vector<std::string> Flags;

        State();

        void AddFlag(const std::string& flag);
        bool HasFlag(const std::string& flag);
    };

    struct XAsset64
    {
        uint64_t Header;
        // Whether or not this asset is a temp slot.
        uint64_t Temp;
        uint64_t Next;
        uint64_t Previous;
    };

    struct XAssetPool64
    {
        // The start of the asset chain.
        uint64_t Root;
        // The end of the asset chain.
        uint64_t End;
        // The asset hash table for this pool.
        uint64_t LookupTable;
        // Storage for asset headers for this pool.
        uint64_t HeaderMemory;
        // Storage for asset entries for this pool.
        uint64_t AssetMemory;
    };

    extern std::unique_ptr<State> state;

    void PoolParser64(uint64_t offset, std::function<XAsset64(const uint64_t&)> request, std::function<void(XAsset64&)> callback);
};

