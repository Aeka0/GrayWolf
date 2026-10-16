#include "Parasyte.h"

// Current Context Information
std::unique_ptr<ps::State> ps::state = nullptr;

ps::State::State() : 
	PoolsAddress(0),
	StringsAddress(0) {}

void ps::State::AddFlag(const std::string& flag)
{
	if (HasFlag(flag))
		return;

	Flags.push_back(flag);
}

bool ps::State::HasFlag(const std::string& flag)
{
	for (auto& setFlag : Flags)
	{
		if (flag.size() == setFlag.size())
		{
			if (std::equal(flag.begin(), flag.end(), setFlag.begin(), [](const char& a, const char b)
			{
				return a == b || tolower(a) == tolower(b);
			}))
			{
				return true;
			}
		}
	}
	return false;
}

void ps::PoolParser64(uint64_t offset, std::function<XAsset64(const uint64_t&)> request, std::function<void(XAsset64&)> callback)
{
	for (auto p = request(offset); ; p = request(p.Next))
	{
		if (p.Header != 0)
			callback(p);

		if (p.Next == 0)
		{
			break;
		}
	}
}

