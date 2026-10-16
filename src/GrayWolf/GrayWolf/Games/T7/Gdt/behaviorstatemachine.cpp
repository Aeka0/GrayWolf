#include "stdafx.h"

// BO3 behaviorstatemachine (79) -> its .ai_bsm file (JSON), as HydraX's BehaviorStateMachine.cs and
// BehaviorStateMachineObj.cs (github.com/Scobalula/HydraX, GPL-3.0): header {name 0, states 8, count
// 0x10}; a state (0x30) {script strings: name 0, ASM state 4, entry point condition 8, start (or
// planner) function 0xc, update 0x10, terminate 0x14, action notify 0x18; connections 0x20, count
// 0x28, flags 0x2c}; a connection (0x10) {to state index 0, condition function 4, flags 8, minimum
// running time 0xc}. Written in type.jsonc's "files" folder: {"states": [...]}, empty strings, zeros
// and false flags left out. No GDT entry: the Mod Tools' share/raw/behavior holds these files.
// HydraX gives "wildcard" and "plannerstate" the same bit (0x1) and never writes a connection's
// conditionFunction; both kept as HydraX writes them, the condition function written.

#include "Gdt.h"
#include "HydraText.h"

#include <algorithm>

namespace
{
    using HydraText::Json;

    bool Convert(const Gdt::Type& T, const Gdt::Memory& M, int64_t Pos, const std::string& Name, std::vector<Gdt::Block>&, Gdt::Stats& S)
    {
        const auto Str = [&](int64_t P) { return M.ScriptString(M.At<uint32_t>(P)); };
        const auto Text = [](Json& J, const char* Key, const std::string& V) {
            if (!V.empty()) J.Set(Key, Json::Str(V));
        };
        const auto Flag = [](Json& J, const char* Key, bool V) {
            if (V) J.Set(Key, Json::Bool(true));
        };
        const int64_t States = M.Follow(Pos + 8);
        const int32_t Count = std::clamp(M.At<int32_t>(Pos + 0x10), 0, 1 << 16);
        Json All = Json::Array();
        for (int32_t I = 0; States >= 0 && I < Count; I++)
        {
            const int64_t P = States + 0x30 * (int64_t)I;
            const uint32_t Flags = M.At<uint32_t>(P + 0x2c);
            const bool Planner = Flags & 0x1;
            Json J;
            Text(J, "name", Str(P));
            Text(J, "asmStateName", Str(P + 4));
            Text(J, "entryPointCondition", Str(P + 8));
            Text(J, "startFunction", Planner ? std::string() : Str(P + 0xc));
            Text(J, "updateFunction", Str(P + 0x10));
            Text(J, "terminateFunction", Str(P + 0x14));
            Text(J, "plannerFunction", Planner ? Str(P + 0xc) : std::string());
            Text(J, "actionNotify", Str(P + 0x18));
            Flag(J, "wildcard", Flags & 0x1);
            Flag(J, "evaluateStateForASM", Flags & 0x4);
            Flag(J, "plannerstate", Flags & 0x1);
            Flag(J, "terminalState", Flags & 0x8);
            Flag(J, "entryState", Flags & 0x10);
            if (Flags & ~0x1du) S.Counters["behaviorstatemachine_flags_" + std::to_string(Flags & ~0x1du)]++;
            const int32_t N = std::clamp(M.At<int32_t>(P + 0x28), 0, 65536);
            const int64_t Connections = M.Follow(P + 0x20);
            if (N > 0)
            {
                Json Cs = Json::Array();
                for (int32_t K = 0; K < N; K++)
                {
                    const int64_t C = Connections < 0 ? -1 : Connections + 0x10 * (int64_t)K;
                    Json Cn;
                    const int32_t To = C < 0 ? -1 : M.At<int32_t>(C);
                    if (C >= 0 && To >= 0 && To < Count) Text(Cn, "toState", Str(States + 0x30 * (int64_t)To));
                    else S.Counters["behaviorstatemachine_bad_state"]++;
                    Text(Cn, "conditionFunction", C < 0 ? std::string() : Str(C + 4));
                    const uint32_t F = C < 0 ? 0 : M.At<uint32_t>(C + 8);
                    if (const int32_t Min = M.At<int32_t>(C + 0xc); C >= 0 && Min != 0) Cn.Set("minRunningStateTime", Json::Int(Min));
                    Flag(Cn, "waitTillStateFinish", F & 0x2);
                    Flag(Cn, "evaluateOnStateFailure", F & 0x4);
                    Flag(Cn, "evaluateDuringTransition", F & 0x8);
                    Cs.Push(std::move(Cn));
                }
                J.Set("connections", std::move(Cs));
            }
            All.Push(std::move(J));
        }
        Json File;
        File.Set("states", std::move(All));
        S.Files.emplace_back(T.Settings().value("files", std::string()) + Name, HydraText::Dump(File));
        return true;
    }
}

GDT_CONVERTER(t7_behaviorstatemachine, Convert);
