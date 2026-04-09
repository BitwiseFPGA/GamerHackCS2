#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#include <cstdint>
#include <cstddef>
#include <type_traits>

/*
 * MEMORY UTILITIES
 * - IDA-style pattern scanning
 * - RIP-relative address resolution
 * - PE header parsing
 * - Virtual function table access
 * - Module enumeration
 *
 * pattern search types:
 *   NONE — raw address of the pattern match
 *   PTR  — dereference RIP-relative (mov reg, [rip+disp32]) at offset
 *   CALL — extract CALL target (E8 xx xx xx xx) at offset
 *   RIP  — generic RIP-relative resolve at offset with custom instruction size
 */
namespace MEM
{
	// ========================================================================
	// search type enum
	// ========================================================================
	enum class ESearchType : std::uint8_t
	{
		NONE = 0, // direct match, return address as-is
		PTR,      // resolve RIP-relative pointer (e.g. mov rcx, [rip+X] — 7 byte insn, disp at +3)
		CALL,     // resolve CALL target (E8 XX XX XX XX — 5 byte insn, disp at +1)
		RIP       // generic RIP-relative with custom offset/size
	};

	// ========================================================================
	// pattern descriptor
	// ========================================================================

	/// stores a named pattern for batch scanning
	/// @note: fill in the 'szComment' with instructions on how to find this pattern
	///        in IDA/Ghidra (e.g. "xref string 'CEntitySystem::AddEntity' -> first function")
	struct PatternInfo_t
	{
		const char*  szName;        // human-readable name (e.g. "CEntitySystem::AddEntity")
		const char*  szModule;      // module to scan (e.g. "client.dll")
		const char*  szPattern;     // IDA-style pattern (e.g. "48 89 5C 24 ? 48 89 74 24 ?")
		ESearchType  eSearchType;   // how to resolve the raw match
		std::int32_t nOffset;       // offset from match start before applying search type
		std::int32_t nRIPSize;      // instruction size for RIP resolves (only used with ESearchType::RIP)
		const char*  szComment;     // how to find this pattern in a disassembler

		// result is filled after scanning
		std::uintptr_t uResult = 0;
	};

	// ========================================================================
	// module info
	// ========================================================================
	struct ModuleInfo_t
	{
		std::uintptr_t uBase;
		std::size_t    nSize;
		std::uintptr_t uCodeBase;
		std::size_t    nCodeSize;
	};

	// ========================================================================
	// module functions
	// ========================================================================

	/// get module handle by name using PEB traversal (no API calls)
	/// @param[in] wszModuleName wide module name, null for current process image
	/// @returns: HMODULE or nullptr
	[[nodiscard]] HMODULE GetModuleBaseHandle(const wchar_t* wszModuleName);

	/// overload for narrow strings (uses GetModuleHandleA internally)
	[[nodiscard]] HMODULE GetModuleBaseHandle(const char* szModuleName);

	/// get module base + size + code section info
	[[nodiscard]] bool GetModuleInfo(const wchar_t* wszModuleName, ModuleInfo_t* pOutInfo);
	[[nodiscard]] bool GetModuleInfo(const char* szModuleName, ModuleInfo_t* pOutInfo);

	/// get export address by name from a module (PE export table walk)
	[[nodiscard]] std::uintptr_t GetExportAddress(HMODULE hModule, const char* szProcName);

	/// get PE section info by name
	[[nodiscard]] bool GetSectionInfo(HMODULE hModule, const char* szSectionName,
		std::uint8_t** ppStart, std::size_t* pSize);

	// ========================================================================
	// address resolution
	// ========================================================================

	/// resolve RIP-relative address: addr + offset + 4 + *(int32_t*)(addr + offset)
	/// @param[in] uAddress base address of the instruction
	/// @param[in] nOffset offset to the 32-bit displacement within the instruction
	/// @param[in] nSize total instruction size (displacement is relative to instruction end)
	[[nodiscard]] inline std::uintptr_t GetAbsoluteAddress(std::uintptr_t uAddress, int nOffset = 3, int nSize = 7) noexcept
	{
		const std::int32_t nDisp = *reinterpret_cast<std::int32_t*>(uAddress + nOffset);
		return uAddress + nSize + nDisp;
	}

	/// extract CALL target: addr + 5 + *(int32_t*)(addr + 1)
	[[nodiscard]] inline std::uintptr_t GetCallAddress(std::uintptr_t uAddress) noexcept
	{
		const std::int32_t nDisp = *reinterpret_cast<std::int32_t*>(uAddress + 1);
		return uAddress + 5 + nDisp;
	}

	// ========================================================================
	// virtual function table
	// ========================================================================

	/// get virtual function pointer by index
	[[nodiscard]] inline std::uintptr_t GetVFunc(const void* pInstance, std::size_t nIndex) noexcept
	{
		const auto pVTable = *reinterpret_cast<const std::uintptr_t* const*>(pInstance);
		return pVTable[nIndex];
	}

	/// call virtual function at given index with arbitrary arguments
	template <typename TReturn, std::size_t nIndex, typename TBase, typename... TArgs>
	inline TReturn CallVFunc(TBase* pInstance, TArgs... args)
	{
		using Fn_t = TReturn(__thiscall*)(const void*, TArgs...);
		const auto pVTable = *reinterpret_cast<Fn_t* const*>(pInstance);
		return pVTable[nIndex](pInstance, args...);
	}

	// ========================================================================
	// pattern scanning
	// ========================================================================

	/// scan for IDA-style pattern in a specific module
	/// @param[in] szModuleName narrow string module name (e.g. "client.dll")
	/// @param[in] szPattern IDA-style pattern (e.g. "48 8B 0D ? ? ? ? 48 85 C9 74 ? E8")
	/// @param[in] eSearchType how to resolve the found address
	/// @param[in] nOffset offset from pattern start before search type resolution
	/// @returns: resolved address or 0 on failure
	[[nodiscard]] std::uintptr_t FindPattern(const char* szModuleName, const char* szPattern,
		ESearchType eSearchType = ESearchType::NONE, std::int32_t nOffset = 0);

	/// scan for IDA-style pattern in a specific module (wide string overload)
	[[nodiscard]] std::uintptr_t FindPattern(const wchar_t* wszModuleName, const char* szPattern,
		ESearchType eSearchType = ESearchType::NONE, std::int32_t nOffset = 0);

	/// scan a raw byte range with mask
	/// @param[in] pStart start of the search region
	/// @param[in] nSize size of the search region in bytes
	/// @param[in] pBytes byte pattern to search for
	/// @param[in] szMask mask string ('x' = match, '?' = wildcard)
	/// @returns: address of first match or 0
	[[nodiscard]] std::uintptr_t FindPatternEx(const std::uint8_t* pStart, std::size_t nSize,
		const std::uint8_t* pBytes, const char* szMask);

	/// batch-scan an array of PatternInfo_t entries, log results
	/// @returns: number of patterns that resolved successfully
	std::size_t ScanPatterns(PatternInfo_t* pPatterns, std::size_t nCount);

	// ========================================================================
	// pattern helpers (internal)
	// ========================================================================

	/// convert IDA-style pattern string to byte array + mask
	/// @returns: number of bytes written
	std::size_t PatternToBytes(const char* szPattern, std::uint8_t* pOutBytes, char* szOutMask);
}
