#include "trace.h"

#include "../core/interfaces.h"
#include "../sdk/functionlist.h"
#include "bones.h"
#include "log.h"
#include "memory.h"
#include "xorstr.h"
#include "../sdk/entity.h"
#include "../sdk/interfaces/cgameentitysystem.h"

// ---------------------------------------------------------------
// TraceData_t API structs (matching reference EngineTrace implementation)
// These are used internally — NOT exposed in the header.
// ---------------------------------------------------------------
namespace
{
	constexpr std::uint64_t TRACE_MASK_SHOT = 0x1C1003;

	// Each surface entry in the trace array
	struct trace_array_element_t { char data[0x30]; };

	// Per-surface bullet modulation data
	struct bullet_modulate_entry_t
	{
		float    startFrac;
		float    endFrac;
		float    damage;
		int      maxSecondaryTraces;
		uint16_t surfStart;
		uint16_t surfEnd;
		uint8_t  flags;
		uint8_t  pad[3];
	};

	struct bullet_mod_array_t
	{
		int    size;
		char   pad4[4];
		bullet_modulate_entry_t* data;
		char   pad16[8];
	};

	struct alignas(16) TraceData_t
	{
		char                      pad0[24];
		trace_array_element_t     m_Arr[0x80];   // 128 * 48 = 6144 bytes
		char                      pad6168[8];
		bullet_mod_array_t        mod_array;
		bullet_modulate_entry_t   mod_inline[8];
		Vector3                   tail_start;
		Vector3                   tail_end;
		char                      _pad6200[12];
	};

	// Opaque filter struct — game fills this in; must be >= 164 bytes
	struct alignas(16) CTraceFilter_Engine { char pad[176]; };

	// ---------------------------------------------------------------
	// Function pointer types
	// ---------------------------------------------------------------
	using fnInitTraceData_t = void(__fastcall*)(TraceData_t*);
	using fnInitTraceInfo_t = void(__fastcall*)(GameTrace_t*);
	using fnInitFilter_t    = void*(__fastcall*)(CTraceFilter_Engine*, uintptr_t pawn, std::uint64_t mask, int traceType, int filterType);
	using fnCreateTrace_t   = bool(__fastcall*)(TraceData_t*, Vector3 start, Vector3 delta, CTraceFilter_Engine*, int penCount, bool unk);
	using fnGetTraceInfo_t  = void(__fastcall*)(TraceData_t*, GameTrace_t*, float startFrac, void* surf);

	fnInitTraceData_t g_pfnInitTraceData = nullptr;
	fnInitTraceInfo_t g_pfnInitTraceInfo = nullptr;
	fnInitFilter_t    g_pfnInitFilter    = nullptr;
	fnCreateTrace_t   g_pfnCreateTrace   = nullptr;
	fnGetTraceInfo_t  g_pfnGetTraceInfo  = nullptr;

	// Global trace data buffer (avoids 6 KB stack alloc per call + _chkstk overhead)
	TraceData_t g_traceData{};

	bool g_bTraceReady = false;

	// ---------------------------------------------------------------
	// Helpers
	// ---------------------------------------------------------------
	C_BaseEntity* GetTraceHitEntity(const GameTrace_t& trace)
	{
		if (!trace.m_pHitEntity) return nullptr;
		return reinterpret_cast<C_BaseEntity*>(trace.m_pHitEntity);
	}

	bool DidClipTarget(const GameTrace_t& trace, C_CSPlayerPawn* pTargetPawn)
	{
		if (!pTargetPawn) return false;
		if (GetTraceHitEntity(trace) == reinterpret_cast<C_BaseEntity*>(pTargetPawn)) return true;
		if (trace.m_bStartSolid) return true;
		if (trace.m_pHitbox != nullptr) return true;
		return trace.m_flFraction > 0.0f && trace.m_flFraction < 1.0f;
	}
}

