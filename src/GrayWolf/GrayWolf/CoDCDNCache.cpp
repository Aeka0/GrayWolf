#include "stdafx.h"

#include "CoDCDNCache.h"

#include "FileSystems.h"
#include "BinaryWriter.h"
#include "BinaryReader.h"

#include <filesystem>

CoDCDNCache::CoDCDNCache()
{
	Loaded = false;
}

bool CoDCDNCache::Load(const std::string& name)
{
    std::error_code Ec;
	std::string cdnDir = (std::filesystem::path(FileSystems::GetApplicationPath()) / "cdn_cache").string();

	InfoFilePath = (std::filesystem::path(cdnDir) / (name + ".cdn_info")).string();
	DataFilePath = (std::filesystem::path(cdnDir) / (name + ".cdn_data")).string();

	std::filesystem::create_directories(cdnDir, Ec);

	Loaded = true;

	BinaryReader reader;

	// Verify our info file can be opened and is valid, not necessarily a fatal
	// issue as we'll generate it later.
	if (!reader.Open(InfoFilePath))
		return false;
	if (reader.Read<uint32_t>() != 0x494E4443)
		return false;
	if (reader.Read<uint32_t>() != 1)
		return false;

	auto numEntries = reader.Read<uint32_t>();

	for (uint32_t i = 0; i < numEntries; i++)
	{
		auto entry = reader.Read<CoDCDNCacheEntry>();
		Entries[entry.Hash] = entry;
	}

	return true;
}

bool CoDCDNCache::Save()
{
	if (!Loaded)
		return false;

	BinaryWriter writer;

	if (!writer.Create(InfoFilePath))
		return false;

	writer.Write<uint32_t>(0x494E4443);
	writer.Write<uint32_t>(1);
	writer.Write<uint32_t>((uint32_t)Entries.size());

	for (auto& entry : Entries)
	{
		writer.Write(entry.second);
	}

	return true;
}

bool CoDCDNCache::Add(const uint64_t hash, const uint8_t* buffer, const size_t bufferSize)
{
	if (!Loaded)
		return false;

	BinaryWriter writer;

	if (!writer.Open(DataFilePath))
		return false;

	CoDCDNCacheEntry entry{};

	entry.Hash = hash;
	entry.Offset = writer.GetLength();;
	entry.Size = bufferSize;

	Entries[entry.Hash] = entry;

	if (!Save())
	{
		return false;
	}

	writer.SetPosition(entry.Offset);
	writer.Write(buffer, (uint32_t)bufferSize);
	return true;
}

std::unique_ptr<uint8_t[]> CoDCDNCache::Extract(const uint64_t hash, size_t expectedSize, size_t& bufferSize)
{
	if (!Loaded)
		return nullptr;

	auto find = Entries.find(hash);

	if (find == Entries.end())
		return nullptr;

	auto entry = find->second;

	// If we have a size mismatch then something happened either
	// when downloading from CDN or the size of this entry has changed
	if (expectedSize != 0 && entry.Size != expectedSize)
		return nullptr;

	BinaryReader reader;

	if (!reader.Open(DataFilePath))
		return nullptr;

	auto result = std::make_unique<uint8_t[]>(entry.Size);

	reader.SetPosition(entry.Offset);

	reader.Read(result.get(), entry.Size, bufferSize);

	if (bufferSize != entry.Size)
	{
		bufferSize = 0;
		return nullptr;
	}

	return result;
}