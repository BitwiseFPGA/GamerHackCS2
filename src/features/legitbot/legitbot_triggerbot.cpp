#include "legitbot_triggerbot.h"

#include "../../core/config.h"
#include "../../core/interfaces.h"
#include "../../core/variables.h"
#include "../../sdk/const.h"
#include "../../sdk/entity.h"
#include "../../sdk/functionlist.h"
#include "../../sdk/interfaces/ccsgoinput.h"
#include "../../sdk/interfaces/cgameentitysystem.h"
#include "../../sdk/interfaces/iengineclient.h"
#include "../../sdk/interfaces/iglobalvars.h"
#include "../../utilities/log.h"
#include "../../utilities/math.h"
#include "../../utilities/trace.h"
#include "../../utilities/xorstr.h"
#include "../bypass/bypass.h"

#include <cstring>
#include <random>

namespace
{
	enum class TriggerPhase : int
	{
		Idle = 0,
		Waiting,
		Holding
	};

	struct TriggerSelection
	{
		C_CSPlayerPawn* pPawn = nullptr;
		GameTrace_t worldTrace{};
		GameTrace_t hitTrace{};
		float flFraction = 2.0f;
	};

	TriggerPhase g_phase = TriggerPhase::Idle;
	float g_fireAt = 0.0f;
	int g_holdTicks = 0;
	bool g_mouseHeld = false;

	std::mt19937& GetTriggerRng()
	{
		static std::mt19937 rng(std::random_device{}());
		return rng;
	}

	bool IsKeyActive()
	{
		if (C::Get<bool>(triggerbot_always_on))
			return true;

		const int nKey = C::Get<int>(triggerbot_key);
		return nKey == 0 || (GetAsyncKeyState(nKey) & 0x8000) != 0;
	}

	bool IsManualAttackActive(CUserCmd* pCmd)
	{
		if (!pCmd)
			return false;

		if ((pCmd->nButtons.nValue & IN_ATTACK) != 0)
			return true;

		auto* pBaseCmd = pCmd->csgoUserCmd.pBaseCmd;
		return pBaseCmd && pBaseCmd->pInButtonState &&
			((pBaseCmd->pInButtonState->nValue & IN_ATTACK) != 0);
	}

	void HoldSyntheticMouse()
	{
		if (!g_mouseHeld)
		{
			mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
			g_mouseHeld = true;
		}
	}

	void ReleaseSyntheticMouse()
	{
		if (g_mouseHeld)
		{
			mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
			g_mouseHeld = false;
		}
	}

	void ResetTrigger(CUserCmd* pCmd)
	{
		if (g_phase == TriggerPhase::Holding && pCmd)
			F::BYPASS::SetDontAttack(pCmd, true);

		g_phase = TriggerPhase::Idle;
		g_fireAt = 0.0f;
		g_holdTicks = 0;
		ReleaseSyntheticMouse();
	}

	float BuildDelaySeconds()
	{
		float flDelay = C::Get<float>(triggerbot_delay);
		const float flRandom = C::Get<float>(triggerbot_delay_rand);

		if (flRandom > 0.0f)
		{
			std::uniform_real_distribution<float> dist(0.0f, flRandom);
			flDelay += dist(GetTriggerRng());
		}

		return flDelay / 1000.0f;
	}

	int BuildHoldTicks()
	{
		int nMin = C::Get<int>(triggerbot_hold_min);
		int nMax = C::Get<int>(triggerbot_hold_max);

		if (nMin < 1)
			nMin = 1;
		if (nMax < nMin)
			nMax = nMin;

		std::uniform_int_distribution<int> dist(nMin, nMax);
		return dist(GetTriggerRng());
	}

	bool HitgroupMatches(int nFilter, const GameTrace_t& trace)
	{
		if (nFilter == 0)
			return true;

		// trace results use trace_hitbox_data_t* — only nGroupId (at 0x38) is reliable.
		// Do NOT read szBoneName — that offset is padding in trace_hitbox_data_t.
		if (!trace.m_pHitbox)
			return false;

		const int nGroup = trace.GetHitgroup();
		if (nGroup <= 0)
			return false;

		switch (nFilter)
		{
		case 1:
			return nGroup == static_cast<int>(HITGROUP_HEAD);
		case 2:
			return nGroup == static_cast<int>(HITGROUP_CHEST) ||
				nGroup == static_cast<int>(HITGROUP_NECK);
		case 3:
			return nGroup == static_cast<int>(HITGROUP_STOMACH);
		default:
			return true;
		}
	}

