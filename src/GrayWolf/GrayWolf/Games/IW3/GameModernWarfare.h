#pragma once

#include <cstdint>
#include <memory>

class CoDAnim_t;
class CoDModel_t;
struct XImage_t;
struct XMaterial_t;
struct XModel_t;
struct XAnim_t;
struct XImageDDS;

class GameModernWarfare
{
public:
    static std::unique_ptr<XAnim_t> ReadXAnim(const CoDAnim_t* Animation);
    static std::unique_ptr<XModel_t> ReadXModel(const CoDModel_t* Model);

    static std::unique_ptr<XImageDDS> LoadXImage(const XImage_t& Image);

    static const XMaterial_t ReadXMaterial(uint64_t MaterialPointer);
};