#include <intrin.h>
#include <cstring>
#include <cstdio>
#include <cstdlib>

#include "memory.h"
#include "log.h"
#include "xorstr.h"

// ============================================================================
// PEB structures (avoid winternl.h dependency)
// ============================================================================
struct _UNICODE_STRING_PEB
{
	USHORT Length;
	USHORT MaximumLength;
	PWSTR  Buffer;
};

struct _LDR_DATA_TABLE_ENTRY_PEB
{
	LIST_ENTRY InLoadOrderLinks;
	LIST_ENTRY InMemoryOrderLinks;
	LIST_ENTRY InInitializationOrderLinks;
	PVOID DllBase;
	PVOID EntryPoint;
	ULONG SizeOfImage;
	_UNICODE_STRING_PEB FullDllName;
	_UNICODE_STRING_PEB BaseDllName;
};

struct _PEB_LDR_DATA_PEB
{
	BYTE       Reserved1[8];
	PVOID      Reserved2[3];
	LIST_ENTRY InMemoryOrderModuleList;
};

struct _PEB_CUSTOM
{
	BYTE          Reserved1[2];
	BYTE          BeingDebugged;
	BYTE          Reserved2[1];
	PVOID         Reserved3[2];
	_PEB_LDR_DATA_PEB* Ldr;
};

// ============================================================================
// internal helpers
// ============================================================================

// case-insensitive wide string compare
static int WideStrCmpI(const wchar_t* a, const wchar_t* b)
{
	while (*a && *b)
	{
		wchar_t ca = (*a >= L'A' && *a <= L'Z') ? (*a + 32) : *a;
		wchar_t cb = (*b >= L'A' && *b <= L'Z') ? (*b + 32) : *b;
		if (ca != cb) return static_cast<int>(ca - cb);
		++a; ++b;
	}
	return static_cast<int>(*a - *b);
}

// hex char to nibble value
static std::uint8_t HexCharToNibble(char c) noexcept
{
	if (c >= '0' && c <= '9') return static_cast<std::uint8_t>(c - '0');
	if (c >= 'A' && c <= 'F') return static_cast<std::uint8_t>(c - 'A' + 10);
	if (c >= 'a' && c <= 'f') return static_cast<std::uint8_t>(c - 'a' + 10);
	return 0;
}

// ============================================================================
// module functions
// ============================================================================

HMODULE MEM::GetModuleBaseHandle(const wchar_t* wszModuleName)
{
	// walk the PEB loader data — no API calls, stealthier
	const auto* pPEB = reinterpret_cast<_PEB_CUSTOM*>(__readgsqword(0x60));

	if (wszModuleName == nullptr)
		return reinterpret_cast<HMODULE>(pPEB->Reserved3[1]); // ImageBaseAddress

	for (auto* pEntry = pPEB->Ldr->InMemoryOrderModuleList.Flink;
		pEntry != &pPEB->Ldr->InMemoryOrderModuleList;
		pEntry = pEntry->Flink)
	{
		const auto* pData = CONTAINING_RECORD(pEntry, _LDR_DATA_TABLE_ENTRY_PEB, InMemoryOrderLinks);

		if (pData->BaseDllName.Buffer != nullptr && WideStrCmpI(wszModuleName, pData->BaseDllName.Buffer) == 0)
			return reinterpret_cast<HMODULE>(pData->DllBase);
	}

	L_PRINT(LOG_ERROR) << _XS("module not found: ") << wszModuleName;
	return nullptr;
}

HMODULE MEM::GetModuleBaseHandle(const char* szModuleName)
{
	if (szModuleName == nullptr)
		return GetModuleBaseHandle(static_cast<const wchar_t*>(nullptr));

	// convert narrow to wide
	wchar_t wszBuffer[MAX_PATH];
	::MultiByteToWideChar(CP_UTF8, 0, szModuleName, -1, wszBuffer, MAX_PATH);
	return GetModuleBaseHandle(wszBuffer);
}

