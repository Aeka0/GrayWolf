#include "stdafx.h"

// ZonekitZone's script bundles kind (ZonekitZone.h, ExportsScriptBundles): each scriptbundle asset
// of a zone as JSON, its key/value tree walked as the game's loader lays it out.

#include "ZonekitZone.h"

#include "ExportNaming.h"
#include "ExportWriters.h"

#include <nlohmann/json.hpp>

#include <format>
#include <functional>
#include <set>

namespace fs = std::filesystem;

namespace
{
    using Json = nlohmann::ordered_json;
    using V = BundleValue;

    const char* KindName(V K)
    {
        switch (K)
        {
        case V::Bool: return "bool";
        case V::Int: return "int";
        case V::Float: return "float";
        case V::Localize: return "localize";
        case V::String: return "string";
        case V::Struct: return "struct";
        case V::KeyedStruct: return "keyed_struct";
        case V::Array: return "array";
        case V::Anim: return "anim";
        case V::Hash: return "hash";
        case V::Asset: return "asset";
        default: return "raw";
        }
    }

    // a float as its shortest text, read back as the double that prints the same (0.1f -> 0.1)
    double Shortest(float F)
    {
        const std::string S = std::format("{}", F);
        try
        {
            return std::stod(S);
        }
        catch (...)
        {
            return (double)F;
        }
    }
}