// ---------------------------------------------------------------
// setup / destroy
// ---------------------------------------------------------------
bool TRACE::Setup()
{
	// fnInitTraceData
	{
		auto uAddr = MEM::FindPattern(_XS("client.dll"),
			_XS("48 89 5C 24 ? 48 89 74 24 ? 57 48 83 EC 20 48 8D 79 ? 33 F6 C7 47"));
		if (uAddr)
		{
			g_pfnInitTraceData = reinterpret_cast<fnInitTraceData_t>(uAddr);
			L_PRINT(LOG_INFO) << _XS("[TRACE] fnInitTraceData = ") << reinterpret_cast<void*>(uAddr);
		}
		else
			L_PRINT(LOG_WARNING) << _XS("[TRACE] fnInitTraceData NOT FOUND");
	}

	// fnInitTraceInfo
	{
		auto uAddr = MEM::FindPattern(_XS("client.dll"), _XS("40 55 41 55 41 57 48 83 EC 30"));
		if (!uAddr)
			uAddr = MEM::FindPattern(_XS("client.dll"), _XS("40 55 41 55 41 57 48 83 EC"));
		if (uAddr)
		{
			g_pfnInitTraceInfo = reinterpret_cast<fnInitTraceInfo_t>(uAddr);
			L_PRINT(LOG_INFO) << _XS("[TRACE] fnInitTraceInfo = ") << reinterpret_cast<void*>(uAddr);
		}
		else
			L_PRINT(LOG_WARNING) << _XS("[TRACE] fnInitTraceInfo NOT FOUND");
	}

	// fnInitFilter
	{
		auto uAddr = MEM::FindPattern(_XS("client.dll"),
			_XS("48 89 5C 24 ? 48 89 74 24 ? 57 48 83 EC ? 0F B6 41 ? 33 FF 24 C9 C7 41 ?"));
		if (!uAddr)
			uAddr = MEM::FindPattern(_XS("client.dll"),
				_XS("48 89 5C 24 ? 48 89 74 24 ? 57 48 83 EC ? 0F B6 41 ? 33 FF 24"));
		if (uAddr)
		{
			g_pfnInitFilter = reinterpret_cast<fnInitFilter_t>(uAddr);
			L_PRINT(LOG_INFO) << _XS("[TRACE] fnInitFilter = ") << reinterpret_cast<void*>(uAddr);
		}
		else
			L_PRINT(LOG_WARNING) << _XS("[TRACE] fnInitFilter NOT FOUND");
	}

	// fnCreateTrace — note: EC 50 not EC 40
	{
		auto uAddr = MEM::FindPattern(_XS("client.dll"),
			_XS("48 89 5C 24 ? 48 89 6C 24 ? 48 89 74 24 ? 57 41 56 41 57 48 83 EC 50 F2 0F 10 02"));
		if (uAddr)
		{
			g_pfnCreateTrace = reinterpret_cast<fnCreateTrace_t>(uAddr);
			L_PRINT(LOG_INFO) << _XS("[TRACE] fnCreateTrace = ") << reinterpret_cast<void*>(uAddr);
		}
		else
			L_PRINT(LOG_WARNING) << _XS("[TRACE] fnCreateTrace NOT FOUND");
	}

	// fnGetTraceInfo
	{
		auto uAddr = MEM::FindPattern(_XS("client.dll"),
			_XS("48 89 5C 24 ? 48 89 6C 24 ? 48 89 74 24 ? 57 48 83 EC ? 48 8B E9 0F 29 74 24 ? 48 8B CA"));
		if (!uAddr)
			uAddr = MEM::FindPattern(_XS("client.dll"),
				_XS("48 89 5C 24 ? 48 89 6C 24 ? 48 89 74 24 ? 57 48 83 EC 60 48 8B E9 0F 29 74 24"));
		if (uAddr)
		{
			g_pfnGetTraceInfo = reinterpret_cast<fnGetTraceInfo_t>(uAddr);
			L_PRINT(LOG_INFO) << _XS("[TRACE] fnGetTraceInfo = ") << reinterpret_cast<void*>(uAddr);
		}
		else
			L_PRINT(LOG_WARNING) << _XS("[TRACE] fnGetTraceInfo NOT FOUND");
	}

	g_bTraceReady = (g_pfnInitTraceData && g_pfnInitTraceInfo && g_pfnInitFilter &&
	                 g_pfnCreateTrace && g_pfnGetTraceInfo);

	L_PRINT(LOG_INFO) << _XS("[TRACE] initialized — ready=") << g_bTraceReady
		<< _XS(" initData=")   << (g_pfnInitTraceData != nullptr)
		<< _XS(" initInfo=")   << (g_pfnInitTraceInfo != nullptr)
		<< _XS(" initFilter=") << (g_pfnInitFilter    != nullptr)
		<< _XS(" create=")     << (g_pfnCreateTrace   != nullptr)
		<< _XS(" getInfo=")    << (g_pfnGetTraceInfo  != nullptr);

	return true;
}

