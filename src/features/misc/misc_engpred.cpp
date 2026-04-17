#include "misc_engpred.h"

#include "../../core/interfaces.h"
#include "../../core/variables.h"
#include "../../sdk/entity.h"
#include "../../sdk/functionlist.h"
#include "../../sdk/interfaces/ccsgoinput.h"
#include "../../sdk/interfaces/cgameentitysystem.h"
#include "../../sdk/interfaces/iengineclient.h"
#include "../../sdk/interfaces/iglobalvars.h"
#include "../../utilities/log.h"
#include "../../utilities/memory.h"
#include "../../utilities/xorstr.h"

// ---------------------------------------------------------------
// game functions for prediction
// ---------------------------------------------------------------
namespace
{
	using fnProcessMovement = void(__fastcall*)(void*, void*);
	fnProcessMovement g_pProcessMovement = nullptr;

	using fnInterpolateShootPos = void(__fastcall*)(void*, void*);
	fnInterpolateShootPos g_pInterpolateShootPos = nullptr;

	bool g_bPredReady = false;
	bool g_bPredActive = false;

	// saved state for restoration
	struct PredState_t
	{
		float  flAbsoluteFrameTime = 0.0f;
		float  flCurrentTime = 0.0f;
		float  flCurrentTime2 = 0.0f;
		int    nTickCount = 0;
		int    nTickBase = 0;
		bool   bValid = false;
	};

	PredState_t g_savedState{};
}

// ---------------------------------------------------------------
// Setup
// ---------------------------------------------------------------
bool F::MISC::ENGPRED::Setup()
{
	// process_movement (darkside pattern — exact match, E8 relative call)
	auto uAddr = MEM::FindPattern(_XS("client.dll"),
		_XS("E8 ? ? ? ? 48 8B F1 48 8B DA FF 90 ? ? ? ? 44 38 63"));
	if (uAddr)
	{
		// resolve relative call
		auto nRel = *reinterpret_cast<std::int32_t*>(uAddr + 1);
		g_pProcessMovement = reinterpret_cast<fnProcessMovement>(uAddr + 5 + nRel);
		L_PRINT(LOG_INFO) << _XS("[ENGPRED] ProcessMovement = ") << reinterpret_cast<void*>(g_pProcessMovement);
	}
	else
		L_PRINT(LOG_WARNING) << _XS("[ENGPRED] ProcessMovement NOT FOUND");

	// interpolate_shoot_position (darkside pattern — exact match, E8 relative call)
	uAddr = MEM::FindPattern(_XS("client.dll"),
		_XS("E8 ? ? ? ? 41 8B 86 ? ? ? ? C1 E8 ? A8 ? 0F 85 ? ? ? ? 48 8B CE"));
	if (uAddr)
	{
		auto nRel = *reinterpret_cast<std::int32_t*>(uAddr + 1);
		g_pInterpolateShootPos = reinterpret_cast<fnInterpolateShootPos>(uAddr + 5 + nRel);
		L_PRINT(LOG_INFO) << _XS("[ENGPRED] InterpolateShootPos = ") << reinterpret_cast<void*>(g_pInterpolateShootPos);
	}
	else
		L_PRINT(LOG_WARNING) << _XS("[ENGPRED] InterpolateShootPos NOT FOUND");

	g_bPredReady = (g_pProcessMovement != nullptr);
	L_PRINT(LOG_INFO) << _XS("[ENGPRED] initialized — ready=") << g_bPredReady
		<< _XS(" processMove=") << (g_pProcessMovement != nullptr)
		<< _XS(" interpShoot=") << (g_pInterpolateShootPos != nullptr);

	return true;
}

void F::MISC::ENGPRED::Destroy()
{
	L_PRINT(LOG_INFO) << _XS("[ENGPRED] destroyed");
}

// ---------------------------------------------------------------
// Run — save state and begin prediction
// ---------------------------------------------------------------
void F::MISC::ENGPRED::Run(CCSGOInput* pInput, CUserCmd* pCmd)
{
	g_bPredActive = false;
	g_savedState.bValid = false;

	if (!g_bPredReady || !pCmd)
		return;

	if (!I::Engine || !I::Engine->IsInGame())
		return;

	if (!I::GlobalVars || !I::GameEntitySystem)
		return;

	__try
	{
		auto* pLocalController = SDK_FUNC::GetLocalPlayerController ?
			SDK_FUNC::GetLocalPlayerController(-1) : nullptr;
		if (!pLocalController || !pLocalController->IsPawnAlive())
			return;

		auto* pLocalPawn = I::GameEntitySystem->Get<C_CSPlayerPawn>(pLocalController->GetPlayerPawnHandle());
		if (!pLocalPawn || !pLocalPawn->IsAlive())
			return;

		// save global vars state
		g_savedState.flAbsoluteFrameTime = I::GlobalVars->flFrameTime;
		g_savedState.flCurrentTime = I::GlobalVars->flCurrentTime;
		g_savedState.flCurrentTime2 = I::GlobalVars->flCurrentTime2;
		g_savedState.nTickCount = I::GlobalVars->nTickCount;
		g_savedState.bValid = true;

		g_bPredActive = true;

		static bool bLoggedFirst = false;
		if (!bLoggedFirst)
		{
			bLoggedFirst = true;
			L_PRINT(LOG_INFO) << _XS("[ENGPRED] first prediction run — frameTime=")
				<< I::GlobalVars->flFrameTime << _XS(" currentTime=") << I::GlobalVars->flCurrentTime;
		}
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		static bool bLogged = false;
		if (!bLogged)
		{
			bLogged = true;
			L_PRINT(LOG_ERROR) << _XS("[ENGPRED] EXCEPTION in Run");
		}
		g_bPredActive = false;
	}
}

// ---------------------------------------------------------------
// End — restore state and interpolate shoot position
// ---------------------------------------------------------------
void F::MISC::ENGPRED::End(CCSGOInput* pInput, CUserCmd* pCmd)
{
	if (!g_bPredActive || !g_savedState.bValid)
		return;

	__try
	{
		// restore global vars
		if (I::GlobalVars)
		{
			I::GlobalVars->flFrameTime = g_savedState.flAbsoluteFrameTime;
			I::GlobalVars->flCurrentTime = g_savedState.flCurrentTime;
			I::GlobalVars->flCurrentTime2 = g_savedState.flCurrentTime2;
			I::GlobalVars->nTickCount = g_savedState.nTickCount;
		}

		// interpolate shoot position for accuracy
		if (g_pInterpolateShootPos)
		{
			auto* pLocalController = SDK_FUNC::GetLocalPlayerController ?
				SDK_FUNC::GetLocalPlayerController(-1) : nullptr;
			if (pLocalController && pLocalController->IsPawnAlive())
			{
				auto* pLocalPawn = I::GameEntitySystem->Get<C_CSPlayerPawn>(
					pLocalController->GetPlayerPawnHandle());
				if (pLocalPawn)
					g_pInterpolateShootPos(pLocalPawn, nullptr);
			}
		}
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		static bool bLogged = false;
		if (!bLogged)
		{
			bLogged = true;
			L_PRINT(LOG_ERROR) << _XS("[ENGPRED] EXCEPTION in End");
		}
	}

	g_bPredActive = false;
	g_savedState.bValid = false;
}