	bool HasVisibleLineToTarget(const GameTrace_t& worldTrace, C_CSPlayerPawn* pTargetPawn)
	{
		if (!pTargetPawn)
			return false;

		if (worldTrace.IsVisible())
			return true;

		return reinterpret_cast<C_BaseEntity*>(worldTrace.m_pHitEntity) ==
			reinterpret_cast<C_BaseEntity*>(pTargetPawn);
	}

	bool IsTriggerCandidateValid(C_CSPlayerPawn* pLocalPawn, C_CSPlayerPawn* pTargetPawn,
	                             bool bTeamCheck, bool bVisibleOnly, int nHitgroupFilter,
	                             const GameTrace_t& worldTrace, const GameTrace_t& hitTrace)
	{
		if (!pTargetPawn || pTargetPawn == pLocalPawn || !pTargetPawn->IsAlive())
			return false;

		if (bTeamCheck && pTargetPawn->GetTeam() == pLocalPawn->GetTeam())
			return false;

		if (bVisibleOnly && !HasVisibleLineToTarget(worldTrace, pTargetPawn))
			return false;

		return HitgroupMatches(nHitgroupFilter, hitTrace);
	}

	bool AcquireSelection(C_CSPlayerPawn* pLocalPawn, const Vector3& vecEyePos, const QAngle& angView,
	                      bool bTeamCheck, bool bVisibleOnly, int nHitgroupFilter,
	                      TriggerSelection& outSelection)
	{
		Vector3 vecForward{};
		MATH::AngleVectors(angView, &vecForward);
		const Vector3 vecEnd = vecEyePos + vecForward * 8192.0f;

		GameTrace_t worldTrace{};
		const bool bWorldTraceOk = TRACE::TraceShape(vecEyePos, vecEnd, pLocalPawn, &worldTrace);

		// Direct hit path: world trace directly hit a player entity
		if (bWorldTraceOk && worldTrace.DidHit() && worldTrace.m_pHitEntity)
		{
			auto* pDirectEntity = reinterpret_cast<C_BaseEntity*>(worldTrace.m_pHitEntity);
			if (pDirectEntity->IsPlayerPawn())
			{
				auto* pDirectPawn = reinterpret_cast<C_CSPlayerPawn*>(pDirectEntity);
				if (IsTriggerCandidateValid(pLocalPawn, pDirectPawn, bTeamCheck,
					bVisibleOnly, nHitgroupFilter, worldTrace, worldTrace))
				{
					outSelection.pPawn = pDirectPawn;
					outSelection.worldTrace = worldTrace;
					outSelection.hitTrace = worldTrace;
					outSelection.flFraction = worldTrace.m_flFraction;
					return true;
				}
			}
		}

		// Clip scan: test each player's hitbox against the view ray
		const int nLocalTeam = static_cast<int>(pLocalPawn->GetTeam());

		for (int i = 1; i <= 64; ++i)
		{
			auto* pController = I::GameEntitySystem->Get<CCSPlayerController>(i);
			if (!pController || !pController->IsPawnAlive())
				continue;

			auto* pPawn = I::GameEntitySystem->Get<C_CSPlayerPawn>(pController->GetPlayerPawnHandle());
			if (!pPawn || pPawn == pLocalPawn || !pPawn->IsAlive())
				continue;

			if (bTeamCheck && static_cast<int>(pPawn->GetTeam()) == nLocalTeam)
				continue;

			GameTrace_t clipTrace{};
			if (!TRACE::ClipRayToPlayer(vecEyePos, vecEnd, pLocalPawn, pPawn, &clipTrace))
				continue;

			if (!IsTriggerCandidateValid(pLocalPawn, pPawn, false, bVisibleOnly,
				nHitgroupFilter, worldTrace, clipTrace))
				continue;

			if (clipTrace.m_flFraction < outSelection.flFraction)
			{
				outSelection.pPawn = pPawn;
				outSelection.worldTrace = worldTrace;
				outSelection.hitTrace = clipTrace;
				outSelection.flFraction = clipTrace.m_flFraction;
			}
		}

		return outSelection.pPawn != nullptr;
	}

