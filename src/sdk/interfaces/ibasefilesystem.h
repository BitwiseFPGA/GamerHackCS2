#pragma once
#include <cstdint>
#include <cstdio>

// ---------------------------------------------------------------
// filesystem seek types
// ---------------------------------------------------------------
enum FileSystemSeek_t
{
	FILESYSTEM_SEEK_HEAD    = SEEK_SET,
	FILESYSTEM_SEEK_CURRENT = SEEK_CUR,
	FILESYSTEM_SEEK_TAIL    = SEEK_END,
};

using FileHandle_t = void*;
constexpr FileHandle_t FILESYSTEM_INVALID_HANDLE = nullptr;

// ---------------------------------------------------------------
// IBaseFileSystem — Valve file system interface
// ---------------------------------------------------------------
class IBaseFileSystem
{
public:
	virtual int          Read(void* pOutput, int size, FileHandle_t file) = 0;
	virtual int          Write(const void* pInput, int size, FileHandle_t file) = 0;
	virtual FileHandle_t Open(const char* pFileName, const char* pOptions, const char* pathID = nullptr) = 0;
	virtual void         Close(FileHandle_t file) = 0;
	virtual void         Seek(FileHandle_t file, int pos, FileSystemSeek_t seekType) = 0;
	virtual unsigned int Tell(FileHandle_t file) = 0;
	virtual unsigned int Size(FileHandle_t file) = 0;
	virtual unsigned int Size(const char* pFileName, const char* pPathID = nullptr) = 0;
	virtual void         Flush(FileHandle_t file) = 0;
	virtual bool         Precache(const char* pFileName, const char* pPathID = nullptr) = 0;
	virtual bool         FileExists(const char* pFileName, const char* pPathID = nullptr) = 0;
	virtual bool         IsFileWritable(const char* pFileName, const char* pPathID = nullptr) = 0;
	virtual bool         SetFileWritable(const char* pFileName, bool writable, const char* pPathID = nullptr) = 0;
	virtual long         GetFileTime(const char* pFileName, const char* pPathID = nullptr) = 0;
};
