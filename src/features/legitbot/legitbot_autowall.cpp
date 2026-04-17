#include "legitbot_autowall.h"

#include "../../core/interfaces.h"
#include "../../core/variables.h"
#include "../../sdk/entity.h"
#include "../../sdk/functionlist.h"
#include "../../sdk/interfaces/cgameentitysystem.h"
#include "../../sdk/interfaces/cgametracemanager.h"
#include "../../utilities/log.h"
#include "../../utilities/math.h"
#include "../../utilities/memory.h"
#include "../../utilities/trace.h"
#include "../../utilities/xorstr.h"

#include <cmath>
#include <algorithm>

// ---------------------------------------------------------------
// trace data structures for bullet penetration (darkside-style)
// ---------------------------------------------------------------
struct TraceArrayElement_t
{
	std::uint8_t _pad[0x30];
};

struct TraceData_t
{
	std::int32_t m_nUnk1 = 0;
	float        m_flUnk2 = 52.0f;
	void*        m_pArrPointer = nullptr;
	std::int32_t m_nUnk3 = 128;
	std::int32_t m_nUnk4 = static_cast<std::int32_t>(0x80000000);
	std::array<TraceArrayElement_t, 0x80> m_arr{};
	std::uint8_t _pad1[0x8]{};
	std::int64_t m_nNumUpdate = 0;
	void*        m_pUpdateValues = nullptr;
	std::uint8_t _pad2[0xC8]{};
	Vector3      m_vecStart{};
	Vector3      m_vecEnd{};
	std::uint8_t _pad3[0x50]{};
};

struct UpdateValue_t
{
	float        m_flPreviousLengthMod;
	float        m_flCurrentLengthMod;
	std::uint8_t _pad1[0x8];
	short        m_nHandleIndex;
	std::uint8_t _pad2[0x6];
};

struct HandleBulletPenData_t
{
	float m_flDamage;
	float m_flPenetration;
	float m_flRangeModifier;
	float m_flRange;
	int   m_nPenCount;
	bool  m_bFailed;

	HandleBulletPenData_t(float dmg, float pen, float range, float rangeMod, int penCount, bool failed)
		: m_flDamage(dmg), m_flPenetration(pen), m_flRangeModifier(rangeMod),
		  m_flRange(range), m_nPenCount(penCount), m_bFailed(failed) {}
};

// ---------------------------------------------------------------
// game function pointers (resolved via pattern scan)
// ---------------------------------------------------------------
namespace
{
	// handle_bullet_penetration — game function that processes wall penetration
	using fnHandleBulletPen = bool(__fastcall*)(TraceData_t*, HandleBulletPenData_t*, UpdateValue_t*,
	                                            void*, void*, void*, void*, void*, bool);
	fnHandleBulletPen g_pHandleBulletPen = nullptr;

	// create_trace — builds trace data for bullet path
	using fnCreateTrace = void(__fastcall*)(TraceData_t*, Vector3, Vector3, TraceFilter_t,
	                                        void*, void*, void*, void*, int);
	fnCreateTrace g_pCreateTrace = nullptr;

	// init_trace_info — initialize a game_trace_t
	using fnInitTraceInfo = void(__fastcall*)(GameTrace_t*);
	fnInitTraceInfo g_pInitTraceInfo = nullptr;

	// get_trace_info — get trace result from trace data
	using fnGetTraceInfo = void(__fastcall*)(TraceData_t*, GameTrace_t*, float, void*);
	fnGetTraceInfo g_pGetTraceInfo = nullptr;

	// get surface data from trace surface pointer
	using fnGetSurfaceData = std::uint64_t(__fastcall*)(void*);
	fnGetSurfaceData g_pGetSurfaceData = nullptr;

	bool g_bAutoWallReady = false;
}

