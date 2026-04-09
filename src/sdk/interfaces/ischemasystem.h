#pragma once
#include <cstdint>

#include "../../utilities/memory.h"

// ---------------------------------------------------------------
// schema field data — describes a single field in a schema class
// ---------------------------------------------------------------
using SchemaString_t = const char*;
struct SchemaMetadataEntryData_t;
class CSchemaType;

struct SchemaClassFieldData_t
{
	SchemaString_t szName;                      // 0x0000
	CSchemaType* pSchemaType;                   // 0x0008
	std::uint32_t nSingleInheritanceOffset;     // 0x0010
	std::int32_t nMetadataSize;                 // 0x0014
	SchemaMetadataEntryData_t* pMetaData;       // 0x0018
};

// ---------------------------------------------------------------
// schema class info — runtime class descriptor
// ---------------------------------------------------------------
struct SchemaClassInfoData_t;

struct SchemaBaseClassInfoData_t
{
	std::int32_t nOffset;
	SchemaClassInfoData_t* pClass;
};

struct SchemaClassInfoData_t
{
private:
	void* pVtable;                              // 0x0000
public:
	const char* szName;                         // 0x0008
	char* szDescription;                        // 0x0010

	std::int32_t nSize;                         // 0x0018
	std::int16_t nFieldSize;                    // 0x001C
	std::int16_t nStaticSize;                   // 0x001E
	std::int16_t nMetadataSize;                 // 0x0020
	std::uint8_t nAlignOf;                      // 0x0022
	std::uint8_t nBaseClassesCount;             // 0x0023
	char pad0[0x4];                             // 0x0024
	SchemaClassFieldData_t* pFields;            // 0x0028
	char pad1[0x8];                             // 0x0030
	SchemaBaseClassInfoData_t* pBaseClasses;    // 0x0038
	char pad2[0x28];                            // 0x0040

	[[nodiscard]] bool InheritsFrom(SchemaClassInfoData_t* pClassInfo)
	{
		if (pClassInfo == this && pClassInfo != nullptr)
			return true;

		if (pBaseClasses == nullptr || pClassInfo == nullptr)
			return false;

		for (int i = 0; i < nBaseClassesCount; i++)
		{
			if (pBaseClasses[i].pClass->InheritsFrom(pClassInfo))
				return true;
		}
		return false;
	}
};

// ---------------------------------------------------------------
// schema class binding — links binary class name to its info
// ---------------------------------------------------------------
class CSchemaSystemTypeScope;

struct CSchemaClassBinding
{
	CSchemaClassBinding* pParent;
	const char* szBinaryName;                   // 0x08 — e.g. "C_BaseEntity"
	const char* szModuleName;                   // 0x10 — e.g. "client.dll"
	const char* szClassName;                    // 0x18 — e.g. "client"
	void* pClassInfoOldSynthesized;
	void* pClassInfo;
	void* pThisModuleBindingPointer;
	CSchemaType* pSchemaType;
};

// ---------------------------------------------------------------
// schema type — base type descriptor
// ---------------------------------------------------------------
class CSchemaType
{
public:
	bool GetSizes(int* pOutSize, std::uint8_t* pUnk)
	{
		return MEM::CallVFunc<bool, 3U>(this, pOutSize, pUnk);
	}

	bool GetSize(int* pOutSize)
	{
		std::uint8_t unk = 0;
		return GetSizes(pOutSize, &unk);
	}

	void* pVtable;                              // 0x0000
	const char* szName;                         // 0x0008
	CSchemaSystemTypeScope* pSystemTypeScope;   // 0x0010
	std::uint8_t nTypeCategory;                 // 0x0018
	std::uint8_t nAtomicCategory;               // 0x0019
};

// ---------------------------------------------------------------
// CSchemaSystemTypeScope — a module's schema type scope
// ---------------------------------------------------------------
class CSchemaSystemTypeScope
{
public:
	void FindDeclaredClass(SchemaClassInfoData_t** ppReturnClass, const char* szClassName)
	{
		// VFunc index 2
		return MEM::CallVFunc<void, 2U>(this, ppReturnClass, szClassName);
	}

	CSchemaType* FindSchemaTypeByName(const char* szName, std::uintptr_t* pSchema)
	{
		return MEM::CallVFunc<CSchemaType*, 4U>(this, szName, pSchema);
	}

	CSchemaType* FindTypeDeclaredClass(const char* szName)
	{
		return MEM::CallVFunc<CSchemaType*, 5U>(this, szName);
	}

	CSchemaType* FindTypeDeclaredEnum(const char* szName)
	{
		return MEM::CallVFunc<CSchemaType*, 6U>(this, szName);
	}

	CSchemaClassBinding* FindRawClassBinding(const char* szName)
	{
		return MEM::CallVFunc<CSchemaClassBinding*, 7U>(this, szName);
	}

	void* pVtable;                              // 0x0000
	char szName[256U];                          // 0x0008
};

// ---------------------------------------------------------------
// ISchemaSystem — top-level schema interface
// ---------------------------------------------------------------
class ISchemaSystem
{
public:
	// VFunc index 13
	CSchemaSystemTypeScope* FindTypeScopeForModule(const char* szModuleName)
	{
		return MEM::CallVFunc<CSchemaSystemTypeScope*, 13U>(this, szModuleName, nullptr);
	}
};
