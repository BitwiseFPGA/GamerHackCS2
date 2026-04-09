#pragma once
#include <cstdint>

// ---------------------------------------------------------------
// KeyValues — Valve key-value data structure
//
// Methods are resolved at runtime via pattern scanning.
// This is a read-only declaration matching the game's memory layout.
// ---------------------------------------------------------------
class KeyValues
{
public:
	[[nodiscard]] const char* GetName();
	[[nodiscard]] KeyValues*  GetFirstSubKey();
	[[nodiscard]] KeyValues*  GetFirstTrueSubKey();
	[[nodiscard]] KeyValues*  GetNextTrueSubKey();
	[[nodiscard]] KeyValues*  GetNextKey();
	[[nodiscard]] KeyValues*  FindKey(const char* pszKeyName, bool bCreate = false);

	[[nodiscard]] int          GetInt(const char* pszKeyName = nullptr, int nDefault = 0);
	[[nodiscard]] float        GetFloat(const char* pszKeyName = nullptr, float flDefault = 0.f);
	[[nodiscard]] std::uint64_t GetUint64(const char* pszKeyName = nullptr, std::uint64_t nDefault = 0);
	[[nodiscard]] const char*  GetString(const char* pszKeyName = nullptr, const char* pszDefault = "");

	void SetInt(const char* pszKeyName, int nValue);
	void SetFloat(const char* pszKeyName, float flValue);
	void SetString(const char* pszKeyName, const char* pszValue);
};

// ---------------------------------------------------------------
// KeyValues3 — Source 2 KV3 loader
// ---------------------------------------------------------------
class KeyValues3
{
public:
	/// load a KV3 resource file
	/// @param pResource — output resource buffer
	/// @param pszFilename — resource file path
	/// @param pszPathID — optional path ID filter
	/// @returns: true if load succeeded
	static bool LoadKV3(void* pResource, const char* pszFilename, const char* pszPathID = nullptr);
};
