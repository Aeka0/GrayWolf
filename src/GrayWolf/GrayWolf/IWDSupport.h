#pragma once

#include <string>
#include <memory>

class CoDImage_t;
struct XImageDDS;

class IWDSupport
{
public:

    static bool ParseIWD(const std::string& FilePath);

    static std::unique_ptr<XImageDDS> ReadImageFile(const CoDImage_t* Image);
};