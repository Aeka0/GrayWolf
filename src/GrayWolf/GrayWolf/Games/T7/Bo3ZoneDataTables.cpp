#include "stdafx.h"

#include "Bo3ZoneDataTables.h"

#include "ExportNaming.h"
#include "ExportWriters.h"
#include <nlohmann/json.hpp>

#include <filesystem>
#include <fstream>

namespace
{
    namespace fs = std::filesystem;

    using ExportNaming::Lower;

    using ExportNaming::SafeFileName;

    using ExportWriters::CsvField;
}

bool Bo3WriteStringTableCsv(const std::string& Name, const Bo3StringTableCapture& T, const std::string& OutDir)
{
    return ExportWriters::WriteStringTable(OutDir, Name, T.Columns, T.Rows, T.Cells);
}

bool Bo3WriteStructuredTableCsv(const std::string& Name, const Bo3StructuredTableCapture& T, const std::string& OutDir)
{
    const auto TablesDir = fs::path(OutDir) / "structuredtables";
    std::error_code Ec;
    fs::create_directories(TablesDir, Ec);
    std::ofstream Out(TablesDir / (SafeFileName(Name) + ".csv"), std::ios::binary | std::ios::trunc);
    if (!Out) return false;

    for (int32_t Col = 0; Col < T.ColumnCount; Col++)
    {
        if (Col) Out << ",";
        if ((size_t)Col < T.Headers.size()) Out << CsvField(T.Headers[(size_t)Col]);
    }
    Out << "\r\n";

    for (int32_t Row = 0; Row < T.RowCount; Row++)
    {
        for (int32_t Col = 0; Col < T.ColumnCount; Col++)
        {
            if (Col) Out << ",";
            const size_t Index = (size_t)Row * (size_t)T.ColumnCount + (size_t)Col;
            if (Index < T.Cells.size())
            {
                const auto& Cell = T.Cells[Index];
                // A real STRING cell (type==1) that carried an inline resolved Text wins;
                // otherwise fall back to the raw Number field (also what a non-string cell, or
                // an unresolved-reference string cell, actually holds).
                if (Cell.Type == 1 && !Cell.Text.empty())
                    Out << CsvField(Cell.Text);
                else
                    Out << Cell.Number;
            }
        }
        Out << "\r\n";
    }
    return (bool)Out;
}

bool Bo3WriteDdlJson(const std::string& Name, const Bo3DdlCapture& Ddl, const std::string& OutDir,
                     size_t& StructsTotal, size_t& EnumsTotal)
{
    nlohmann::json J;
    J["ddl"] = Name;
    nlohmann::json Revisions = nlohmann::json::array();

    for (const auto& Node : Ddl.Chain)
    {
        nlohmann::json Rev;

        nlohmann::json Structs = nlohmann::json::array();
        for (const auto& S : Node.Structs)
        {
            StructsTotal++;
            nlohmann::json Sj;
            Sj["name"] = S.Name;
            nlohmann::json Members = nlohmann::json::array();
            for (const auto& M : S.Members) Members.push_back(M.Name);
            Sj["members"] = std::move(Members);
            Structs.push_back(std::move(Sj));
        }
        Rev["structs"] = std::move(Structs);

        nlohmann::json Enums = nlohmann::json::array();
        for (const auto& E : Node.Enums)
        {
            EnumsTotal++;
            nlohmann::json Ej;
            Ej["name"] = E.Name;
            Ej["members"] = E.Members;
            Enums.push_back(std::move(Ej));
        }
        Rev["enums"] = std::move(Enums);

        Revisions.push_back(std::move(Rev));
    }
    J["revisions"] = std::move(Revisions);

    const auto DdlDir = fs::path(OutDir) / "ddl";
    std::error_code Ec;
    fs::create_directories(DdlDir, Ec);
    std::ofstream Out(DdlDir / (SafeFileName(Name) + ".json"), std::ios::binary | std::ios::trunc);
    if (!Out) return false;
    Out << J.dump(2);
    return (bool)Out;
}
