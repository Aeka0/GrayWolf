#include "stdafx.h"

#include "Games/GameModule.h"

// Call of Duty Online's fast files, sealed IW4 streams (SealedFastFile.h), found inside the
// install's .ifs archives (IfsArchive.h).
namespace
{
    const GameModuleRegistration Registered{
        { "CODOL", "Call of Duty Online", "codoMP_client_shipRetail.exe", "IWff0100", 461, GameSupportStage::Assets },
    };
}
