#pragma once
#include "../sdk/common.h"

/*
 * SCHEMA SYSTEM
 * resolves entity field offsets at runtime by dumping the game's
 * schema system (ISchemaSystem). Stores offsets as:
 *   unordered_map<FNV1A(className), unordered_map<FNV1A(fieldName), offset>>
 *
 * usage: SCHEMA_FIELD(int, m_iHealth, "C_BaseEntity", "m_iHealth");
 */

// ---------------------------------------------------------------
// schema field access macros
// ---------------------------------------------------------------

/// standard schema field — returns reference to field at runtime offset
#define SCHEMA_FIELD(type, varName, className, fieldName) \
	[[nodiscard]] std::add_lvalue_reference_t<type> varName() \
	{ \
		static const std::uint32_t nOffset = SCHEMA::GetOffset( \
			FNV1A::HashConst(className), FNV1A::HashConst(fieldName)); \
		return *reinterpret_cast<type*>(reinterpret_cast<std::uintptr_t>(this) + nOffset); \
	}

/// schema field returning pointer (for arrays / nested structs)
#define SCHEMA_FIELD_POINTER(type, varName, className, fieldName) \
	[[nodiscard]] std::add_pointer_t<type> varName() \
	{ \
		static const std::uint32_t nOffset = SCHEMA::GetOffset( \
			FNV1A::HashConst(className), FNV1A::HashConst(fieldName)); \
		return reinterpret_cast<type*>(reinterpret_cast<std::uintptr_t>(this) + nOffset); \
	}

/// schema field with extra byte offset added on top
#define SCHEMA_FIELD_OFFSET(type, varName, className, fieldName, extra) \
	[[nodiscard]] std::add_lvalue_reference_t<type> varName() \
	{ \
		static const std::uint32_t nOffset = SCHEMA::GetOffset( \
			FNV1A::HashConst(className), FNV1A::HashConst(fieldName)); \
		return *reinterpret_cast<type*>(reinterpret_cast<std::uintptr_t>(this) + nOffset + (extra)); \
	}

// ---------------------------------------------------------------
// schema namespace
// ---------------------------------------------------------------
namespace SCHEMA
{
	/* @section: lifecycle */
	bool Setup();
	void Destroy();

	/* @section: lookup */
	/// get the byte offset for a field, identified by hashed class and field names
	/// @returns: offset in bytes, or 0 on failure
	[[nodiscard]] std::uint32_t GetOffset(FNV1A_t nClassHash, FNV1A_t nFieldHash);

	/// get the byte offset for a field, identified by a single combined hash
	/// of the string "ClassName->fieldName"
	/// @returns: offset in bytes, or 0 on failure
	[[nodiscard]] std::uint32_t GetOffset(FNV1A_t nCombinedHash);

	/* @section: dumping */
	/// dump all schema classes and fields from a game module
	bool DumpModule(const char* szModuleName);

	/// write all dumped schemas to a human-readable file (for debugging)
	void DumpToFile(const char* szFilePath);
}
