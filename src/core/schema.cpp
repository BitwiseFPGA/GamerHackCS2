#include "schema.h"
#include "interfaces.h"
#include "patterns.h"
#include "../utilities/xorstr.h"
#include <fstream>

// ============================================================================
// schema system structures (mirroring game's CSchemaSystem layout)
// ============================================================================

class CSchemaSystemTypeScope;

// field descriptor from the game's schema system
struct SchemaClassFieldData_t
{
	const char* szName;       // 0x00
	void*       pSchemaType;  // 0x08
	std::int32_t nSingleInheritanceOffset; // 0x10
	std::int32_t nMetadataSize;            // 0x14
	void*        pMetadata;   // 0x18
};

// class info returned by FindDeclaredClass (vtable index 2)
struct SchemaClassInfoData_t
{
	void*                   pVtable;          // 0x00
	const char*             szName;           // 0x08
	const char*             szDescription;    // 0x10
	int                     m_nSize;          // 0x18
	std::int16_t            nFieldCount;      // 0x1C
	std::int16_t            nStaticSize;      // 0x1E
	std::int16_t            nMetadataSize;    // 0x20
	std::uint8_t            nAlignOf;         // 0x22
	std::uint8_t            nBaseClassesCount;// 0x23
	std::uint8_t            _pad2[0x04];      // 0x24
	SchemaClassFieldData_t* pFields;          // 0x28
};

// class binding stored in the CUtlTSHash (hash table entries)
struct CSchemaClassBinding
{
	CSchemaClassBinding* pParent;             // 0x00
	const char*          szBinaryName;        // 0x08
	const char*          szModuleName;        // 0x10
	const char*          szClassName;         // 0x18
};

// type scope wrapper — represents a module's schema namespace
class CSchemaSystemTypeScope
{
public:
	const char* GetModuleName()
	{
		return reinterpret_cast<const char*>(reinterpret_cast<std::uintptr_t>(this) + 0x08);
	}

	// FindDeclaredClass — vtable index 2
	SchemaClassInfoData_t* FindDeclaredClass(const char* szClassName)
	{
		SchemaClassInfoData_t* pResult = nullptr;
		using Fn = void(__thiscall*)(void*, SchemaClassInfoData_t**, const char*);
		const auto pVTable = *reinterpret_cast<std::uintptr_t**>(this);
		reinterpret_cast<Fn>(pVTable[2])(this, &pResult, szClassName);
		return pResult;
	}
};

// ISchemaSystem virtual function indices
namespace SchemaVFuncs
{
	inline constexpr std::size_t FIND_TYPE_SCOPE_FOR_MODULE = PATTERNS::VTABLE::SCHEMA::FIND_TYPE_SCOPE;
}

// CUtlTSHash bucket structure (40 bytes per bucket, 256 buckets)
// Linked list node in the hash table
struct HashFixedData_t
{
	std::uint32_t       uiKey;    // 0x00
	std::uint32_t       _pad;     // 0x04
	HashFixedData_t*    pNext;    // 0x08
	CSchemaClassBinding* pData;   // 0x10
};

// Single hash bucket (contains mutex/internals + two linked list heads)
struct HashBucket_t
{
	std::uint8_t      _pad0[0x18]; // 0x00 — mutex / lock internals
	HashFixedData_t*  pFirst;      // 0x18
	HashFixedData_t*  pFirstUncommited; // 0x20
	// total: 0x28 (40 bytes)
};

// ============================================================================
// internal storage
// ============================================================================

// classHash -> (fieldHash -> offset)
static std::unordered_map<FNV1A_t, std::unordered_map<FNV1A_t, std::uint32_t>> s_mapSchemaOffsets;

// flat combined-hash -> offset (for "ClassName->fieldName" single-hash lookups)
static std::unordered_map<FNV1A_t, std::uint32_t> s_mapFlatOffsets;

// for debug dump: classHash -> className, fieldHash -> fieldName
struct DebugFieldInfo_t
{
	std::string szClassName;
	std::string szFieldName;
	std::uint32_t nOffset;
};
static std::vector<DebugFieldInfo_t> s_vecDebugFields;

static std::size_t s_nTotalClasses = 0;
static std::size_t s_nTotalFields  = 0;

// ============================================================================
// internal helpers
// ============================================================================

// get type scope for a module via ISchemaSystem virtual call
static CSchemaSystemTypeScope* FindTypeScopeForModule(const char* szModuleName)
{
	if (!I::SchemaSystem)
		return nullptr;

	// ISchemaSystem::FindTypeScopeForModule(const char* szModule, void*)
	using FindTypeScopeFn = CSchemaSystemTypeScope * (__thiscall*)(void*, const char*, void*);
	const auto pVTable = *reinterpret_cast<std::uintptr_t**>(I::SchemaSystem);
	const auto fn = reinterpret_cast<FindTypeScopeFn>(pVTable[SchemaVFuncs::FIND_TYPE_SCOPE_FOR_MODULE]);

	return fn(I::SchemaSystem, szModuleName, nullptr);
}

