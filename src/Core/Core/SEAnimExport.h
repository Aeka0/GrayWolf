#pragma once

#include <string>

class WraithAnim;

class SEAnim
{
public:
    static void ExportSEAnim(const WraithAnim& Animation, const std::string& FileName);
};