	void ApplyTriggerShot(CUserCmd* pCmd)
	{
		F::BYPASS::SetAttack(pCmd, true);
		HoldSyntheticMouse();
	}
}

void F::LEGITBOT::TRIGGERBOT::OnCreateMove(CCSGOInput* pInput, CUserCmd* pCmd)
{
	if (!C::Get<bool>(triggerbot_enabled) || !pInput || !pCmd)
	{
		ResetTrigger(pCmd);
		return;
	}

	if (!I::Engine || !I::Engine->IsInGame() || !I::GameEntitySystem)
	{
		ResetTrigger(pCmd);
		return;
	}

	if (!IsKeyActive())
	{
		static std::uint32_t s_nKeyMiss = 0;
		if ((++s_nKeyMiss % 256) == 1)
			L_PRINT(LOG_INFO) << _XS("[TRIG] key NOT active (call#") << s_nKeyMiss << _XS(")");
		ResetTrigger(pCmd);
		return;
	}

	if (g_phase != TriggerPhase::Holding && IsManualAttackActive(pCmd))
	{
		ResetTrigger(pCmd);
		return;
	}

	auto* pLocalController = SDK_FUNC::GetLocalPlayerController ?
		SDK_FUNC::GetLocalPlayerController(-1) : nullptr;
	if (!pLocalController || !pLocalController->IsPawnAlive())
	{
		ResetTrigger(pCmd);
		return;
	}

	auto* pLocalPawn = I::GameEntitySystem->Get<C_CSPlayerPawn>(pLocalController->GetPlayerPawnHandle());
	if (!pLocalPawn || !pLocalPawn->IsAlive())
	{
		ResetTrigger(pCmd);
		return;
	}

	const Vector3 vecEyePos = pLocalPawn->GetEyePosition();
	if (vecEyePos.IsZero())
	{
		ResetTrigger(pCmd);
		return;
	}

	if (g_phase == TriggerPhase::Holding)
	{
		ApplyTriggerShot(pCmd);
		--g_holdTicks;

		static bool bLoggedHold = false;
		if (!bLoggedHold) { bLoggedHold = true;
			L_PRINT(LOG_INFO) << _XS("[TRIG] HOLDING — injected attack, ticks left=") << g_holdTicks;
		}

		if (g_holdTicks <= 0)
			ResetTrigger(pCmd);

		return;
	}

	TriggerSelection selection{};
	const bool bGotSelection = AcquireSelection(pLocalPawn, vecEyePos, pInput->GetViewAngles(),
		C::Get<bool>(triggerbot_team_check),
		C::Get<bool>(triggerbot_visible_only),
		C::Get<int>(triggerbot_hitgroup),
		selection);

	if (!bGotSelection)
	{
		if (g_phase != TriggerPhase::Idle)
		{
			L_PRINT(LOG_INFO) << _XS("[TRIG] no selection — resetting from phase ") << static_cast<int>(g_phase);
			ResetTrigger(pCmd);
		}
		return;
	}

	const float flNow = I::GlobalVars ? I::GlobalVars->flCurrentTime : 0.0f;

	if (g_phase == TriggerPhase::Idle)
	{
		g_fireAt = flNow + BuildDelaySeconds();
		g_phase = TriggerPhase::Waiting;
		L_PRINT(LOG_INFO) << _XS("[TRIG] Idle→Waiting, fire at t=") << g_fireAt << _XS(" now=") << flNow;
	}

	if (g_phase == TriggerPhase::Waiting && flNow < g_fireAt)
		return;

	g_holdTicks = BuildHoldTicks();
	g_phase = TriggerPhase::Holding;
	ApplyTriggerShot(pCmd);
	--g_holdTicks;

	L_PRINT(LOG_INFO) << _XS("[TRIG] FIRED — holdTicks=") << g_holdTicks
		<< _XS(" pawn=") << static_cast<void*>(selection.pPawn)
		<< _XS(" frac=") << selection.flFraction;

	if (g_holdTicks <= 0)
		ResetTrigger(pCmd);
}
