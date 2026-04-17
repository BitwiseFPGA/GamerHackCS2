#include "ragebot.h"

#include "../../core/config.h"
#include "../../core/interfaces.h"
#include "../../core/variables.h"
#include "../../sdk/const.h"
#include "../../sdk/entity.h"
#include "../../sdk/functionlist.h"
#include "../../sdk/interfaces/ccsgoinput.h"
#include "../../sdk/interfaces/cgameentitysystem.h"
#include "../../sdk/interfaces/iengineclient.h"
#include "../../utilities/bones.h"
#include "../../utilities/math.h"
#include "../../utilities/trace.h"
#include "../bypass/bypass.h"
#include "../legitbot/legitbot_autowall.h"

#include <algorithm>
#include <array>
#include <cmath>

namespace
{
	struct RageCandidate
	{
		C_CSPlayerPawn* pPawn = nullptr;
		Vector3 vecPoint{};
		float flDamage = 0.0f;
		float flFov = 360.0f;
		int nBone = 0;
	};

	bool IsRageActive()
	{
		if (C::Get<bool>(rage_always_on))
			return true;

		const int nKey = C::Get<int>(rage_key);
		return nKey == 0 || (GetAsyncKeyState(nKey) & 0x8000) != 0;
	}

	void SetSecondaryAttack(CUserCmd* pCmd)
	{
		if (!pCmd)
			return;

		pCmd->nButtons.nValue |= IN_SECOND_ATTACK;
		pCmd->nButtons.nValueChanged |= IN_SECOND_ATTACK;

		auto* pBaseCmd = pCmd->csgoUserCmd.pBaseCmd;
		if (pBaseCmd && pBaseCmd->pInButtonState)
		{
			pBaseCmd->pInButtonState->nValue |= IN_SECOND_ATTACK;
			pBaseCmd->pInButtonState->nValueChanged |= IN_SECOND_ATTACK;
			pBaseCmd->pInButtonState->SetBits(BUTTON_STATE_PB_BITS_BUTTONSTATE1 | BUTTON_STATE_PB_BITS_BUTTONSTATE2);
			pBaseCmd->SetBits(BASE_BITS_BUTTONPB);
		}
	}

	bool CanScopeWeapon(C_CSPlayerPawn* pLocalPawn)
	{
		if (!pLocalPawn)
			return false;

		auto* pWeaponServices = pLocalPawn->GetWeaponServices();
		if (!pWeaponServices)
			return false;

		const CBaseHandle hWeapon = pWeaponServices->GetActiveWeapon();
		if (!hWeapon.IsValid())
			return false;

		auto* pWeapon = I::GameEntitySystem->Get<C_CSWeaponBase>(hWeapon);
		if (!pWeapon)
			return false;

		auto* pAttributeManager = pWeapon->GetAttributeManager();
		if (!pAttributeManager)
			return false;

		auto* pItemView = pAttributeManager->GetItem();
		if (!pItemView)
			return false;

		auto* pWeaponData = pItemView->GetBasePlayerWeaponVData();
		return pWeaponData &&
			pWeaponData->GetWeaponType() == static_cast<int>(CSWeaponType_t::WEAPONTYPE_SNIPER_RIFLE);
	}

	void ApplyAutoStop(CUserCmd* pCmd, C_CSPlayerPawn* pLocalPawn, const QAngle& angView)
	{
		if (!pCmd || !pLocalPawn)
			return;

		auto* pBaseCmd = pCmd->csgoUserCmd.pBaseCmd;
		if (!pBaseCmd)
			return;

		if ((pLocalPawn->GetFlags() & FL_ONGROUND) == 0)
			return;

		const Vector3 vecVelocity = pLocalPawn->GetAbsVelocity();
		const float flSpeed2D = std::sqrtf(vecVelocity.x * vecVelocity.x + vecVelocity.y * vecVelocity.y);
		if (flSpeed2D < 5.0f)
		{
			pBaseCmd->flForwardMove = 0.0f;
			pBaseCmd->flSideMove = 0.0f;
			return;
		}

		const float flVelocityYaw = MATH::RAD2DEG * std::atan2f(vecVelocity.y, vecVelocity.x);
		const float flDelta = MATH::DEG2RAD * (angView.y - flVelocityYaw);

		pBaseCmd->flForwardMove = std::clamp(-std::cosf(flDelta), -1.0f, 1.0f);
		pBaseCmd->flSideMove = std::clamp(std::sinf(flDelta), -1.0f, 1.0f);
		pCmd->nButtons.nValue &= ~IN_SPRINT;
	}