// ---------------------------------------------------------------
// Setup — resolve all patterns
// ---------------------------------------------------------------
bool F::LEGITBOT::AUTOWALL::Setup()
{
	// handle_bullet_penetration (darkside pattern — exact match)
	auto uAddr = MEM::FindPattern(_XS("client.dll"),
		_XS("48 8B C4 44 89 48 20 55 57 41 55"));
	if (uAddr)
	{
		g_pHandleBulletPen = reinterpret_cast<fnHandleBulletPen>(uAddr);
		L_PRINT(LOG_INFO) << _XS("[AUTOWALL] HandleBulletPenetration = ") << reinterpret_cast<void*>(uAddr);
	}
	else
		L_PRINT(LOG_WARNING) << _XS("[AUTOWALL] HandleBulletPenetration NOT FOUND");

	// create_trace (darkside pattern — exact match)
	uAddr = MEM::FindPattern(_XS("client.dll"),
		_XS("48 89 5C 24 ? 48 89 6C 24 ? 48 89 74 24 ? 57 41 56 41 57 48 83 EC 40 F2"));
	if (uAddr)
	{
		g_pCreateTrace = reinterpret_cast<fnCreateTrace>(uAddr);
		L_PRINT(LOG_INFO) << _XS("[AUTOWALL] CreateTrace = ") << reinterpret_cast<void*>(uAddr);
	}
	else
		L_PRINT(LOG_WARNING) << _XS("[AUTOWALL] CreateTrace NOT FOUND");

	// init_trace_info (darkside pattern — exact match)
	uAddr = MEM::FindPattern(_XS("client.dll"),
		_XS("48 89 5C 24 08 57 48 83 EC 20 48 8B D9 33 FF 48 8B 0D"));
	if (uAddr)
	{
		g_pInitTraceInfo = reinterpret_cast<fnInitTraceInfo>(uAddr);
		L_PRINT(LOG_INFO) << _XS("[AUTOWALL] InitTraceInfo = ") << reinterpret_cast<void*>(uAddr);
	}
	else
		L_PRINT(LOG_WARNING) << _XS("[AUTOWALL] InitTraceInfo NOT FOUND");

	// get_trace_info (darkside pattern — exact match)
	uAddr = MEM::FindPattern(_XS("client.dll"),
		_XS("48 89 5C 24 08 48 89 6C 24 10 48 89 74 24 18 57 48 83 EC 60 48 8B E9 0F"));
	if (uAddr)
	{
		g_pGetTraceInfo = reinterpret_cast<fnGetTraceInfo>(uAddr);
		L_PRINT(LOG_INFO) << _XS("[AUTOWALL] GetTraceInfo = ") << reinterpret_cast<void*>(uAddr);
	}
	else
		L_PRINT(LOG_WARNING) << _XS("[AUTOWALL] GetTraceInfo NOT FOUND");

	// get_surface_data (darkside pattern — exact match, E8 relative call)
	uAddr = MEM::FindPattern(_XS("client.dll"),
		_XS("E8 ? ? ? ? 48 85 C0 74 ? 44 38 60"));
	if (uAddr)
	{
		// resolve E8 relative call
		auto nRelative = *reinterpret_cast<std::int32_t*>(uAddr + 1);
		g_pGetSurfaceData = reinterpret_cast<fnGetSurfaceData>(uAddr + 5 + nRelative);
		L_PRINT(LOG_INFO) << _XS("[AUTOWALL] GetSurfaceData = ") << reinterpret_cast<void*>(g_pGetSurfaceData);
	}
	else
		L_PRINT(LOG_WARNING) << _XS("[AUTOWALL] GetSurfaceData NOT FOUND");

	g_bAutoWallReady = (g_pCreateTrace && g_pInitTraceInfo && g_pGetTraceInfo);
	L_PRINT(LOG_INFO) << _XS("[AUTOWALL] initialized — ready=") << g_bAutoWallReady
		<< _XS(" bulletPen=") << (g_pHandleBulletPen != nullptr);
	return true;
}

void F::LEGITBOT::AUTOWALL::Destroy()
{
	L_PRINT(LOG_INFO) << _XS("[AUTOWALL] destroyed");
}

