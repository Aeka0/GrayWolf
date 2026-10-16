#pragma once

#include <string>
#include <utility>
#include <vector>

// What a streamed XAnim's header names, read before its keys: bones and notetracks (frame index)
class CoDXAnimReader
{
public:
    std::vector<std::string> BoneNames;
    std::vector<std::pair<std::string, size_t>> Notetracks;
};
