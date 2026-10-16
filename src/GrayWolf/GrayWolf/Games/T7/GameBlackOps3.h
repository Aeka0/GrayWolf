#pragma once

#include <cstdint>
#include <memory>

class CoDAnim_t;
class CoDModel_t;
struct XImage_t;
struct XMaterial_t;
struct XModelLod_t;
struct XModel_t;
struct XAnim_t;
struct XImageDDS;
class WraithModel;

class GameBlackOps3
{
public:
    static std::unique_ptr<XAnim_t> ReadXAnim(const CoDAnim_t* Animation);
    static std::unique_ptr<XModel_t> ReadXModel(const CoDModel_t* Model);
    // Public: the disk path's map reader (Games/T7/DiskGame.cpp) reads materials with it.
    static const XMaterial_t ReadXMaterial(uint64_t MaterialPointer);

    static std::unique_ptr<XImageDDS> LoadXImage(const XImage_t& Image);
    static void LoadXModel(const XModelLod_t& ModelLOD, const std::unique_ptr<WraithModel>& ResultModel);
};