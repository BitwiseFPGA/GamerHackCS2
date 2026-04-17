#include "legitbot_targeting.h"

#include "legitbot_math.h"

#include "../../core/config.h"
#include "../../core/interfaces.h"
#include "../../core/variables.h"
#include "../../sdk/entity.h"
#include "../../sdk/interfaces/cgameentitysystem.h"
#include "../../sdk/interfaces/iglobalvars.h"
#include "../../utilities/bones.h"
#include "../../utilities/log.h"
#include "../../utilities/trace.h"
#include "../../utilities/xorstr.h"

#include <cmath>

namespace
{
	constexpr int kFallbackBones[] = { 6, 5, 4, 2, 0 };
	constexpr float kLockDuration = 0.20f;
	constexpr float kLockFovScale = 1.35f;
	constexpr float kLockScoreBias = 0.85f;

	CBaseHandle g_lockedTarget{};
	float g_lockedUntil = 0.0f;

	bool IsEnemyAllowed(C_CSPlayerPawn* pLocalPawn, C_CSPlayerPawn* pTargetPawn, bool bTeamCheck)
	{
		if (!pTargetPawn || pTargetPawn == pLocalPawn || !pTargetPawn->IsAlive())
			return false;

		if (!bTeamCheck)
			return true;

		return pTargetPawn->GetTeam() != pLocalPawn->GetTeam();
	}

	bool IsPointVisible(C_CSPlayerPawn* pLocalPawn, C_CSPlayerPawn* pTargetPawn,
	                    const Vector3& vecEyePos, const Vector3& vecPoint)
	{
		GameTrace_t worldTrace{};
		if (!TRACE::TraceShape(vecEyePos, vecPoint, pLocalPawn, &worldTrace))
			return false;

		if (worldTrace.IsVisible())
			return true;

		if (reinterpret_cast<C_BaseEntity*>(worldTrace.m_pHitEntity) ==
			reinterpret_cast<C_BaseEntity*>(pTargetPawn))
			return true;

		// m_pHitEntity can be null even when hitting a player (CS2 trace quirk).
		// Use ClipRayToPlayer as secondary confirmation when no entity was set.
		if (worldTrace.m_pHitEntity == nullptr && worldTrace.m_flFraction < 1.0f)
		{
			GameTrace_t clipTrace{};
			if (TRACE::ClipRayToPlayer(vecEyePos, vecPoint, pLocalPawn, pTargetPawn, &clipTrace))
			{
				if (clipTrace.m_flFraction <= worldTrace.m_flFraction + 0.15f)
					return true;
			}
		}

		return false;
	}

	int BuildBonePriority(int nRequestedBone, int* pBones, int nCapacity)
	{
		if (!pBones || nCapacity <= 0)
			return 0;

		int nCount = 0;
		pBones[nCount++] = nRequestedBone;

		for (int i = 0; i < static_cast<int>(sizeof(kFallbackBones) / sizeof(kFallbackBones[0])); ++i)
		{
			const int nBone = kFallbackBones[i];
			bool bAlreadyAdded = false;

			for (int j = 0; j < nCount; ++j)
			{
				if (pBones[j] == nBone)
				{
					bAlreadyAdded = true;
					break;
				}
			}

			if (!bAlreadyAdded && nCount < nCapacity)
				pBones[nCount++] = nBone;
		}

		return nCount;
	}

	bool ResolveAimPoint(C_CSPlayerPawn* pLocalPawn, C_CSPlayerPawn* pTargetPawn,
	                     const Vector3& vecEyePos, int nRequestedBone, bool bVisibleOnly,
	                     Vector3& vecOutPoint, int& nOutBone)
	{
		int arrBonePriority[8]{};
		const int nBoneCount = BuildBonePriority(nRequestedBone, arrBonePriority, 8);

		for (int i = 0; i < nBoneCount; ++i)
		{
			const int nBone = arrBonePriority[i];

			Vector3 vecBonePos{};
			if (!BONES::GetBonePosition(pTargetPawn, nBone, vecBonePos) || vecBonePos.IsZero())
				continue;

			if (bVisibleOnly && !IsPointVisible(pLocalPawn, pTargetPawn, vecEyePos, vecBonePos))
				continue;

			vecOutPoint = vecBonePos;
			nOutBone = nBone;
			return true;
		}

		return false;
	}

	float BuildCandidateScore(int nFilter, float flFov, float flDistance, int nHealth)
	{
		switch (nFilter)
		{
		case 1:
			return static_cast<float>(nHealth) * 1000.0f + flFov;
		case 2:
			return flDistance + flFov * 8.0f;
		case 0:
		default:
			return flFov;
		}
	}

