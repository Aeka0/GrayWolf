#pragma once

#include <cstdint>
#include <memory>

#include <WraithModel.h>

#include "CoDAssets.h"

struct WeightsData
{
    float WeightValues[8];
    uint32_t BoneValues[8];
    uint8_t WeightCount;

    WeightsData()
    {
        WeightCount = 1;
        WeightValues[0] = 1.0f;
        WeightValues[1] = 1.0f;
        WeightValues[2] = 1.0f;
        WeightValues[3] = 1.0f;
        WeightValues[4] = 1.0f;
        WeightValues[5] = 1.0f;
        WeightValues[6] = 1.0f;
        WeightValues[7] = 1.0f;
        BoneValues[0] = 0;
        BoneValues[1] = 0;
        BoneValues[2] = 0;
        BoneValues[3] = 0;
        BoneValues[4] = 0;
        BoneValues[5] = 0;
        BoneValues[6] = 0;
        BoneValues[7] = 0;
    }
};

class CoDXModelTranslator
{
public:
    static std::unique_ptr<WraithModel> TranslateXModel(const std::unique_ptr<XModel_t>& Model, uint32_t LodIndex, bool JustBones = false);
    static std::unique_ptr<WraithModel> TranslateXModelHitbox(const std::unique_ptr<XModel_t>& Model);

    // Calculates the index of the biggest lod, returns -1 if there are no lods
    static int32_t CalculateBiggestLodIndex(const std::unique_ptr<XModel_t>& Model);

public:
    // Normal unpack, exposed so the offline by-file model exporter decodes identically
    static Vector3 UnpackNormalA(uint32_t Normal);
    static Vector3 UnpackNormalB(uint32_t Normal);
    static Vector3 UnpackNormalC(uint32_t Normal);
};