	bool HasDirectPath(C_CSPlayerPawn* pLocalPawn, C_CSPlayerPawn* pTargetPawn,
	                   const Vector3& vecStart, const Vector3& vecPoint)
	{
		GameTrace_t worldTrace{};
		if (!TRACE::TraceShape(vecStart, vecPoint, pLocalPawn, &worldTrace))
			return false;

		if (reinterpret_cast<C_BaseEntity*>(worldTrace.m_pHitEntity) ==
			reinterpret_cast<C_BaseEntity*>(pTargetPawn))
			return true;

		GameTrace_t clipTrace{};
		if (!TRACE::ClipRayToPlayer(vecStart, vecPoint, pLocalPawn, pTargetPawn, &clipTrace))
			return false;

		if (worldTrace.IsVisible())
			return true;

		return worldTrace.m_flFraction >= clipTrace.m_flFraction - 0.001f;
	}

	float EvaluatePointDamage(C_CSPlayerPawn* pLocalPawn, C_CSPlayerPawn* pTargetPawn,
	                          const Vector3& vecEyePos, const Vector3& vecPoint)
	{
		if (HasDirectPath(pLocalPawn, pTargetPawn, vecEyePos, vecPoint))
			return 100.0f;

		return F::LEGITBOT::AUTOWALL::GetDamageToTarget(pLocalPawn, pTargetPawn, vecPoint);
	}

	int BuildHitboxList(std::array<int, 8>& hitboxes)
	{
		int nCount = 0;

		if (C::Get<bool>(rage_hitbox_head))
			hitboxes[nCount++] = 6;
		if (C::Get<bool>(rage_hitbox_chest))
		{
			hitboxes[nCount++] = 4;
			hitboxes[nCount++] = 3;
		}
		if (C::Get<bool>(rage_hitbox_stomach))
			hitboxes[nCount++] = 2;
		if (C::Get<bool>(rage_hitbox_pelvis))
			hitboxes[nCount++] = 0;

		if (nCount == 0)
			hitboxes[nCount++] = 6;

		return nCount;
	}

	int BuildPointCloud(int nBone, const Vector3& vecCenter, float flScale,
	                    std::array<Vector3, 6>& points)
	{
		const float flRadius = (nBone == 6 ? 3.25f : 4.5f) * flScale;
		int nCount = 0;

		points[nCount++] = vecCenter;

		if (flRadius <= 0.0f)
			return nCount;

		points[nCount++] = { vecCenter.x + flRadius, vecCenter.y, vecCenter.z };
		points[nCount++] = { vecCenter.x - flRadius, vecCenter.y, vecCenter.z };
		points[nCount++] = { vecCenter.x, vecCenter.y + flRadius, vecCenter.z };
		points[nCount++] = { vecCenter.x, vecCenter.y - flRadius, vecCenter.z };

		if (nBone == 6)
			points[nCount++] = { vecCenter.x, vecCenter.y, vecCenter.z + flRadius };

		return nCount;
	}

