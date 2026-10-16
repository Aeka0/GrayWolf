#include "stdafx.h"

// BO3 animstatemachine (77) -> its .ai_asm (JSON), as HydraX's AnimationStateMachine.cs and
// AnimationStateMachineObj.cs (github.com/Scobalula/HydraX, GPL-3.0): header {name 0, root states 8,
// count 0x10, substates 0x18, count 0x20, transitions 0x28, count 0x30}; a root state (0x18) {name 0,
// substate indices 8, count 0x10}; a substate (0x40) and a transition (0x30) {name 0, path 4,
// requirements 8, flags 0xc, selectors: substate 0x14.., transition 0x1c..}; a substate's transition
// indices 0x30, count 0x38. Written in "files": {"states": {root: {"substates": {...}}}}, empty
// strings and false flags left out. HydraX reads a transition's requirements at 4 (its path) and
// flags at 8: here as a substate's, without the 0x1 every transition holds (zm_factory's two: no
// flag in the Mod Tools' file). No GDT entry: the Mod Tools' share/raw/animstatemachines.

#include "Gdt.h"
#include "HydraText.h"

#include <algorithm>

namespace
{
    using HydraText::Json;

    // a state's keys in AnimationStateMachineObj.StateObj's order; Sub: a substate (it has a
    // transition decorator and a transdec layer function)
    Json State(const Gdt::Memory& M, int64_t P, bool Sub)
    {
        const auto Str = [&](int64_t At) { return M.ScriptString(M.At<uint32_t>(P + At)); };
        const uint32_t Req = M.At<uint32_t>(P + 8), Flags = M.At<uint32_t>(P + 0xc) & (Sub ? ~0u : ~1u);
        const int64_t A = Sub ? 0x14 : 0x1c;   // the animation selector; aim, shoot follow
        Json J;
        const auto Text = [&](const char* Key, const std::string& V) {
            if (!V.empty()) J.Set(Key, Json::Str(V));
        };
        const auto Flag = [&](const char* Key, bool V) {
            if (V) J.Set(Key, Json::Bool(true));
        };
        Text("animation_selector", Str(A));
        if (Sub) Text("transition_decorator", Str(0x20));
        Text("aim_selector", Str(A + 4));
        Text("shoot_selector", Str(A + 8));
        Text("delta_layer_function", Str(Sub ? 0x24 : 0x28));
        if (Sub) Text("transdec_layer_function", Str(0x28));
        Text("asm_client_notify", Str(0x2c));
        Flag("loopsync", Flags & 0x2);
        Flag("cleanloop", Flags & 0x80);
        Flag("multipledelta", Flags & 0x4);
        Flag("terminal", Flags & 0x1);
        Flag("parametric2d", Flags & 0x8);
        Flag("animdrivenlocomotion", Flags & 0x100);
        Flag("coderate", Flags & 0x10);
        Flag("speedblend", Flags & 0x200);
        Flag("allow_transdec_aim", Flags & 0x20);
        Flag("force_fire", Flags & 0x40);
        Flag("requires_ragdoll_notetrack", Req & 0x2);
        Flag("delta_requires_translation", Req & 0x1);
        return J;
    }

    bool Convert(const Gdt::Type& T, const Gdt::Memory& M, int64_t Pos, const std::string& Name, std::vector<Gdt::Block>&, Gdt::Stats& S)
    {
        const auto Str = [&](int64_t P) { return M.ScriptString(M.At<uint32_t>(P)); };
        const auto Count = [&](int64_t At) { return std::clamp(M.At<int32_t>(Pos + At), 0, 1 << 16); };

        const int64_t Transitions = M.Follow(Pos + 0x28);
        std::vector<std::pair<std::string, Json>> Trans;
        for (int32_t I = 0; Transitions >= 0 && I < Count(0x30); I++)
        {
            const int64_t P = Transitions + 0x30 * (int64_t)I;
            Trans.emplace_back(Str(P), State(M, P, false));
        }

        const int64_t Subs = M.Follow(Pos + 0x18);
        std::vector<std::pair<std::string, Json>> Sub;
        for (int32_t I = 0; Subs >= 0 && I < Count(0x20); I++)
        {
            const int64_t P = Subs + 0x40 * (int64_t)I;
            Json J = State(M, P, true);
            const int32_t N = M.At<int32_t>(P + 0x38);
            if (N > 0)
            {
                Json All = Json::Object();
                const int64_t Indices = M.Follow(P + 0x30);
                for (int32_t K = 0; Indices >= 0 && K < N && K < 65536; K++)
                {
                    const int32_t X = M.At<int32_t>(Indices + 4 * (int64_t)K);
                    if (X >= 0 && (size_t)X < Trans.size()) All.Set(Trans[(size_t)X].first, Trans[(size_t)X].second);
                    else S.Counters["animstatemachine_bad_index"]++;
                }
                J.Set("transitions", std::move(All));
            }
            Sub.emplace_back(Str(P), std::move(J));
        }

        Json Roots = Json::Object();
        const int64_t RootList = M.Follow(Pos + 8);
        for (int32_t I = 0; RootList >= 0 && I < Count(0x10); I++)
        {
            const int64_t P = RootList + 0x18 * (int64_t)I;
            Json Root;
            if (M.At<uint64_t>(P + 8) != 0)   // HydraX: a substates object when the pointer is set
            {
                Json All = Json::Object();
                const int64_t Indices = M.Follow(P + 8);
                const int32_t N = M.At<int32_t>(P + 0x10);
                for (int32_t K = 0; Indices >= 0 && K < N && K < 65536; K++)
                {
                    const int32_t X = M.At<int32_t>(Indices + 4 * (int64_t)K);
                    if (X >= 0 && (size_t)X < Sub.size()) All.Set(Sub[(size_t)X].first, Sub[(size_t)X].second);
                    else S.Counters["animstatemachine_bad_index"]++;
                }
                Root.Set("substates", std::move(All));
            }
            Roots.Set(Str(P), std::move(Root));
        }
        Json File;
        File.Set("states", std::move(Roots));
        S.Files.emplace_back(T.Settings().value("files", std::string()) + Name, HydraText::Dump(File));
        return true;
    }
}

GDT_CONVERTER(t7_animstatemachine, Convert);
