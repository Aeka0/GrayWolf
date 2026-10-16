#pragma once

// Black Ops 6's techsets for the techsets kind (Techsets.h): TechsetFormat.cpp

#include "Techsets.h"

#include <memory>

namespace BO6
{
    std::shared_ptr<const Techsets::Format> TechsetFormat();
}
