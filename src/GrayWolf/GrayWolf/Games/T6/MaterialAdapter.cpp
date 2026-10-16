#include "stdafx.h"

#include "MaterialDescription.h"

// Black Ops II materials: colour, normal and a pre-PBR specular map (MaterialDescription.h, LegacySpecularAdapter).
namespace
{
    const Materials::AdapterRegistration Registered("T6", Materials::LegacySpecularAdapter());
}
