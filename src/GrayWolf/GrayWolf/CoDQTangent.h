#pragma once
#include "VectorMath.h"

class CoDQTangent
{
private:
	uint32_t Packed = 0;
public:
	// Unpacks the QTangent value into a normal. Optionally can output tangent and bitangent.
	Vector3 Unpack(Vector3* tangent, Vector3* bitangents);
};