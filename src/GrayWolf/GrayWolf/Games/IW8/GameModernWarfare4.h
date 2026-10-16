#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <functional>
#include <utility>
#include <vector>

#include "CoDXModelTranslator.h"

class GameModernWarfare4
{
public:
    static bool LoadAssets();

    static std::unique_ptr<XAnim_t> ReadXAnim(const CoDAnim_t* Animation);
    static std::unique_ptr<XModel_t> ReadXModel(const CoDModel_t* Model);
    static std::unique_ptr<XImageDDS> ReadXImage(const CoDImage_t* Image);
    // What a name-only material header (a leading ',') stands for, when the caller knows the
    // zones defining it (a disk zone's techsets_ zones): its images as (semantic, image name)
    static thread_local std::function<bool(const std::string& Name, std::vector<std::pair<uint32_t, std::string>>& Images)> ImagesElsewhere;
    static const XMaterial_t ReadXMaterial(uint64_t MaterialPointer);
    static void TranslateRawfile(const CoDRawFile_t* Rawfile, const std::string& ExportPath);

    static std::unique_ptr<XImageDDS> LoadXImage(const XImage_t& Image);
    static void LoadXModel(const XModelLod_t& ModelLOD, const std::unique_ptr<WraithModel>& ResultModel);

    static std::string LoadStringEntry(uint64_t Index);

    static void PerformInitialSetup();
};