// SEH-safe pointer validation helper
static bool IsValidReadPtr(const void* ptr)
{
	__try
	{
		volatile auto test = *static_cast<const volatile char*>(ptr);
		(void)test;
		return true;
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		return false;
	}
}

// iterate declared classes in a type scope using internal CUtlTSHash bucket table
// The CSchemaSystemTypeScope stores class bindings in a CUtlTSHash at offset 0x588.
// The bucket array (256 buckets, each 0x28 bytes) starts at offset SCHEMA_HASH_TABLE (0x5C0).
// Each bucket has a linked list of HashFixedData_t entries containing CSchemaClassBinding*.
static bool IterateTypeScopeClasses(CSchemaSystemTypeScope* pScope)
{
	if (!pScope)
		return false;

	const auto uScopeAddr = reinterpret_cast<std::uintptr_t>(pScope);
	constexpr std::uintptr_t HASH_TABLE_OFFSET = PATTERNS::OFFSETS::SCHEMA_HASH_TABLE;
	constexpr int NUM_BUCKETS = 256;
	constexpr std::uintptr_t BUCKET_SIZE = sizeof(HashBucket_t); // 0x28

	const auto pBucketsBase = uScopeAddr + HASH_TABLE_OFFSET;

	// quick sanity check on the bucket base address
	if (!IsValidReadPtr(reinterpret_cast<const void*>(pBucketsBase)))
	{
		L_PRINT(LOG_ERROR) << _XS("  bucket base is not readable");
		return false;
	}

	int nProcessed = 0;
	int nBucketsWithEntries = 0;

	for (int i = 0; i < NUM_BUCKETS; ++i)
	{
		const auto pBucket = reinterpret_cast<HashBucket_t*>(pBucketsBase + i * BUCKET_SIZE);

		// iterate linked list from pFirstUncommited (committed/safe entries)
		auto pEntry = pBucket->pFirstUncommited;
		if (!pEntry)
			pEntry = pBucket->pFirst; // fallback to pFirst

		if (pEntry)
			++nBucketsWithEntries;

		for (; pEntry != nullptr; pEntry = pEntry->pNext)
		{
			if (!IsValidReadPtr(pEntry))
				break;

			CSchemaClassBinding* pBinding = pEntry->pData;
			if (!pBinding || !IsValidReadPtr(pBinding) || !pBinding->szBinaryName)
				continue;

			// use FindDeclaredClass (vtable index 2) to get full class info with fields
			SchemaClassInfoData_t* pClassInfo = pScope->FindDeclaredClass(pBinding->szBinaryName);
			if (!pClassInfo || !pClassInfo->szName)
				continue;

			const FNV1A_t nClassHash = FNV1A::Hash(pClassInfo->szName);
			auto& fieldMap = s_mapSchemaOffsets[nClassHash];

			if (pClassInfo->nFieldCount > 0 && pClassInfo->pFields)
			{
				for (std::int16_t f = 0; f < pClassInfo->nFieldCount; ++f)
				{
					const auto& field = pClassInfo->pFields[f];
					if (!field.szName)
						continue;

					const FNV1A_t nFieldHash = FNV1A::Hash(field.szName);
					const auto nOffset = static_cast<std::uint32_t>(field.nSingleInheritanceOffset);

					fieldMap[nFieldHash] = nOffset;

					// also store in flat map as "ClassName->fieldName"
					std::string combined = std::string(pClassInfo->szName) + "->" + field.szName;
					s_mapFlatOffsets[FNV1A::Hash(combined.c_str())] = nOffset;

					++s_nTotalFields;

					s_vecDebugFields.push_back({
						pClassInfo->szName,
						field.szName,
						nOffset
					});
				}
			}

			++s_nTotalClasses;
			++nProcessed;

			if (nProcessed > 100000) // safety limit
				break;
		}

		if (nProcessed > 100000)
			break;
	}

	L_PRINT(LOG_INFO) << _XS("  iterated ") << nProcessed << _XS(" classes from ")
		<< nBucketsWithEntries << _XS("/") << NUM_BUCKETS << _XS(" buckets");

	return nProcessed > 0;
}

// ============================================================================
// public API
// ============================================================================

