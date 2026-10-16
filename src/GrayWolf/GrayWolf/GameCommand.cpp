#include "stdafx.h"

#include "GameCommand.h"

namespace
{
    std::vector<const GameCommand*>& Commands()
    {
        static std::vector<const GameCommand*> List;
        return List;
    }
}

GameCommandRegistration::GameCommandRegistration(const GameCommand& Command)
{
    Commands().push_back(&Command);
}

const std::vector<const GameCommand*>& GameCommands()
{
    return Commands();
}