// ---------------------------------------------------------------
// ScaleDamage — apply hitgroup multiplier + armor reduction
// ---------------------------------------------------------------
void F::LEGITBOT::AUTOWALL::ScaleDamage(int nHitGroup, C_CSPlayerPawn* pTarget,
                                         CCSWeaponBaseVData* pWeaponData, float& flDamage)
{
	if (!pTarget || !pWeaponData)
		return;

	// team-based scaling defaults (mp_damage_scale convars)
	float flHeadScale = 1.0f;
	float flBodyScale = 1.0f;

	switch (nHitGroup)
	{
	case 1: // HITGROUP_HEAD
		flDamage *= pWeaponData->GetHeadshotMultiplier() * flHeadScale;
		break;
	case 2: // HITGROUP_CHEST
	case 4: // HITGROUP_LEFTARM
	case 5: // HITGROUP_RIGHTARM
	case 8: // HITGROUP_NECK
		flDamage *= flBodyScale;
		break;
	case 3: // HITGROUP_STOMACH
		flDamage *= 1.25f * flBodyScale;
		break;
	case 6: // HITGROUP_LEFTLEG
	case 7: // HITGROUP_RIGHTLEG
		flDamage *= 0.75f * flBodyScale;
		break;
	default:
		break;
	}

	// armor reduction
	if (!pTarget->HasArmor(nHitGroup))
		return;

	float flArmorRatio = pWeaponData->GetArmorRatio() * 0.5f;
	float flArmorBonus = 0.5f;

	float flDamageToHealth = flDamage * flArmorRatio;
	float flDamageToArmor = (flDamage - flDamageToHealth) * flArmorBonus;

	if (flDamageToArmor > static_cast<float>(pTarget->GetArmorValue()))
		flDamageToHealth = flDamage - static_cast<float>(pTarget->GetArmorValue()) / flArmorBonus;

	flDamage = flDamageToHealth;
}

// ---------------------------------------------------------------
// FireBullet — simulate full bullet path with penetration
// ---------------------------------------------------------------
bool F::LEGITBOT::AUTOWALL::FireBullet(const Vector3& vecStart, const Vector3& vecEnd,
                                        C_CSPlayerPawn* pTarget, CCSWeaponBaseVData* pWeaponData,
                                        PenetrationData_t& penData, bool bIsTaser)
{
	if (!g_bAutoWallReady || !pWeaponData)
		return false;

	auto* pLocalController = SDK_FUNC::GetLocalPlayerController ?
		SDK_FUNC::GetLocalPlayerController(-1) : nullptr;
	if (!pLocalController)
		return false;

	auto* pLocalPawn = I::GameEntitySystem->Get<C_CSPlayerPawn>(pLocalController->GetPlayerPawnHandle());
	if (!pLocalPawn)
		return false;

	__try
	{
		// build trace filter — oversized buffer to prevent stack corruption
		alignas(16) std::uint8_t filterBuf[0x100]{};
		auto* pFilter = reinterpret_cast<TraceFilter_t*>(filterBuf);

		// Try using the game's constructor
		if (SDK_FUNC::CTraceFilter_Constructor)
		{
			SDK_FUNC::CTraceFilter_Constructor(reinterpret_cast<CTraceFilter*>(pFilter),
				pLocalPawn, 0x1C300B, 3, 15);
		}
		else
		{
			// manual fallback
			pFilter->m_uTraceMask = 0x1C300B;
			pFilter->m_v3 = 3;

			CBaseHandle hSkip = pLocalPawn->GetRefEHandle();
			if (hSkip.IsValid())
			{
				pFilter->m_arrSkipHandles[0] = static_cast<std::int32_t>(hSkip.GetRawIndex());
				pFilter->m_arrSkipHandles[1] = static_cast<std::int32_t>(hSkip.GetRawIndex());
			}
		}

		// calculate direction and end pos scaled by weapon range
		Vector3 vecDir = vecEnd - vecStart;
		float flLen = std::sqrtf(vecDir.x * vecDir.x + vecDir.y * vecDir.y + vecDir.z * vecDir.z);
		if (flLen > 0.0f)
		{
			vecDir.x /= flLen;
			vecDir.y /= flLen;
			vecDir.z /= flLen;
		}

		float flRange = pWeaponData->GetRange();
		Vector3 vecEndPos = vecStart + vecDir * flRange;

		// create trace data
		TraceData_t traceData{};
		traceData.m_pArrPointer = &traceData.m_arr;

		g_pCreateTrace(&traceData, vecStart, vecEndPos, *pFilter, nullptr, nullptr, nullptr, nullptr, 4);

		// setup penetration data
		HandleBulletPenData_t bulletData(
			static_cast<float>(pWeaponData->GetDamage()),
			pWeaponData->GetPenetration(),
			flRange,
			pWeaponData->GetRangeModifier(),
			4,
			false
		);

		float flMaxRange = flRange;
		float flTraceLength = 0.0f;
		penData.m_flDamage = static_cast<float>(pWeaponData->GetDamage());

		for (int i = 0; i < traceData.m_nNumUpdate; i++)
		{
			auto* pModValues = reinterpret_cast<UpdateValue_t*>(
				reinterpret_cast<std::uintptr_t>(traceData.m_pUpdateValues) + i * sizeof(UpdateValue_t));

			GameTrace_t gameTrace{};
			g_pInitTraceInfo(&gameTrace);
			g_pGetTraceInfo(&traceData, &gameTrace, 0.0f,
				reinterpret_cast<void*>(
					reinterpret_cast<std::uintptr_t>(traceData.m_arr.data()) +
					sizeof(TraceArrayElement_t) * (pModValues->m_nHandleIndex & ENT_ENTRY_MASK)));

			flMaxRange -= flTraceLength;

			if (gameTrace.m_flFraction == 1.0f)
				break;

			flTraceLength += gameTrace.m_flFraction * flMaxRange;
			penData.m_flDamage *= std::powf(pWeaponData->GetRangeModifier(), flTraceLength / 500.0f);

			if (flTraceLength > 3000.0f)
				break;

			if (gameTrace.m_pHitEntity && reinterpret_cast<C_BaseEntity*>(gameTrace.m_pHitEntity)->IsPlayerPawn())
			{
				if (penData.m_flDamage < 1.0f)
					continue;

				int nHitGroup = gameTrace.GetHitgroup();
				ScaleDamage(nHitGroup, gameTrace.m_pHitEntity, pWeaponData, penData.m_flDamage);

				penData.m_nHitGroup = nHitGroup;
				penData.m_nHitbox = gameTrace.GetHitboxId();
				penData.m_bPenetrated = (i == 0);

				if (bIsTaser && penData.m_bPenetrated)
					break;

				return true;
			}

			// try to penetrate the wall
			if (g_pHandleBulletPen)
			{
				if (g_pHandleBulletPen(&traceData, &bulletData, pModValues,
					nullptr, nullptr, nullptr, nullptr, nullptr, false))
					return false; // penetration failed

				penData.m_flDamage = bulletData.m_flDamage;
			}
			else
			{
				return false; // can't penetrate without the function
			}
		}
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		static bool bLogged = false;
		if (!bLogged)
		{
			bLogged = true;
			L_PRINT(LOG_ERROR) << _XS("[AUTOWALL] EXCEPTION in FireBullet");
		}
	}

	return false;
}

