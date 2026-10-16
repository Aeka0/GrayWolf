#pragma once

#include <cstdint>
#include <memory>
#include <string>

#include "CoDXModelTranslator.h"

class MemoryReader;

class GameModernWarfare5
{
public:
    static bool LoadAssets();

    static std::unique_ptr<XAnim_t> ReadXAnim(const CoDAnim_t* Animation);
    static std::unique_ptr<XModel_t> ReadXModel(const CoDModel_t* Model);
    static std::unique_ptr<XImageDDS> ReadXImage(const CoDImage_t* Image);
    static std::unique_ptr<XSound> ReadXSound(const CoDSound_t* Sound);
    static const XMaterial_t ReadXMaterial(uint64_t MaterialPointer);
    static void TranslateRawfile(const CoDRawFile_t* Rawfile, const std::string& ExportPath);

    static std::unique_ptr<XImageDDS> LoadXImage(const XImage_t& Image);
    static void LoadXModel(const std::unique_ptr<XModel_t>& Model, const XModelLod_t& ModelLOD, const std::unique_ptr<WraithModel>& ResultModel);
    static void LoadXAnim(const std::unique_ptr<XAnim_t>& Anim, std::unique_ptr<WraithAnim>& ResultAnim);

    static std::string LoadStringEntry(uint64_t Index);

    static void PerformInitialSetup();
    static void PerformShutDown();
private:
    static void PrepareVertexWeights(MemoryReader& ComplexReader, std::vector<WeightsData>& Weights, const XModelSubmesh_t& Submesh);
};