#pragma once

#include <string>

class WraithModel;

class CodXME
{
public:
    static void ExportXME(const WraithModel& Model, const std::string& FileName);
};