	bool EvaluateTarget(C_CSPlayerPawn* pLocalPawn, C_CSPlayerPawn* pTargetPawn,
	                    const Vector3& vecEyePos, const QAngle& angView,
	                    const std::array<int, 8>& hitboxes, int nHitboxCount,
	                    float flScale, float flMinDamage, RageCandidate& bestOut)
	{
		if (!pTargetPawn || !pTargetPawn->IsAlive())
			return false;

		auto* pSceneNode = pTargetPawn->GetGameSceneNode();
		if (!pSceneNode)
			return false;

		if (auto* pSkeleton = pSceneNode->GetSkeletonInstance())
			BONES::CalcWorldSpaceBones(pSkeleton, 0xFFFFF);

		bool bFound = false;

		for (int i = 0; i < nHitboxCount; ++i)
		{
			const int nBone = hitboxes[i];
			Vector3 vecBonePos{};
			if (!pSceneNode->GetBonePosition(nBone, vecBonePos) || vecBonePos.IsZero())
				continue;

			std::array<Vector3, 6> points{};
			const int nPointCount = BuildPointCloud(nBone, vecBonePos, flScale, points);

			for (int nPoint = 0; nPoint < nPointCount; ++nPoint)
			{
				const float flDamage = EvaluatePointDamage(pLocalPawn, pTargetPawn, vecEyePos, points[nPoint]);
				if (flDamage < flMinDamage)
					continue;

				const QAngle angPoint = MATH::CalcAngle(vecEyePos, points[nPoint]);
				const float flFov = MATH::GetFOV(angView, angPoint);
				const bool bBetterDamage = flDamage > bestOut.flDamage;
				const bool bTieBreaker = std::fabs(flDamage - bestOut.flDamage) <= 0.01f &&
					flFov < bestOut.flFov;

				if (!bestOut.pPawn || bBetterDamage || bTieBreaker)
				{
					bestOut.pPawn = pTargetPawn;
					bestOut.vecPoint = points[nPoint];
					bestOut.flDamage = flDamage;
					bestOut.flFov = flFov;
					bestOut.nBone = nBone;
					bFound = true;
				}
			}
		}

		return bFound;
	}
}

bool F::RAGEBOT::Setup()
{
	return true;
}

void F::RAGEBOT::Destroy()
{
}

void F::RAGEBOT::OnCreateMove(CCSGOInput* pInput, CUserCmd* pCmd)
{
	if (!C::Get<bool>(rage_enabled) || !pInput || !pCmd)
		return;

	if (!I::Engine || !I::Engine->IsInGame() || !I::GameEntitySystem)
		return;

	if (!IsRageActive())
		return;

	auto* pLocalController = SDK_FUNC::GetLocalPlayerController ?
		SDK_FUNC::GetLocalPlayerController(-1) : nullptr;
	if (!pLocalController || !pLocalController->IsPawnAlive())
		return;

	auto* pLocalPawn = I::GameEntitySystem->Get<C_CSPlayerPawn>(pLocalController->GetPlayerPawnHandle());
	if (!pLocalPawn || !pLocalPawn->IsAlive())
		return;

	const Vector3 vecEyePos = pLocalPawn->GetEyePosition();
	if (vecEyePos.IsZero())
		return;

	std::array<int, 8> hitboxes{};
	const int nHitboxCount = BuildHitboxList(hitboxes);
	const float flScale = C::Get<bool>(rage_multipoint) ?
		(C::Get<float>(rage_multipoint_scale) / 100.0f) : 0.0f;
	const float flMinDamage = C::Get<float>(rage_min_damage);
	const bool bTeamCheck = C::Get<bool>(rage_team_check);
	const int nLocalTeam = static_cast<int>(pLocalPawn->GetTeam());
	const QAngle angView = pInput->GetViewAngles();

	RageCandidate best{};

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

		EvaluateTarget(pLocalPawn, pPawn, vecEyePos, angView, hitboxes, nHitboxCount,
			flScale, flMinDamage, best);
	}

	if (!best.pPawn)
		return;

	if (C::Get<bool>(rage_auto_stop))
		ApplyAutoStop(pCmd, pLocalPawn, angView);

	QAngle angAim = MATH::CalcAngle(vecEyePos, best.vecPoint);
	const QAngle angPunch = pLocalPawn->GetAimPunchAngle();
	angAim.x -= angPunch.x * 2.0f;
	angAim.y -= angPunch.y * 2.0f;
	angAim.Normalize().Clamp();

	if (C::Get<bool>(rage_silent))
	{
		pCmd->SetSubTickAngle(angAim);

		if (auto* pBaseCmd = pCmd->csgoUserCmd.pBaseCmd; pBaseCmd && pBaseCmd->pViewAngles)
		{
			pBaseCmd->pViewAngles->angValue = angAim;
			pBaseCmd->SetBits(BASE_BITS_VIEWANGLES);
		}
	}
	else
	{
		F::BYPASS::SetViewAngles(angAim, pInput, pCmd);
	}

	if (C::Get<bool>(rage_auto_scope) && !pLocalPawn->IsScoped() && CanScopeWeapon(pLocalPawn))
	{
		SetSecondaryAttack(pCmd);
		return;
	}

	if (C::Get<bool>(rage_auto_shoot))
		F::BYPASS::SetAttack(pCmd, true);
}