void ZonekitZone::ExportsScriptBundles(const ScriptBundleLayout& L, std::function<std::string(const std::string&)> Decrypt)
{
    using DiskGame::Kind;
    Exports(Kind::ScriptBundles, [this, L, Decrypt](const DiskGame::Context& C, DiskGame::Totals& T) {
        auto& Kt = T.Kinds[Kind::ScriptBundles];
        constexpr uint64_t IdBits = 0x7FFFFFFFFFFFFFFFull;
        const auto& Img = Image();
        const auto In = [&](int64_t P, int64_t N) { return P >= 0 && P + N <= (int64_t)Img.size(); };
        const auto Hex = [](uint64_t H) { return std::format("{:016x}", H); };
        const auto Named = [this, &Hex](uint64_t H) {
            const std::string N = KnownHash(H);
            return N.empty() ? Hex(H) : N;
        };
        const auto KindOf = [&L](uint8_t Code) { return Code < L.Types.size() ? L.Types[Code] : V::Raw; };
        size_t Fields = 0, KeysNamed = 0;
        // a string record (the text's pointer first), through the game's cipher when it has one
        const auto Text = [&](int64_t Record) {
            if (!In(Record, 8)) return std::string();
            std::string S = TextAt(Record);
            return Decrypt && !S.empty() ? Decrypt(S) : S;
        };
        std::function<Json(int64_t, int)> Table;
        // the value of Kind in the 8 bytes at Slot (an entry's value, an array's element)
        std::function<Json(V, int64_t, int, Json&)> Value = [&](V K, int64_t Slot, int Depth, Json& Field) -> Json {
            const uint64_t U = At<uint64_t>(Img, Slot);
            switch (K)
            {
            case V::Bool: return At<uint32_t>(Img, Slot) != 0;
            case V::Int: return At<int32_t>(Img, Slot);
            case V::Float: return Shortest(At<float>(Img, Slot));
            case V::Localize:
            case V::Hash:
            case V::Asset:
                if (U == 0) return nullptr;
                Field["value_hash"] = Hex(U & (K == V::Asset ? IdBits : ~0ull));
                return Named(K == V::Asset ? U & IdBits : U);
            case V::String: return U == 0 ? Json(nullptr) : Json(Text(Off(U)));
            case V::Anim:
            {
                // {its name's string record, ...}
                const int64_t To = Off(U);
                return To < 0 ? Json(nullptr) : Json(Text(Off(PointerAt(To))));
            }
            case V::Struct:
            case V::KeyedStruct:
            {
                const int64_t To = Off(U);
                return To < 0 || Depth > 32 ? Json::array() : Table(To, Depth + 1);
            }
            case V::Array:
            {
                // its elements, of one type (8 bytes each) or typed one by one (16-byte entries); the
                // lookup index the game builds over them is left out
                const int64_t To = Off(U);
                Json Items = Json::array();
                if (To < 0 || Depth > 32) return Items;
                const uint32_t N = At<uint32_t>(Img, To + L.ArrayCount);
                const int64_t Elems = Off(PointerAt(To + L.ArrayElements));
                const bool Read = N != 0 && Elems >= 0 && In(Elems, (int64_t)N * L.ArrayStride);
                if (L.ArrayType < 0 && !Read) return Items;
                const V Elem = KindOf(At<uint8_t>(Img, L.ArrayType >= 0 ? To + L.ArrayType : Elems));
                Field["element_type"] = KindName(Elem);
                if (!Read) return Items;
                for (uint32_t J = 0; J < N; J++)
                {
                    const int64_t E = Elems + (int64_t)J * L.ArrayStride;
                    Json Ignored;
                    if (L.ArrayType >= 0) Items.push_back(Value(Elem, E, Depth + 1, Ignored));
                    else Items.push_back(Value(KindOf(At<uint8_t>(Img, E)), E + 8, Depth + 1, Ignored));
                }
                return Items;
            }
            default: return Hex(U);
            }
        };
        // a key/value table: u32 count at Count, the keys' FNV-1a 64 (sorted) at Keys, the script
        // identifier hashes at ScriptHashes, the entries {u8 type, value at 8} at Entries
        Table = [&](int64_t P, int Depth) -> Json {
            Json Out = Json::array();
            const uint32_t Count = At<uint32_t>(Img, P + L.Count);
            const int64_t Keys = Off(PointerAt(P + L.Keys)), Script = Off(PointerAt(P + L.ScriptHashes)), Entries = Off(PointerAt(P + L.Entries));
            if (Count == 0 || Keys < 0 || Entries < 0 || !In(Keys, (int64_t)Count * 8) || !In(Entries, (int64_t)Count * 0x10)) return Out;
            for (uint32_t K = 0; K < Count; K++)
            {
                const uint64_t Key = At<uint64_t>(Img, Keys + (int64_t)K * 8);
                const uint64_t ScriptHash = Script >= 0 && In(Script, (int64_t)Count * 8) ? At<uint64_t>(Img, Script + (int64_t)K * 8) : 0;
                const int64_t E = Entries + (int64_t)K * 0x10;
                const uint8_t Code = At<uint8_t>(Img, E);
                const V Kd = KindOf(Code);
                const std::string KeyName = KnownHash(Key);
                Fields++;
                if (!KeyName.empty()) KeysNamed++;
                Json F;
                F["key"] = KeyName.empty() ? Hex(Key) : KeyName;
                F["key_hash"] = Hex(Key);
                if (ScriptHash != 0) F["script_hash"] = std::format("{:x}", ScriptHash);
                F["type"] = Kd == V::Raw ? std::format("raw{}", Code) : KindName(Kd);
                Json Extra = Json::object();
                Json Val = Value(Kd, E + 8, Depth, Extra);
                if (Extra.contains("element_type")) F["element_type"] = Extra["element_type"];
                F["value"] = std::move(Val);
                if (Extra.contains("value_hash")) F["value_hash"] = Extra["value_hash"];
                Out.push_back(std::move(F));
            }
            return Out;
        };
        const fs::path Dir = fs::path(C.KindDir(Kind::ScriptBundles)) / ExportNaming::SafeFileName(ZoneName);
        std::set<uint64_t> Done;
        for (const auto& I : R.Instances)
        {
            if (I.Parent >= 0 || !OwnerIs(I, "scriptbundle")) continue;
            const uint64_t Id = At<uint64_t>(Img, I.Pos) & IdBits;
            const int64_t Tab = I.Pos + L.Table;
            // a bundle the zone only names keeps no table
            if (Off(PointerAt(Tab + L.Entries)) < 0 && At<uint32_t>(Img, Tab + L.Count) != 0)
            {
                Kt.References++;
                continue;
            }
            if (!Done.insert(Id).second) continue;
            const std::string Name = KnownHash(Id);
            if (!C.Wants(Kind::ScriptBundles, Name.empty() ? Hex(Id) : Name)) continue;
            Kt.Seen++;
            Json B;
            B["id"] = Hex(Id);
            B["name"] = Name;
            if (L.TypeHash >= 0)
                if (const uint64_t Ty = At<uint64_t>(Img, I.Pos + L.TypeHash) & IdBits; Ty != 0) B["type"] = Named(Ty);
            B["fields"] = Table(Tab, 0);
            const std::string S = B.dump(1, '\t', false, nlohmann::json::error_handler_t::replace) + "\n";
            const fs::path File = Dir / (ExportNaming::SafeFileName(Name.empty() ? Hex(Id) : Name) + ".json");
            if (ExportWriters::WriteBytes(File, (const uint8_t*)S.data(), S.size())) Kt.Written++;
            else T.Fail(ZoneName, Kind::ScriptBundles, Name.empty() ? Hex(Id) : Name, "write failed");
        }
        Counter("bundle_fields") += Fields;
        Counter("bundle_keys_named") += KeysNamed;
    });
}
