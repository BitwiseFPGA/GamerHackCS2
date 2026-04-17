#pragma once
#include <cstdint>
#include <array>

#include "../../utilities/memory.h"
#include "../../utilities/xorstr.h"
#include "../datatypes/vector.h"
#include "../datatypes/color.h"

// ---------------------------------------------------------------
// ray
// ---------------------------------------------------------------
struct Ray_t
{
	Vector3 m_vecStart;
	Vector3 m_vecEnd;
	Vector3 m_vecMins;
	Vector3 m_vecMaxs;

private:
	std::uint8_t _pad0[0x4];
public:
	std::uint8_t m_nUnkType;
};

static_assert(sizeof(Ray_t) == 0x38);

// ---------------------------------------------------------------
// trace info (bullet penetration info)
// ---------------------------------------------------------------
struct CTraceInfo
{
	float         m_flUnk;
	float         m_flDistance;
	float         m_flDamage;
	std::uint32_t m_nPenCount;
	std::uint32_t m_nHandle;
	std::uint32_t m_nPenetrationFlags;
};

// ---------------------------------------------------------------
// hitbox — individual hitbox on a skeleton
// ---------------------------------------------------------------
class CHitBox
{
public:
	const char*   szName;
	const char*   szSurfaceProperty;
	const char*   szBoneName;
	Vector3       vecMinBounds;
	Vector3       vecMaxBounds;
	float         flShapeRadius;
	std::uint32_t uBoneNameHash;
	int           nGroupId;
	std::uint8_t  nShapeType;
	bool          bTranslationOnly;

private:
	std::uint8_t _padHB[0x2];

public:
	std::uint32_t uCRC;
	Color         colRender;
	std::uint16_t nHitBoxIndex;
};

// ---------------------------------------------------------------
// surface data
// ---------------------------------------------------------------
struct SurfaceData_t
{
private:
	std::uint8_t _pad0[0x8];
public:
	float m_flPenetrationModifier;
	float m_flDamageModifier;

private:
	std::uint8_t _pad1[0x4];
public:
	int m_iMaterial;
};

static_assert(sizeof(SurfaceData_t) == 0x18);

// ---------------------------------------------------------------
// game trace result (CGameTrace)
// ---------------------------------------------------------------
class C_CSPlayerPawn;

struct GameTrace_t
{
	GameTrace_t() = default;

	[[nodiscard]] bool DidHit() const { return m_flFraction < 1.0f || m_bStartSolid; }
	[[nodiscard]] bool IsVisible() const { return m_flFraction > 0.97f; }
	[[nodiscard]] int GetHitgroup() const { return m_pHitbox ? m_pHitbox->nGroupId : -1; }
	[[nodiscard]] int GetHitboxId() const { return m_pHitbox ? static_cast<int>(m_pHitbox->nHitBoxIndex) : -1; }

	void* m_pSurface;                // 0x00
	C_CSPlayerPawn* m_pHitEntity;    // 0x08
	CHitBox* m_pHitbox;              // 0x10

private:
	std::uint8_t _pad0[0x38];
public:
	std::uint32_t m_uContents;       // 0x50

private:
	std::uint8_t _pad1[0x24];
public:
	Vector3 m_vecStartPos;           // 0x78
	Vector3 m_vecEndPos;             // 0x84
	Vector3 m_vecNormal;             // 0x90
	Vector3 m_vecPosition;           // 0x9C

private:
	std::uint8_t _pad2[0x4];
public:
	float m_flFraction;              // 0xAC

private:
	std::uint8_t _pad3[0x6];
public:
	std::uint8_t m_nShapeType;       // 0xB6
	bool m_bStartSolid;              // 0xB7

private:
	std::uint8_t _pad4[0x50];
};

static_assert(sizeof(GameTrace_t) == 0x108);

// ---------------------------------------------------------------
// trace filter
// ---------------------------------------------------------------
struct TraceFilter_t
{
private:
	std::uint8_t _pad0[0x8];
public:
	std::int64_t m_uTraceMask;
	std::array<std::int64_t, 2> m_v1;
	std::array<std::int32_t, 4> m_arrSkipHandles;
	std::array<std::int16_t, 2> m_arrCollisions;
	std::int16_t m_v2;
	std::uint8_t m_v3;
	std::uint8_t m_v4;
	std::uint8_t m_v5;

	TraceFilter_t() = default;
	TraceFilter_t(std::uint64_t uMask, C_CSPlayerPawn* pSkip1, C_CSPlayerPawn* pSkip2, int nLayer);
};

static_assert(sizeof(TraceFilter_t) == 0x40);

// ---------------------------------------------------------------
// CGameTraceManager — trace / raycast system
// ---------------------------------------------------------------
class CGameTraceManager
{
public:
	bool TraceShape(Ray_t* pRay, Vector3 vecStart, Vector3 vecEnd, TraceFilter_t* pFilter, GameTrace_t* pGameTrace)
	{
		using fnTraceShape = bool(__fastcall*)(CGameTraceManager*, Ray_t*, Vector3*, Vector3*, TraceFilter_t*, GameTrace_t*);
		static fnTraceShape oTraceShape = []() -> fnTraceShape {
			// Primary: direct prolog scan (2026-04-03 build)
			auto uAddr = MEM::FindPattern(_XS("client.dll"), _XS("48 89 5C 24 ? 48 89 4C 24 ? 55 57"));
			if (uAddr) return reinterpret_cast<fnTraceShape>(uAddr);
			// Fallback: scan_absolute via E8 call site
			const auto uCallSite = MEM::FindPattern(_XS("client.dll"),
				_XS("E8 ? ? ? ? 80 7D ? ? 75 ? F3 0F 10 45 ? 48"));
			if (!uCallSite) return nullptr;
			const auto nRel = *reinterpret_cast<std::int32_t*>(uCallSite + 1);
			return reinterpret_cast<fnTraceShape>(
				uCallSite + 5 + static_cast<std::uintptr_t>(static_cast<std::ptrdiff_t>(nRel)));
		}();

		if (!oTraceShape)
			return false;

		return oTraceShape(this, pRay, &vecStart, &vecEnd, pFilter, pGameTrace);
	}

	bool ClipRayToEntity(Ray_t* pRay, Vector3 vecStart, Vector3 vecEnd, C_CSPlayerPawn* pPawn, TraceFilter_t* pFilter, GameTrace_t* pGameTrace)
	{
		using fnClipRayToEntity = bool(__fastcall*)(CGameTraceManager*, Ray_t*, Vector3*, Vector3*, C_CSPlayerPawn*, TraceFilter_t*, GameTrace_t*);
		// Updated pattern (2026-04-03 build)
		static auto oClipRayToEntity = reinterpret_cast<fnClipRayToEntity>(
			MEM::FindPattern(_XS("client.dll"),
				_XS("48 8B C4 48 89 58 ? 55 56 57 41 54 41 56 48 8D 68 ? 48 81 EC ? ? ? ? 48 8B 5D")));

		if (!oClipRayToEntity)
			return false;

		return oClipRayToEntity(this, pRay, &vecStart, &vecEnd, pPawn, pFilter, pGameTrace);
	}
};
