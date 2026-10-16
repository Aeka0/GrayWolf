#pragma once

#include <cstdint>
#include <memory>
#include <string>

class CoDAnim_t;
class CoDModel_t;
class CoDImage_t;
struct XImage_t;
struct XMaterial_t;
struct XModel_t;
struct XAnim_t;
struct XImageDDS;

class GameModernWarfare2RM
{
public:
    static bool LoadAssets();
    static bool LoadAssetsPS();

    static std::unique_ptr<XAnim_t> ReadXAnim(const CoDAnim_t* Animation);
    static std::unique_ptr<XModel_t> ReadXModel(const CoDModel_t* Model);
    static std::unique_ptr<XImageDDS> ReadXImage(const CoDImage_t* Image);

    static std::unique_ptr<XImageDDS> LoadXImage(const XImage_t& Image);
    static std::unique_ptr<XImageDDS> LoadXImagePS(const XImage_t& Image);

    static std::string LoadStringEntry(uint64_t Index);

private:
    static const XMaterial_t ReadXMaterial(uint64_t MaterialPointer);
};