bool MEM::GetModuleInfo(const wchar_t* wszModuleName, ModuleInfo_t* pOutInfo)
{
	HMODULE hModule = GetModuleBaseHandle(wszModuleName);
	if (hModule == nullptr)
		return false;

	const auto pBase = reinterpret_cast<const std::uint8_t*>(hModule);
	const auto pDOS = reinterpret_cast<const IMAGE_DOS_HEADER*>(pBase);
	if (pDOS->e_magic != IMAGE_DOS_SIGNATURE)
		return false;

	const auto pNT = reinterpret_cast<const IMAGE_NT_HEADERS64*>(pBase + pDOS->e_lfanew);
	if (pNT->Signature != IMAGE_NT_SIGNATURE)
		return false;

	pOutInfo->uBase = reinterpret_cast<std::uintptr_t>(pBase);
	pOutInfo->nSize = pNT->OptionalHeader.SizeOfImage;
	pOutInfo->uCodeBase = reinterpret_cast<std::uintptr_t>(pBase) + pNT->OptionalHeader.BaseOfCode;
	pOutInfo->nCodeSize = pNT->OptionalHeader.SizeOfCode;
	return true;
}

bool MEM::GetModuleInfo(const char* szModuleName, ModuleInfo_t* pOutInfo)
{
	wchar_t wszBuffer[MAX_PATH];
	::MultiByteToWideChar(CP_UTF8, 0, szModuleName, -1, wszBuffer, MAX_PATH);
	return GetModuleInfo(wszBuffer, pOutInfo);
}

std::uintptr_t MEM::GetExportAddress(HMODULE hModule, const char* szProcName)
{
	if (hModule == nullptr)
		return 0;

	const auto pBase = reinterpret_cast<const std::uint8_t*>(hModule);
	const auto pDOS = reinterpret_cast<const IMAGE_DOS_HEADER*>(pBase);
	if (pDOS->e_magic != IMAGE_DOS_SIGNATURE)
		return 0;

	const auto pNT = reinterpret_cast<const IMAGE_NT_HEADERS64*>(pBase + pDOS->e_lfanew);
	if (pNT->Signature != IMAGE_NT_SIGNATURE)
		return 0;

	const auto& exportDir = pNT->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT];
	if (exportDir.Size == 0 || exportDir.VirtualAddress == 0)
	{
		L_PRINT(LOG_ERROR) << _XS("module has no exports");
		return 0;
	}

	const auto pExport = reinterpret_cast<const IMAGE_EXPORT_DIRECTORY*>(pBase + exportDir.VirtualAddress);
	const auto pNames = reinterpret_cast<const std::uint32_t*>(pBase + pExport->AddressOfNames);
	const auto pOrdinals = reinterpret_cast<const std::uint16_t*>(pBase + pExport->AddressOfNameOrdinals);
	const auto pFunctions = reinterpret_cast<const std::uint32_t*>(pBase + pExport->AddressOfFunctions);

	// binary search on sorted export name table
	std::size_t nLeft = 0, nRight = pExport->NumberOfNames;
	while (nLeft < nRight)
	{
		const std::size_t nMid = nLeft + ((nRight - nLeft) >> 1);
		const int nCmp = strcmp(szProcName, reinterpret_cast<const char*>(pBase + pNames[nMid]));

		if (nCmp == 0)
		{
			const std::uint32_t uRVA = pFunctions[pOrdinals[nMid]];

			// check for forwarded export
			if (uRVA >= exportDir.VirtualAddress && uRVA < exportDir.VirtualAddress + exportDir.Size)
			{
				L_PRINT(LOG_WARNING) << _XS("forwarded export not supported: ") << szProcName;
				return 0;
			}

			return reinterpret_cast<std::uintptr_t>(pBase + uRVA);
		}

		if (nCmp > 0)
			nLeft = nMid + 1;
		else
			nRight = nMid;
	}

	L_PRINT(LOG_ERROR) << _XS("export not found: ") << szProcName;
	return 0;
}

