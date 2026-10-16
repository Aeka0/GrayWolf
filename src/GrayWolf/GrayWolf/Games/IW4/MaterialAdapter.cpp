#include "stdafx.h"

#include "MaterialDescription.h"

// Modern Warfare 2 materials: colour, normal and a pre-PBR specular map (MaterialDescription.h, LegacySpecularAdapter).
namespace
{
    const Materials::AdapterRegistration Registered("IW4", Materials::LegacySpecularAdapter());
}
