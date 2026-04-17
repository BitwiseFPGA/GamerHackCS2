#pragma once
#include "../sdk/common.h"
#include <array>

/*
 * SCHEMA SYSTEM — Andromeda-style full hash table iteration
 *
 * Resolves entity field offsets at runtime by walking the game's
 * CSchemaSystem 256-bucket hash table (CUtlTSHash) for each type scope.
 *
 * Approach:
 *   1. Get GlobalTypeScope via VMT index 11
 *   2. Get all type scopes via pattern-based GetAllTypeScope
 *   3. Walk 256-bucket hash table at offset 0x5C0 from each scope
 *   4. Extract CSchemaClassBinding → field arrays → cache offsets
 *   5. Fallback to hardcoded offsets if runtime dump fails
 *
 * Storage:
 *   - FNV1A flat map: Hash("ClassName->fieldName") → offset  (for entity.h macros)
 *   - FNV1A nested map: Hash(className) → Hash(fieldName) → offset
 *   - String map: className → fieldName → offset  (for debugging/Andromeda-style access)
 */

// ---------------------------------------------------------------
// Andromeda-style schema structures (in SchemaInternal namespace
// to avoid ODR conflicts with ischemasystem.h definitions)
// ---------------------------------------------------------------
namespace SchemaInternal
{

struct SchemaFieldData_t
{
	char*  FieldName;   // 0x00
	void*  FieldType;   // 0x08
	int    FieldOffset; // 0x10
private:
	std::byte _pad[0xC]; // 0x14
};

class ClassBinding
{
public:
	const char*       GetName()          const { return *reinterpret_cast<const char* const*>(reinterpret_cast<std::uintptr_t>(this) + 0x8); }
	const char*       GetDllName()       const { return *reinterpret_cast<const char* const*>(reinterpret_cast<std::uintptr_t>(this) + 0x10); }
	int               GetSizeOf()        const { return *reinterpret_cast<const int*>(reinterpret_cast<std::uintptr_t>(this) + 0x18); }
	unsigned short    GetDataArraySize() const { return *reinterpret_cast<const unsigned short*>(reinterpret_cast<std::uintptr_t>(this) + 0x1C); }
	SchemaFieldData_t* GetDataArray()    const { return *reinterpret_cast<SchemaFieldData_t* const*>(reinterpret_cast<std::uintptr_t>(this) + 0x28); }
};

template <class T = ClassBinding>
class SchemaList
{
public:
	class SchemaBlock
	{
	public:
		SchemaBlock* Next()       const { return m_nextBlock; }
		T*           GetBinding() const { return m_classBinding; }
	private:
		void*        unkn0;
		SchemaBlock* m_nextBlock;
		T*           m_classBinding;
	};

	class BlockContainer
	{
	public:
		typename SchemaList<T>::SchemaBlock* GetFirstBlock() const { return m_firstBlock; }
	private:
		void*                               unkn[2];
		typename SchemaList<T>::SchemaBlock* m_firstBlock;
	};

	using BlockContainers = std::array<BlockContainer, 256>;

	int GetNumSchema()
	{
		return *reinterpret_cast<int*>(reinterpret_cast<std::uintptr_t>(this) - 0x74);
	}

	const BlockContainers& GetBlockContainers()
	{
		return *reinterpret_cast<const BlockContainers*>(reinterpret_cast<std::uintptr_t>(this));
	}
};

class TypeScope
{
public:
	const char* GetModuleName()
	{
		return reinterpret_cast<const char*>(reinterpret_cast<std::uintptr_t>(this) + 0x08);
	}

	SchemaList<ClassBinding>* GetClassContainer()
	{
		return reinterpret_cast<SchemaList<ClassBinding>*>(
			reinterpret_cast<std::uintptr_t>(this) + 0x5C0);
	}

	// VMT[2] FindDeclaredClass (fallback approach)
	void* FindDeclaredClass(const char* szClassName)
	{
		void* pResult = nullptr;
		using Fn = void(__thiscall*)(void*, void**, const char*);
		const auto pVTable = *reinterpret_cast<std::uintptr_t**>(this);
		reinterpret_cast<Fn>(pVTable[2])(this, &pResult, szClassName);
		return pResult;
	}
};

} // namespace SchemaInternal

// ---------------------------------------------------------------
// schema namespace
// ---------------------------------------------------------------
namespace SCHEMA
{
	/* @section: lifecycle */
	bool Setup();
	void Destroy();

	/* @section: lookup */
	/// get offset from FNV1A hash of "ClassName->fieldName" (used by entity.h macros)
	[[nodiscard]] std::uint32_t GetOffset(FNV1A_t nCombinedHash);

	/// get offset from separate class and field hashes
	[[nodiscard]] std::uint32_t GetOffset(FNV1A_t nClassHash, FNV1A_t nFieldHash);

	/// get offset from string class/field names (Andromeda-style)
	[[nodiscard]] std::uint32_t GetOffset(const char* szClassName, const char* szFieldName);

	/* @section: stats */
	[[nodiscard]] std::size_t GetTotalClasses();
	[[nodiscard]] std::size_t GetTotalFields();

	/* @section: dumping */
	void DumpToFile(const char* szFilePath);
}
