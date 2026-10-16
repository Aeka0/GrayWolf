#pragma once

#include <string>

class WraithModel;

class CodXMB
{
public:
    static void ExportXMB(const WraithModel& Model, const std::string& FileName);
};