bool SCHEMA::Setup()
{
	L_PRINT(LOG_INFO) << _XS("--- dumping schema system ---");

	if (!I::SchemaSystem)
	{
		L_PRINT(LOG_ERROR) << _XS("ISchemaSystem is null, cannot dump schemas");
		return false;
	}

	s_mapSchemaOffsets.clear();
	s_mapFlatOffsets.clear();
	s_vecDebugFields.clear();
	s_nTotalClasses = 0;
	s_nTotalFields  = 0;

	// dump schemas from primary game modules
	const char* arrModules[] = {
		PATTERNS::MODULES::CLIENT,
		PATTERNS::MODULES::ENGINE2,
		PATTERNS::MODULES::SCHEMASYSTEM
	};

	for (const auto* szModule : arrModules)
	{
		DumpModule(szModule);
	}

	L_PRINT(LOG_INFO) << _XS("schema dump complete: ") << s_nTotalClasses << _XS(" classes, ")
		<< s_nTotalFields << _XS(" fields");

	if (s_nTotalFields == 0)
	{
		L_PRINT(LOG_WARNING) << _XS("no schema fields were dumped — offsets may not resolve correctly");
		L_PRINT(LOG_WARNING) << _XS("this could mean the hash table offsets need updating for this game build");
	}

	return true;
}

void SCHEMA::Destroy()
{
	s_mapSchemaOffsets.clear();
	s_mapFlatOffsets.clear();
	s_vecDebugFields.clear();
	s_nTotalClasses = 0;
	s_nTotalFields  = 0;

	L_PRINT(LOG_INFO) << _XS("schema data cleared");
}

std::uint32_t SCHEMA::GetOffset(FNV1A_t nClassHash, FNV1A_t nFieldHash)
{
	const auto itClass = s_mapSchemaOffsets.find(nClassHash);
	if (itClass == s_mapSchemaOffsets.end())
	{
		L_PRINT(LOG_ERROR) << _XS("schema class not found: hash=")
			<< L::AddFlags(LOG_MODE_INT_HEX | LOG_MODE_SHOWBASE) << nClassHash;
		return 0;
	}

	const auto itField = itClass->second.find(nFieldHash);
	if (itField == itClass->second.end())
	{
		L_PRINT(LOG_ERROR) << _XS("schema field not found: classHash=")
			<< L::AddFlags(LOG_MODE_INT_HEX | LOG_MODE_SHOWBASE) << nClassHash
			<< " fieldHash="
			<< L::AddFlags(LOG_MODE_INT_HEX | LOG_MODE_SHOWBASE) << nFieldHash;
		return 0;
	}

	return itField->second;
}

std::uint32_t SCHEMA::GetOffset(FNV1A_t nCombinedHash)
{
	const auto it = s_mapFlatOffsets.find(nCombinedHash);
	if (it == s_mapFlatOffsets.end())
	{
		L_PRINT(LOG_ERROR) << _XS("schema field not found (combined): hash=")
			<< L::AddFlags(LOG_MODE_INT_HEX | LOG_MODE_SHOWBASE) << nCombinedHash;
		return 0;
	}
	return it->second;
}

bool SCHEMA::DumpModule(const char* szModuleName)
{
	L_PRINT(LOG_INFO) << _XS("dumping schemas from: ") << szModuleName;

	CSchemaSystemTypeScope* pScope = FindTypeScopeForModule(szModuleName);
	if (!pScope)
	{
		L_PRINT(LOG_WARNING) << _XS("  failed to find type scope for ") << szModuleName;
		return false;
	}

	L_PRINT(LOG_INFO) << _XS("  type scope = ") << static_cast<const void*>(pScope);

	if (!IterateTypeScopeClasses(pScope))
	{
		L_PRINT(LOG_WARNING) << _XS("  failed to iterate classes in ") << szModuleName;
		return false;
	}

	return true;
}

void SCHEMA::DumpToFile(const char* szFilePath)
{
	std::ofstream file(szFilePath);
	if (!file.is_open())
	{
		L_PRINT(LOG_ERROR) << _XS("failed to open file for schema dump: ") << szFilePath;
		return;
	}

	file << "// GamerHack schema dump\n";
	file << "// Classes: " << s_nTotalClasses << "  Fields: " << s_nTotalFields << "\n\n";

	// sort by class name for readability
	std::sort(s_vecDebugFields.begin(), s_vecDebugFields.end(),
		[](const DebugFieldInfo_t& a, const DebugFieldInfo_t& b)
		{
			if (a.szClassName != b.szClassName)
				return a.szClassName < b.szClassName;
			return a.nOffset < b.nOffset;
		});

	std::string szLastClass;
	for (const auto& field : s_vecDebugFields)
	{
		if (field.szClassName != szLastClass)
		{
			if (!szLastClass.empty())
				file << "}\n\n";

			file << "class " << field.szClassName << " {\n";
			szLastClass = field.szClassName;
		}

		char szOffsetHex[16];
		snprintf(szOffsetHex, sizeof(szOffsetHex), "0x%04X", field.nOffset);
		file << "    " << szOffsetHex << " " << field.szFieldName << "\n";
	}

	if (!szLastClass.empty())
		file << "}\n";

	file.close();
	L_PRINT(LOG_INFO) << _XS("schema dump written to: ") << szFilePath;
}