	bool FillAimTarget(const Vector3& vecEyePos, const QAngle& angView, float flMaxFov,
	                   int nRequestedBone, bool bVisibleOnly, int nFilter, bool bApplyLockBias,
	                   C_CSPlayerPawn* pLocalPawn, C_CSPlayerPawn* pTargetPawn,
	                   F::LEGITBOT::TARGETING::AimTarget& outTarget)
	{
		Vector3 vecAimPoint{};
		int nAimBone = -1;
		if (!ResolveAimPoint(pLocalPawn, pTargetPawn, vecEyePos, nRequestedBone, bVisibleOnly, vecAimPoint, nAimBone))
			return false;

		const QAngle angToTarget = F::LEGITBOT::MATH::CalcAngle(vecEyePos, vecAimPoint);
		const float flFov = F::LEGITBOT::MATH::GetFOVDistance(angView, angToTarget);
		if (flFov > flMaxFov)
			return false;

		const Vector3 vecDelta = vecAimPoint - vecEyePos;
		const float flDistance = std::sqrtf(vecDelta.x * vecDelta.x + vecDelta.y * vecDelta.y + vecDelta.z * vecDelta.z);
		const int nHealth = pTargetPawn->GetHealth();

		outTarget.pPawn = pTargetPawn;
		outTarget.vecBonePos = vecAimPoint;
		outTarget.flFOV = flFov;
		outTarget.flDistance = flDistance;
		outTarget.nHealth = nHealth;
		outTarget.nBone = nAimBone;
		outTarget.flScore = BuildCandidateScore(nFilter, flFov, flDistance, nHealth);

		if (bApplyLockBias)
			outTarget.flScore *= kLockScoreBias;

		return true;
	}

	bool IsTargetStillLocked(C_CSPlayerPawn* pLocalPawn, const Vector3& vecEyePos, const QAngle& angView,
	                         float flMaxFov, int nRequestedBone, bool bVisibleOnly, int nFilter,
	                         F::LEGITBOT::TARGETING::AimTarget& outTarget)
	{
		if (!I::GlobalVars || I::GlobalVars->flCurrentTime > g_lockedUntil || !g_lockedTarget.IsValid())
			return false;

		auto* pLockedPawn = I::GameEntitySystem->Get<C_CSPlayerPawn>(g_lockedTarget);
		if (!pLockedPawn || pLockedPawn == pLocalPawn || !pLockedPawn->IsAlive())
			return false;

		return FillAimTarget(vecEyePos, angView, flMaxFov * kLockFovScale, nRequestedBone,
			bVisibleOnly, nFilter, true, pLocalPawn, pLockedPawn, outTarget);
	}
}

bool F::LEGITBOT::TARGETING::GetBestTarget(const Vector3& vecEyePos, const QAngle& angView, float flMaxFOV,
	int nTargetBone, C_CSPlayerPawn* pLocalPawn, AimTarget& bestOut)
{
	bestOut = {};

	if (!I::GameEntitySystem || !pLocalPawn)
		return false;

	const bool bTeamCheck = C::Get<bool>(aimbot_team_check);
	const bool bVisibleOnly = C::Get<bool>(aimbot_visible_only);
	const int nFilter = C::Get<int>(aimbot_target_filter);

	bool bFound = false;
	AimTarget candidate{};

	if (IsTargetStillLocked(pLocalPawn, vecEyePos, angView, flMaxFOV, nTargetBone, bVisibleOnly, nFilter, candidate))
	{
		bestOut = candidate;
		bFound = true;
	}

	for (int i = 1; i <= 64; ++i)
	{
		auto* pController = I::GameEntitySystem->Get<CCSPlayerController>(i);
		if (!pController || !pController->IsPawnAlive())
			continue;

		auto* pPawn = I::GameEntitySystem->Get<C_CSPlayerPawn>(pController->GetPlayerPawnHandle());
		if (!IsEnemyAllowed(pLocalPawn, pPawn, bTeamCheck))
			continue;

		AimTarget evaluated{};
		if (!FillAimTarget(vecEyePos, angView, flMaxFOV, nTargetBone, bVisibleOnly, nFilter, false, pLocalPawn, pPawn, evaluated))
			continue;

		if (!bFound || evaluated.flScore < bestOut.flScore ||
			(evaluated.flScore == bestOut.flScore && evaluated.flFOV < bestOut.flFOV))
		{
			bestOut = evaluated;
			bFound = true;
		}
	}

	if (!bFound)
	{
		g_lockedTarget = {};
		g_lockedUntil = 0.0f;
		return false;
	}

	g_lockedTarget = bestOut.pPawn->GetRefEHandle();
	g_lockedUntil = I::GlobalVars ? (I::GlobalVars->flCurrentTime + kLockDuration) : 0.0f;
	return true;
}
