#include "stdafx.h"

#include "MaterialDescription.h"

// World at War materials: colour, normal and a pre-PBR specular map (MaterialDescription.h, LegacySpecularAdapter).
namespace
{
    const Materials::AdapterRegistration Registered("T4", Materials::LegacySpecularAdapter());
}
