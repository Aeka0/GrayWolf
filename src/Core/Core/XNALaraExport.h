#pragma once

#include <string>

class WraithModel;

class XNALara
{
public:
    static void ExportXNA(const WraithModel& Model, const std::string& FileName);
};