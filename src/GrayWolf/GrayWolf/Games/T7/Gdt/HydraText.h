#pragma once

// The text HydraX (github.com/Scobalula/HydraX, GPL-3.0) writes its side files in, for the BO3 gdt
// converters that write files beside the .gdt (animtables, behavior, animstatemachines, xcams,
// fonticons): .NET Framework numbers and Newtonsoft.Json's indented JSON (4 spaces, CRLF, no final
// newline), so an export compares byte for byte with HydraX's.

#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <utility>
#include <vector>

namespace HydraText
{
    // X's "G" text with P significant digits as the .NET Framework writes it: the digits rounded half
    // up (C rounds the exact value half to even: 466.247589111328125 is ...813 there, ...812 here),
    // trailing zeros dropped, fixed notation for exponents -5 < e < P, else d.ddE+XX
    inline std::string NetG(double X, int P)
    {
        if (std::isnan(X)) return "NaN";
        if (std::isinf(X)) return X > 0 ? "Infinity" : "-Infinity";
        if (X == 0) return "0";   // the Framework writes -0 as 0
        char B[64];
        std::snprintf(B, sizeof(B), "%.30E", std::fabs(X));
        std::string D;   // the mantissa's digits
        const char* C = B;
        for (; *C && *C != 'E'; C++)
            if (*C >= '0' && *C <= '9') D += *C;
        int E = std::atoi(C + 1);
        const bool Up = (int)D.size() > P && D[(size_t)P] >= '5';
        D.resize((size_t)P);
        if (Up)
        {
            int I = P - 1;
            for (; I >= 0 && D[(size_t)I] == '9'; I--) D[(size_t)I] = '0';
            if (I >= 0) D[(size_t)I]++;
            else
            {
                D.insert(D.begin(), '1');
                D.pop_back();
                E++;
            }
        }
        while (D.size() > 1 && D.back() == '0') D.pop_back();
        std::string S = X < 0 ? "-" : "";
        if (E > -5 && E < P)
        {
            if (E < 0) S += "0." + std::string((size_t)(-E - 1), '0') + D;
            else
            {
                if ((int)D.size() <= E) D += std::string((size_t)(E + 1) - D.size(), '0');
                S += D.substr(0, (size_t)E + 1);
                if ((int)D.size() > E + 1) S += "." + D.substr((size_t)E + 1);
            }
            return S;
        }
        S += D.substr(0, 1);
        if (D.size() > 1) S += "." + D.substr(1);
        char Ex[16];
        std::snprintf(Ex, sizeof(Ex), "E%c%02d", E < 0 ? '-' : '+', E < 0 ? -E : E);
        return S + Ex;
    }

    // a ".0" on a number with neither point nor exponent, as Newtonsoft writes a float or double
    inline std::string Decimal(std::string S)
    {
        if (S.find_first_of(".EN") == std::string::npos) S += ".0";
        return S;
    }

    // float.ToString() of the .NET Framework: "G", 7 significant digits
    inline std::string Float(float X) { return NetG(X, 7); }

    // double.ToString("R") of the .NET Framework, as Newtonsoft writes a double: 15 digits when they
    // read back the same, else 17
    inline std::string Double(double X)
    {
        std::string S = NetG(X, 15);
        if (std::isfinite(X) && std::strtod(S.c_str(), nullptr) != X) S = NetG(X, 17);
        return Decimal(S);
    }

    // float.ToString("R") of the .NET Framework, as Newtonsoft writes a float: 7 digits, else 9
    inline std::string Single(float X)
    {
        std::string S = NetG(X, 7);
        if (std::isfinite(X) && std::strtof(S.c_str(), nullptr) != X) S = NetG(X, 9);
        return Decimal(S);
    }

    // Math.Round(X, 4) of the .NET Framework: scaled, rounded half to even, scaled back
    inline double Round4(double X)
    {
        if (std::fabs(X) >= 1e16) return X;
        return std::nearbyint(X * 10000.0) / 10000.0;
    }

    // A JSON value as Newtonsoft writes it; an object keeps its keys' order, a key set twice keeps
    // its first place (a Dictionary's).
    struct Json
    {
        enum class Kind { Raw, String, Array, Object } K = Kind::Object;
        std::string Text;   // Raw: the number or literal; String: the text
        std::vector<Json> Items;
        std::vector<std::pair<std::string, Json>> Keys;

        static Json Raw(std::string T) { Json J; J.K = Kind::Raw; J.Text = std::move(T); return J; }
        static Json Str(std::string T) { Json J; J.K = Kind::String; J.Text = std::move(T); return J; }
        static Json Int(int64_t V) { return Raw(std::to_string(V)); }
        static Json Num(double V) { return Raw(Double(V)); }
        static Json Bool(bool V) { return Raw(V ? "true" : "false"); }
        static Json Array() { Json J; J.K = Kind::Array; return J; }
        static Json Object() { return Json(); }

        Json& Set(const std::string& Key, Json V)
        {
            for (auto& [Name, Old] : Keys)
                if (Name == Key) return Old = std::move(V);
            Keys.emplace_back(Key, std::move(V));
            return Keys.back().second;
        }
        Json& Push(Json V)
        {
            Items.push_back(std::move(V));
            return Items.back();
        }
        bool Empty() const { return Keys.empty() && Items.empty(); }
    };

    inline void Quote(const std::string& S, std::string& Out)
    {
        Out += '"';
        for (unsigned char C : S)
        {
            switch (C)
            {
            case '"': Out += "\\\""; break;
            case '\\': Out += "\\\\"; break;
            case '\n': Out += "\\n"; break;
            case '\r': Out += "\\r"; break;
            case '\t': Out += "\\t"; break;
            case '\b': Out += "\\b"; break;
            case '\f': Out += "\\f"; break;
            default:
                if (C < 0x20)
                {
                    char B[8];
                    std::snprintf(B, sizeof(B), "\\u%04x", C);
                    Out += B;
                }
                else Out += (char)C;
            }
        }
        Out += '"';
    }

    inline void Write(const Json& J, int Depth, std::string& Out)
    {
        const auto Indent = [&](int D) { Out += "\r\n" + std::string((size_t)D * 4, ' '); };
        switch (J.K)
        {
        case Json::Kind::Raw: Out += J.Text; return;
        case Json::Kind::String: Quote(J.Text, Out); return;
        case Json::Kind::Array:
            if (J.Items.empty())
            {
                Out += "[]";
                return;
            }
            Out += '[';
            for (size_t I = 0; I < J.Items.size(); I++)
            {
                if (I) Out += ',';
                Indent(Depth + 1);
                Write(J.Items[I], Depth + 1, Out);
            }
            Indent(Depth);
            Out += ']';
            return;
        case Json::Kind::Object:
            if (J.Keys.empty())
            {
                Out += "{}";
                return;
            }
            Out += '{';
            for (size_t I = 0; I < J.Keys.size(); I++)
            {
                if (I) Out += ',';
                Indent(Depth + 1);
                Quote(J.Keys[I].first, Out);
                Out += ": ";
                Write(J.Keys[I].second, Depth + 1, Out);
            }
            Indent(Depth);
            Out += '}';
            return;
        }
    }

    // Newtonsoft's Formatting.Indented, 4 spaces, CRLF, no final newline
    inline std::string Dump(const Json& J)
    {
        std::string Out;
        Write(J, 0, Out);
        return Out;
    }
}
