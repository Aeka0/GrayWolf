#pragma once
#include <WraithModel.h>

#include "CoDAssets.h"

#include <memory>

class CoDXModelBonesHelper
{
public:
	static void ReadXModelBones(const std::unique_ptr<XModel_t>& Model, const XModelLod_t& ModelLOD, const std::unique_ptr<WraithModel>& ResultModel);
};