// ---------------------------------------------------------------
// GetDamageToTarget — convenience: compute damage from local eye to target pos
// ---------------------------------------------------------------
float F::LEGITBOT::AUTOWALL::GetDamageToTarget(C_CSPlayerPawn* pLocalPawn, C_CSPlayerPawn* pTarget,
                                                 const Vector3& vecTargetPos)
{
	if (!pLocalPawn || !pTarget || !g_bAutoWallReady)
		return 0.0f;

	auto* pWeaponServices = pLocalPawn->GetWeaponServices();
	if (!pWeaponServices)
		return 0.0f;

	CBaseHandle hWeapon = pWeaponServices->GetActiveWeapon();
	if (!hWeapon.IsValid() || !I::GameEntitySystem)
		return 0.0f;

	auto* pWeapon = I::GameEntitySystem->Get<C_CSWeaponBase>(hWeapon);
	if (!pWeapon)
		return 0.0f;

	auto* pAttrMgr = pWeapon->GetAttributeManager();
	if (!pAttrMgr)
		return 0.0f;

	auto* pItemView = pAttrMgr->GetItem();
	if (!pItemView)
		return 0.0f;

	auto* pWeaponData = pItemView->GetBasePlayerWeaponVData();
	if (!pWeaponData)
		return 0.0f;

	Vector3 vecEyePos = pLocalPawn->GetEyePosition();
	if (vecEyePos.IsZero())
		return 0.0f;

	PenetrationData_t penData;
	if (FireBullet(vecEyePos, vecTargetPos, pTarget, pWeaponData, penData))
		return penData.m_flDamage;

	return 0.0f;
}