void TRACE::Destroy()
{
	L_PRINT(LOG_INFO) << _XS("[TRACE] destroyed");
}

// ---------------------------------------------------------------
// IsVisible — point-to-point visibility check (reference logic)
// fraction > 0.97 = clear shot; hit entity == target = direct hit
// ---------------------------------------------------------------
bool TRACE::IsVisible(const Vector3& vecStart, const Vector3& vecEnd,
                      C_CSPlayerPawn* pSkipEntity, C_BaseEntity* pTargetEntity)
{
	if (!g_bTraceReady)
		return false;

	GameTrace_t trace{};
	if (!TraceShape(vecStart, vecEnd, pSkipEntity, &trace))
		return false;

	if (trace.m_flFraction > 0.97f)
		return true;

	if (pTargetEntity && GetTraceHitEntity(trace) == pTargetEntity)
		return true;

	return false;
}

// ---------------------------------------------------------------
// IsBoneVisible — check visibility to a specific bone
// ---------------------------------------------------------------
bool TRACE::IsBoneVisible(C_CSPlayerPawn* pLocalPawn, C_CSPlayerPawn* pTargetPawn,
                          int nBoneIndex, Vector3* pOutBonePos)
{
	if (!pLocalPawn || !pTargetPawn)
		return false;

	Vector3 vecEyePos = pLocalPawn->GetEyePosition();
	if (vecEyePos.IsZero())
		return false;

	auto* pSceneNode = pTargetPawn->GetGameSceneNode();
	if (!pSceneNode)
		return false;

	auto* pSkeleton = pSceneNode->GetSkeletonInstance();
	if (!pSkeleton)
		return false;

	BONES::CalcWorldSpaceBones(pSkeleton, 0xFFFFF);

	Vector3 vecBonePos;
	if (!pSceneNode->GetBonePosition(nBoneIndex, vecBonePos) || vecBonePos.IsZero())
		return false;

	if (pOutBonePos)
		*pOutBonePos = vecBonePos;

	return IsVisible(vecEyePos, vecBonePos, pLocalPawn, reinterpret_cast<C_BaseEntity*>(pTargetPawn));
}

// ---------------------------------------------------------------
// IsPlayerVisible — check if any bone on target player is visible
// ---------------------------------------------------------------
bool TRACE::IsPlayerVisible(C_CSPlayerPawn* pLocalPawn, CCSPlayerController* pTargetController)
{
	if (!pLocalPawn || !pTargetController || !I::GameEntitySystem)
		return false;

	auto* pTargetPawn = I::GameEntitySystem->Get<C_CSPlayerPawn>(pTargetController->GetPlayerPawnHandle());
	if (!pTargetPawn)
		return false;

	for (int i = 0; i < NUM_VISCHECK_BONES; i++)
	{
		if (IsBoneVisible(pLocalPawn, pTargetPawn, arrVisCheckBones[i]))
			return true;
	}

	return false;
}

bool TRACE::ClipRayToPlayer(const Vector3& vecStart, const Vector3& vecEnd,
                             C_CSPlayerPawn* pSkipEntity, C_CSPlayerPawn* pTargetPawn,
                             GameTrace_t* pOutTrace)
{
	if (!I::GameTraceManager || !pTargetPawn)
		return false;

	Ray_t ray{};
	ray.m_vecStart = vecStart;
	ray.m_vecEnd = vecEnd;
	ray.m_vecMins = {};
	ray.m_vecMaxs = {};
	ray.m_nUnkType = 0;

	// ClipRayToEntity dereferences the filter (vtable call) — must NOT be nullptr
	alignas(16) CTraceFilter_Engine filterBuf{};
	TraceFilter_t* pFilter = nullptr;
	if (g_pfnInitFilter)
	{
		g_pfnInitFilter(&filterBuf, reinterpret_cast<uintptr_t>(pSkipEntity), TRACE_MASK_SHOT, 4, 7);
		pFilter = reinterpret_cast<TraceFilter_t*>(&filterBuf);
	}

	GameTrace_t clipTrace{};
	const bool bClipOk = I::GameTraceManager->ClipRayToEntity(&ray, vecStart, vecEnd, pTargetPawn, pFilter, &clipTrace);

	if (pOutTrace)
		*pOutTrace = clipTrace;

	const bool bResult = DidClipTarget(clipTrace, pTargetPawn);

	static std::uint32_t s_nClipCalls = 0;
	if (((++s_nClipCalls) % 16384) == 1)
	{
		L_PRINT(LOG_INFO) << _XS("[TRACE] ClipRayToPlayer#") << s_nClipCalls
			<< _XS(" fn=") << bClipOk
			<< _XS(" frac=") << clipTrace.m_flFraction
			<< _XS(" hit=") << static_cast<void*>(clipTrace.m_pHitEntity)
			<< _XS(" hbox=") << static_cast<void*>(clipTrace.m_pHitbox)
			<< _XS(" solid=") << static_cast<int>(clipTrace.m_bStartSolid)
			<< _XS(" r=") << bResult;
	}

	return bResult;
}

