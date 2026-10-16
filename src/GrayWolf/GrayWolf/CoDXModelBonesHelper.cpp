#include "stdafx.h"
#include "CoDXModelBonesHelper.h"
#include "Strings.h"
#include "DBGameGenerics.h"

#include <format>

void CoDXModelBonesHelper::ReadXModelBones(const std::unique_ptr<XModel_t>& Model, const XModelLod_t& ModelLOD, const std::unique_ptr<WraithModel>& ResultModel)
{
    size_t BoneCount = (size_t)Model->BoneCount + Model->CosmeticBoneCount;

    for (size_t i = 0; i < BoneCount; i++)
    {
        auto& NewBone = ResultModel->AddBone();

        uint64_t BoneID = 0;

        // TODO: Switch this out for a bone ID "type" enum for when we port other games to use this helper
        switch (Model->BoneIndexSize)
        {
        case 4: NewBone.TagName = CoDAssets::GetHashedString("bone", (uint64_t)CoDAssets::GameInstance->Read<uint32_t>(Model->BoneIDsPtr + i * 4)); break;
        case 8: NewBone.TagName = CoDAssets::GetHashedString("bone", (uint64_t)CoDAssets::GameInstance->Read<uint32_t>(Model->BoneIDsPtr + i * 8 + 4)); break;
        }

        if (Strings::IsNullOrWhiteSpace(NewBone.TagName))
        {
            if (i == 0)
            {
                NewBone.TagName = "tag_origin";
            }
            else
            {
                NewBone.TagName = std::format("no_tag_{}", i);
            }
        }

        if (i >= Model->RootBoneCount)
        {
            size_t ParentIDIndex = i - Model->RootBoneCount;

            switch (Model->BoneParentSize)
            {
            case 1: NewBone.BoneParent = (int32_t)CoDAssets::GameInstance->Read<uint8_t>(Model->BoneParentsPtr + ParentIDIndex); break;
            case 2: NewBone.BoneParent = (int32_t)CoDAssets::GameInstance->Read<uint16_t>(Model->BoneParentsPtr + ParentIDIndex * sizeof(uint16_t)); break;
            case 4: NewBone.BoneParent = (int32_t)CoDAssets::GameInstance->Read<uint32_t>(Model->BoneParentsPtr + ParentIDIndex * sizeof(uint32_t)); break;
            }

            // Check if we're cosmetic bones
            if (i < Model->BoneCount)
            {
                // Subtract position
                NewBone.BoneParent = (i - NewBone.BoneParent);
            }
            else
            {
                // Absolute position
                NewBone.IsCosmetic = true;
            }
        }
        else
        {
            NewBone.BoneParent = -1;
        }

        auto GlobalData = CoDAssets::GameInstance->Read<DObjAnimMat>(Model->BaseMatriciesPtr + i * sizeof(DObjAnimMat));

        NewBone.GlobalPosition = GlobalData.Translation;
        NewBone.GlobalRotation = GlobalData.Rotation;
    }

    // Check if we didn't parse any bones, if we didn't, we need can "inject" tag_origin (this is mostly for MW which has weird models
    // that are rare with thousands of bones)
    if (Model->BoneCount == 0)
    {
        auto& NewBone = ResultModel->AddBone();
        NewBone.TagName = "tag_origin";
        NewBone.BoneParent = -1;
    }

    ResultModel->GenerateLocalPositions(true, true);
}
