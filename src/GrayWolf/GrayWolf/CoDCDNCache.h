#pragma once

#include <cstdint>
#include <map>
#include <memory>
#include <mutex>
#include <string>

struct CoDCDNCacheEntry
{
	uint64_t Hash;
	// The offset within the data file.
	uint64_t Offset;
	uint64_t Size;
};

class CoDCDNCache
{
private:
	std::mutex Mutex;
	std::string InfoFilePath;
	std::string DataFilePath;
	std::map<uint64_t, CoDCDNCacheEntry> Entries;
	bool Loaded;
public:
	CoDCDNCache();
	bool Load(const std::string& name);
	bool Save();
	bool Add(const uint64_t hash, const uint8_t* buffer, const size_t bufferSize);
	std::unique_ptr<uint8_t[]> Extract(const uint64_t hash, size_t expectedSize, size_t& bufferSize);
};