bool MEM::GetSectionInfo(HMODULE hModule, const char* szSectionName,
	std::uint8_t** ppStart, std::size_t* pSize)
{
	const auto pBase = reinterpret_cast<const std::uint8_t*>(hModule);
	const auto pDOS = reinterpret_cast<const IMAGE_DOS_HEADER*>(pBase);
	if (pDOS->e_magic != IMAGE_DOS_SIGNATURE) return false;

	const auto pNT = reinterpret_cast<const IMAGE_NT_HEADERS64*>(pBase + pDOS->e_lfanew);
	if (pNT->Signature != IMAGE_NT_SIGNATURE) return false;

	const IMAGE_SECTION_HEADER* pSection = IMAGE_FIRST_SECTION(pNT);
	for (WORD i = 0; i < pNT->FileHeader.NumberOfSections; ++i, ++pSection)
	{
		if (strncmp(szSectionName, reinterpret_cast<const char*>(pSection->Name), IMAGE_SIZEOF_SHORT_NAME) == 0)
		{
			if (ppStart) *ppStart = const_cast<std::uint8_t*>(pBase) + pSection->VirtualAddress;
			if (pSize)   *pSize   = pSection->SizeOfRawData;
			return true;
		}
	}

	L_PRINT(LOG_ERROR) << _XS("section not found: ") << szSectionName;
	return false;
}

// ============================================================================
// pattern conversion
// ============================================================================

std::size_t MEM::PatternToBytes(const char* szPattern, std::uint8_t* pOutBytes, char* szOutMask)
{
	std::size_t nCount = 0;

	while (*szPattern != '\0')
	{
		if (*szPattern == ' ')
		{
			++szPattern;
			continue;
		}

		if (*szPattern == '?')
		{
			pOutBytes[nCount] = 0x00;
			szOutMask[nCount] = '?';
			++nCount;
			++szPattern;
			// skip second '?' in "??"
			if (*szPattern == '?')
				++szPattern;
			continue;
		}

		// parse two hex digits
		pOutBytes[nCount] = static_cast<std::uint8_t>((HexCharToNibble(szPattern[0]) << 4) | HexCharToNibble(szPattern[1]));
		szOutMask[nCount] = 'x';
		++nCount;
		szPattern += 2;
	}

	pOutBytes[nCount] = 0;
	szOutMask[nCount] = '\0';
	return nCount;
}

// ============================================================================
// pattern scanning engine
// ============================================================================

std::uintptr_t MEM::FindPatternEx(const std::uint8_t* pStart, std::size_t nSize,
	const std::uint8_t* pBytes, const char* szMask)
{
	const std::size_t nPatternLen = strlen(szMask);
	if (nPatternLen == 0 || nSize < nPatternLen)
		return 0;

	const std::uint8_t* const pModuleEnd = pStart + nSize;
	const std::uint8_t* const pScanBound = pModuleEnd - nPatternLen;
	const std::uint8_t* pCur = pStart;

	// Walk region-by-region using VirtualQuery to skip non-readable/guard pages.
	// Critical fix: PAGE_GUARD pages raise STATUS_GUARD_PAGE_VIOLATION (not AV).
	// Non-committed gaps in a DLL's SizeOfImage range must be skipped, not abort.
	while (pCur <= pScanBound)
	{
		MEMORY_BASIC_INFORMATION mbi{};
		if (!VirtualQuery(pCur, &mbi, sizeof(mbi)))
			break; // VirtualQuery failure — cannot safely advance

		const std::uint8_t* pRegionEnd =
			reinterpret_cast<const std::uint8_t*>(mbi.BaseAddress) + mbi.RegionSize;

		// Safety: ensure we make forward progress
		if (pRegionEnd <= pCur)
			break;

		// Skip non-committed regions (MEM_FREE / MEM_RESERVE) and guard pages.
		// PAGE_GUARD must be checked on the RAW Protect value before stripping modifiers.
		if (mbi.State != MEM_COMMIT || (mbi.Protect & PAGE_GUARD))
		{
			pCur = pRegionEnd;
			continue;
		}

		// Check readability (strip only cache-mode modifier bits, NOT PAGE_GUARD)
		const DWORD nProt = mbi.Protect & ~(PAGE_NOCACHE | PAGE_WRITECOMBINE);
		const bool bReadable = (nProt == PAGE_READONLY           ||
		                        nProt == PAGE_READWRITE           ||
		                        nProt == PAGE_EXECUTE             ||
		                        nProt == PAGE_EXECUTE_READ        ||
		                        nProt == PAGE_EXECUTE_READWRITE   ||
		                        nProt == PAGE_EXECUTE_WRITECOPY   ||
		                        nProt == PAGE_WRITECOPY);

		if (!bReadable)
		{
			pCur = pRegionEnd;
			continue;
		}

		// Clamp inner scan end to this readable region and the overall scan bound
		const std::uint8_t* pInnerEnd = pRegionEnd - nPatternLen;
		if (pInnerEnd > pScanBound)
			pInnerEnd = pScanBound;

		for (; pCur <= pInnerEnd; ++pCur)
		{
			bool bFound = true;
			for (std::size_t j = 0; j < nPatternLen; ++j)
			{
				if (szMask[j] != '?' && pCur[j] != pBytes[j])
				{
					bFound = false;
					break;
				}
			}
			if (bFound)
				return reinterpret_cast<std::uintptr_t>(pCur);
		}

		pCur = pRegionEnd;
	}

	return 0;
}

