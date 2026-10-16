#include "stdafx.h"

#include "MaterialDescription.h"

// Call of Duty Online materials: colour, normal and a pre-PBR specular map (MaterialDescription.h, LegacySpecularAdapter).
namespace
{
    const Materials::AdapterRegistration Registered("CODOL", Materials::LegacySpecularAdapter());
}
