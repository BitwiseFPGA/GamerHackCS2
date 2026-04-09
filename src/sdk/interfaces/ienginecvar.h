#pragma once
#include <cstdint>

#include "../../utilities/fnv1a.h"
#include "../datatypes/color.h"
#include "../datatypes/vector.h"
#include "../datatypes/qangle.h"

// ---------------------------------------------------------------
// convar flags
// ---------------------------------------------------------------
enum EConVarFlag : int
{
	FCVAR_NONE                  = 0,
	FCVAR_UNREGISTERED          = (1 << 0),
	FCVAR_DEVELOPMENTONLY       = (1 << 1),
	FCVAR_GAMEDLL               = (1 << 2),
	FCVAR_CLIENTDLL             = (1 << 3),
	FCVAR_HIDDEN                = (1 << 4),
	FCVAR_PROTECTED             = (1 << 5),
	FCVAR_SPONLY                = (1 << 6),
	FCVAR_ARCHIVE               = (1 << 7),
	FCVAR_NOTIFY                = (1 << 8),
	FCVAR_USERINFO              = (1 << 9),
	FCVAR_PRINTABLEONLY         = (1 << 10),
	FCVAR_UNLOGGED              = (1 << 11),
	FCVAR_NEVER_AS_STRING       = (1 << 12),
	FCVAR_REPLICATED            = (1 << 13),
	FCVAR_CHEAT                 = (1 << 14),
	FCVAR_DEMO                  = (1 << 16),
	FCVAR_DONTRECORD            = (1 << 17),
	FCVAR_RELOAD_MATERIALS      = (1 << 20),
	FCVAR_RELOAD_TEXTURES       = (1 << 21),
	FCVAR_NOT_CONNECTED         = (1 << 22),
	FCVAR_MATERIAL_SYSTEM_THREAD = (1 << 23),
	FCVAR_ARCHIVE_XBOX          = (1 << 24),
	FCVAR_ACCESSIBLE_FROM_THREADS = (1 << 25),
	FCVAR_SERVER_CAN_EXECUTE    = (1 << 28),
	FCVAR_SERVER_CANNOT_QUERY   = (1 << 29),
	FCVAR_CLIENTCMD_CAN_EXECUTE = (1 << 30)
};

// ---------------------------------------------------------------
// convar type
// ---------------------------------------------------------------
enum EConVarType : short
{
	EConVarType_Invalid = -1,
	EConVarType_Bool,
	EConVarType_Int16,
	EConVarType_UInt16,
	EConVarType_Int32,
	EConVarType_UInt32,
	EConVarType_Int64,
	EConVarType_UInt64,
	EConVarType_Float32,
	EConVarType_Float64,
	EConVarType_String,
	EConVarType_Color,
	EConVarType_Vector2,
	EConVarType_Vector3,
	EConVarType_Vector4,
	EConVarType_Qangle,
	EConVarType_MAX
};

// ---------------------------------------------------------------
// convar value union
// ---------------------------------------------------------------
union CVValue_t
{
	bool i1;
	short i16;
	std::uint16_t u16;
	int i32;
	std::uint32_t u32;
	std::int64_t i64;
	std::uint64_t u64;
	float fl;
	double db;
	const char* sz;
	Color clr;
	Vector2D vec2;
	Vector3 vec3;
	Vector4 vec4;
	QAngle ang;
};

// ---------------------------------------------------------------
// CConVar
// ---------------------------------------------------------------
class CConVar
{
public:
	const char* szName;             // 0x0000
	CConVar* m_pNext;               // 0x0008

private:
	std::uint8_t _pad0[0x10];      // 0x0010
public:
	const char* szDescription;      // 0x0020
	std::uint32_t nType;            // 0x0028
	std::uint32_t nRegistered;      // 0x002C
	std::uint32_t nFlags;           // 0x0030

private:
	std::uint8_t _pad1[0x8];       // 0x0034
public:
	CVValue_t value;                // 0x0040
};

// ---------------------------------------------------------------
// IEngineCVar — console variable system
// ---------------------------------------------------------------
class IEngineCVar
{
public:
	/// find a convar by hashed name (iterates internal linked list)
	CConVar* Find(FNV1A_t uHashedName)
	{
		for (auto* pCur = m_pFirst; pCur != nullptr; pCur = pCur->m_pNext)
		{
			if (FNV1A::Hash(pCur->szName) == uHashedName)
				return pCur;
		}
		return nullptr;
	}

	/// remove hidden / dev-only flags from all convars
	void UnlockHiddenCVars()
	{
		for (auto* pCur = m_pFirst; pCur != nullptr; pCur = pCur->m_pNext)
		{
			if (pCur->nFlags & FCVAR_HIDDEN)
				pCur->nFlags &= ~FCVAR_HIDDEN;

			if (pCur->nFlags & FCVAR_DEVELOPMENTONLY)
				pCur->nFlags &= ~FCVAR_DEVELOPMENTONLY;
		}
	}

private:
	std::uint8_t _pad0[0x40];
	CConVar* m_pFirst;
};