std::uintptr_t MEM::FindPattern(const char* szModuleName, const char* szPattern,
	ESearchType eSearchType, std::int32_t nOffset)
{
	wchar_t wszModule[MAX_PATH];
	::MultiByteToWideChar(CP_UTF8, 0, szModuleName, -1, wszModule, MAX_PATH);
	return FindPattern(wszModule, szPattern, eSearchType, nOffset);
}

std::uintptr_t MEM::FindPattern(const wchar_t* wszModuleName, const char* szPattern,
	ESearchType eSearchType, std::int32_t nOffset)
{
	ModuleInfo_t modInfo{};
	if (!GetModuleInfo(wszModuleName, &modInfo))
	{
		L_PRINT(LOG_ERROR) << _XS("failed to get module info for pattern scan");
		return 0;
	}

	// convert IDA pattern to bytes + mask
	const std::size_t nApproxSize = (strlen(szPattern) / 2) + 2;
	// stack-allocated buffers (safe for pattern sizes up to ~512 bytes)
	std::uint8_t byteBuffer[512];
	char maskBuffer[512];

	const std::size_t nByteCount = PatternToBytes(szPattern, byteBuffer, maskBuffer);
	if (nByteCount == 0)
	{
		L_PRINT(LOG_ERROR) << _XS("empty pattern");
		return 0;
	}

	// scan the entire module image
	std::uintptr_t uResult = FindPatternEx(
		reinterpret_cast<const std::uint8_t*>(modInfo.uBase),
		modInfo.nSize,
		byteBuffer,
		maskBuffer
	);

	if (uResult == 0)
	{
		L_PRINT(LOG_ERROR) << _XS("pattern not found: ") << szPattern;
		return 0;
	}

	// apply offset
	uResult += nOffset;

	// resolve based on search type
	switch (eSearchType)
	{
	case ESearchType::NONE:
		break;
	case ESearchType::PTR:
		// mov reg, [rip+disp32] — displacement at +3, instruction size 7
		uResult = GetAbsoluteAddress(uResult, 3, 7);
		break;
	case ESearchType::CALL:
		// call rel32 — displacement at +1, instruction size 5
		uResult = GetCallAddress(uResult);
		break;
	case ESearchType::RIP:
		// generic — caller must set nOffset to point to the instruction,
		// then we resolve with default 3/7 (can be overridden per-pattern in batch scan)
		uResult = GetAbsoluteAddress(uResult, 3, 7);
		break;
	}

	return uResult;
}

std::size_t MEM::ScanPatterns(PatternInfo_t* pPatterns, std::size_t nCount)
{
	std::size_t nSuccessCount = 0;

	L_PRINT(LOG_INFO) << _XS("--- begin pattern scan (") << nCount << _XS(" patterns) ---");

	for (std::size_t i = 0; i < nCount; ++i)
	{
		PatternInfo_t& pat = pPatterns[i];

		pat.uResult = FindPattern(pat.szModule, pat.szPattern, pat.eSearchType, pat.nOffset);

		if (pat.uResult != 0)
		{
			++nSuccessCount;
			L_PRINT(LOG_INFO) << _XS("[+] ") << pat.szName << _XS(" = ")
				<< L::AddFlags(LOG_MODE_INT_HEX | LOG_MODE_SHOWBASE) << pat.uResult;
		}
		else
		{
			L_PRINT(LOG_ERROR) << _XS("[-] ") << pat.szName << _XS(" FAILED")
				<< (pat.szComment ? " | hint: " : "") << (pat.szComment ? pat.szComment : "");
		}
	}

	L_PRINT(LOG_INFO) << _XS("--- pattern scan complete: ") << nSuccessCount << _XS("/") << nCount << _XS(" succeeded ---");
	return nSuccessCount;
}
