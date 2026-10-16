#include "stdafx.h"

#include "MaterialDescription.h"

// Call of Duty 4 materials: colour, normal and a pre-PBR specular map (MaterialDescription.h, LegacySpecularAdapter).
namespace
{
    const Materials::AdapterRegistration Registered("IW3", Materials::LegacySpecularAdapter());
}