// ---------------------------------------------------------------
// LineGoesThroughSmoke — check if line crosses smoke volume
// ---------------------------------------------------------------
bool TRACE::LineGoesThroughSmoke(const Vector3& vecStart, const Vector3& vecEnd)
{
	using fnLineGoesThroughSmoke = bool(__cdecl*)(const Vector3&, const Vector3&);
	static auto oLineGoesThroughSmoke = reinterpret_cast<fnLineGoesThroughSmoke>(
		MEM::FindPattern(_XS("client.dll"),
			_XS("48 89 5C 24 ? 48 89 6C 24 ? 48 89 74 24 ? 57 41 56 41 57 48 83 EC ? 44 0F B6")));

	if (!oLineGoesThroughSmoke)
		return false;

	return oLineGoesThroughSmoke(vecStart, vecEnd);
}

// ---------------------------------------------------------------
// TraceShape — world trace using TraceData_t multi-step API
// (replaces old CGameTraceManager::TraceShape approach)
// ---------------------------------------------------------------
bool TRACE::TraceShape(const Vector3& vecStart, const Vector3& vecEnd,
                       C_CSPlayerPawn* pSkipEntity, GameTrace_t* pOutTrace)
{
	if (!pOutTrace)
		return false;

	*pOutTrace = {};
	pOutTrace->m_flFraction = 1.0f;

	if (!g_pfnInitTraceData || !g_pfnInitTraceInfo || !g_pfnInitFilter ||
	    !g_pfnCreateTrace   || !g_pfnGetTraceInfo)
		return false;

	static bool s_bVerified = false;
	static bool s_bBad      = false;

	if (s_bBad)
		return false;

	// Reset global trace buffer
	g_pfnInitTraceData(&g_traceData);

	// Build the trace filter
	CTraceFilter_Engine filter{};
	g_pfnInitFilter(&filter, reinterpret_cast<uintptr_t>(pSkipEntity), TRACE_MASK_SHOT, 4, 7);

	// delta = end - start (NOT end itself)
	Vector3 delta{ vecEnd.x - vecStart.x, vecEnd.y - vecStart.y, vecEnd.z - vecStart.z };

	if (!s_bVerified)
	{
		__try
		{
			g_pfnCreateTrace(&g_traceData, vecStart, delta, &filter, 4, true);
			s_bVerified = true;
			L_PRINT(LOG_INFO) << _XS("[TRACE] TraceData_t API — first call ok");
		}
		__except (EXCEPTION_EXECUTE_HANDLER)
		{
			s_bBad = true;
			L_PRINT(LOG_ERROR) << _XS("[TRACE] TraceData_t API crashed on first call — disabled");
			return false;
		}
	}
	else
	{
		g_pfnCreateTrace(&g_traceData, vecStart, delta, &filter, 4, true);
	}

	// Initialize the output trace struct
	g_pfnInitTraceInfo(pOutTrace);

	// Pull result from mod_array entry [0]
	const auto& arr = g_traceData.mod_array;
	if (arr.size > 0 && arr.data != nullptr)
	{
		const auto* entry    = &arr.data[0];
		const uint16_t surf_idx = entry->surfEnd & 0x7FFF;

		if (surf_idx < 0x80)
		{
			g_pfnGetTraceInfo(&g_traceData, pOutTrace, entry->startFrac,
			                  &g_traceData.m_Arr[surf_idx]);
			return true;
		}
	}

	// No surface hit — clear line, fraction stays 1.0
	pOutTrace->m_flFraction = 1.0f;
	return true;
}
