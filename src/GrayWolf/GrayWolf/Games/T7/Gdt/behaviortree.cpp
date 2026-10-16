#include "stdafx.h"

// BO3 behaviortree (78) -> its .ai_bt file (JSON), as HydraX's BehaviorTree.cs and BehaviorTreeObj.cs
// (github.com/Scobalula/HydraX, GPL-3.0): header {name 0, nodes 8, count 0x10}; a node (0x48)
// {name script string 0, type 4, parent index 0xc (-1: the root), child indices 0x10, count 0x18,
// 9 script strings 0x1c, 8 bytes of data 0x40}. What a node writes depends on its type (HydraX's
// table): an action its ASM state, action name, notify, start/update/terminate functions (strings
// 3-8) and looping/max time (data ints); a condition its script function, interrupt name (strings
// 6, 7) and cooldowns; a probability selector or random decorator its percent chance (data float).
// Written in type.jsonc's "files" folder from the root node down, empty strings and zeros left out.
// No GDT entry: the Mod Tools' share/raw/behavior holds these files.

#include "Gdt.h"
#include "HydraText.h"

#include <algorithm>

namespace
{
    using HydraText::Json;

    const char* const Types[] = { "action", "condition_blackboard", "condition_script", "condition_script_negate",
                                  "condition_service_script", "decorator_random", "decorator_script", "decorator_timer",
                                  "parallel", "sequence", "selector", "probability_selector", "behavior_state_machine",
                                  "link_node" };

    struct Tree
    {
        const Gdt::Memory& M;
        int64_t Nodes = -1;
        int32_t Count = 0;
        Gdt::Stats& S;

        Json Node(int32_t I, int Depth) const
        {
            const int64_t P = Nodes + 0x48 * (int64_t)I;
            const auto Str = [&](int K) { return M.ScriptString(M.At<uint32_t>(P + 0x1c + 4 * (int64_t)K)); };
            const auto Text = [](Json& J, const char* Key, const std::string& V) {
                if (!V.empty()) J.Set(Key, Json::Str(V));
            };
            const auto Int = [](Json& J, const char* Key, int32_t V) {
                if (V != 0) J.Set(Key, Json::Int(V));
            };
            Json J;
            Text(J, "id", M.ScriptString(M.At<uint32_t>(P)));
            const int32_t Type = M.At<int32_t>(P + 4);
            std::string T;
            if (Type >= 0 && Type < (int32_t)std::size(Types)) T = Types[Type];
            else
            {
                S.Counters["behaviortree_unknown_type"]++;
                T = "type_" + std::to_string(Type);
            }
            Text(J, "type", T);
            const int32_t D0 = M.At<int32_t>(P + 0x40), D1 = M.At<int32_t>(P + 0x44);
            if (T == "action" || T == "behavior_state_machine")
            {
                Text(J, "ActionName", Str(4));
                Text(J, "ASMStateName", Str(3));
                Text(J, "actionNotify", Str(5));
                Text(J, "StartFunction", Str(6));
                Text(J, "TerminateFunction", Str(8));
                Text(J, "UpdateFunction", Str(7));
                Int(J, "loopingAction", D0);
                Int(J, "actionTimeMax", D1);
            }
            else if (T == "condition_script" || T == "condition_blackboard" || T == "condition_script_negate" ||
                     T == "condition_service_script")
            {
                Text(J, "scriptFunction", Str(6));
                Text(J, "interruptName", Str(7));
                Int(J, "cooldownMin", D0);
                Int(J, "cooldownMax", D1);
            }
            else if (T == "probability_selector" || T == "decorator_random")
            {
                const float Chance = M.At<float>(P + 0x40);
                if (Chance != 0) J.Set("percentChance", Json::Raw(HydraText::Single(Chance)));
            }
            const int32_t Children = std::clamp(M.At<int32_t>(P + 0x18), 0, 65536);
            const int64_t Indices = M.Follow(P + 0x10);
            if (Children > 0)
            {
                Json All = Json::Array();
                for (int32_t K = 0; K < Children; K++)
                {
                    const int32_t C = Indices < 0 ? -1 : M.At<int32_t>(Indices + 4 * (int64_t)K);
                    if (C < 0 || C >= Count || Depth > 256)
                    {
                        S.Counters["behaviortree_bad_child"]++;
                        All.Push(Json::Raw("null"));   // HydraX's children array keeps the slot
                        continue;
                    }
                    All.Push(Node(C, Depth + 1));
                }
                J.Set("children", std::move(All));
            }
            return J;
        }
    };

    bool Convert(const Gdt::Type& T, const Gdt::Memory& M, int64_t Pos, const std::string& Name, std::vector<Gdt::Block>&, Gdt::Stats& S)
    {
        Tree Tr{ M, M.Follow(Pos + 8), std::clamp(M.At<int32_t>(Pos + 0x10), 0, 1 << 16), S };
        int32_t Root = -1;
        for (int32_t I = 0; Tr.Nodes >= 0 && I < Tr.Count; I++)
            if (M.At<int32_t>(Tr.Nodes + 0x48 * (int64_t)I + 0xc) == -1) Root = I;   // the last, as HydraX
        if (Root < 0)
        {
            S.Counters["behaviortree_no_root"]++;
            return false;
        }
        S.Files.emplace_back(T.Settings().value("files", std::string()) + Name, HydraText::Dump(Tr.Node(Root, 0)));
        return true;
    }
}

GDT_CONVERTER(t7_behaviortree, Convert);
