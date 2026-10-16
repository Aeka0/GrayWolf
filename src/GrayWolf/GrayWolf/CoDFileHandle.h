#pragma once
#include "CoDFileSystem.h"

// A class to hold a safe file handle that automatically closes itself.
class CoDFileHandle
{
private:
	// The file system this handle was opened from.
	CoDFileSystem* FileSystem;
	HANDLE Handle;
public:
	CoDFileHandle(HANDLE handle, CoDFileSystem* fileSystem);
	~CoDFileHandle();
	const bool IsValid() const;
	std::unique_ptr<uint8_t[]> Read(const size_t size);
	size_t Read(uint8_t* buffer, const size_t size);
	size_t Read(uint8_t* buffer, const size_t offset, const size_t size);
	size_t Tell();
	size_t Seek(size_t position, size_t direction);
	size_t Size();
	template <class T>
	T Read()
	{
		T r{};
		Read((uint8_t*)&r, 0, sizeof(r));
		return r;
	}
};