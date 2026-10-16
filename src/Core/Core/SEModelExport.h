#pragma once

#include <string>

class WraithModel;

class SEModel
{
public:
    static void ExportSEModel(const WraithModel& Model, const std::string& FileName, bool SupportsScale = false);
};