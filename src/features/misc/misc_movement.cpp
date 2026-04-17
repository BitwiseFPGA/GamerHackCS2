#include "misc_movement.h"

#include "../../core/config.h"
#include "../../core/interfaces.h"
#include "../../core/variables.h"
#include "../../sdk/const.h"
#include "../../sdk/entity.h"
#include "../../sdk/functionlist.h"
#include "../../sdk/interfaces/ccsgoinput.h"
#include "../../sdk/interfaces/cgameentitysystem.h"
#include "../../sdk/interfaces/iengineclient.h"
#include "../../utilities/log.h"
#include "../../utilities/math.h"
#include "../../utilities/xorstr.h"

#include <cmath>
#include <algorithm>

// ---------------------------------------------------------------
// movement type constants — use enum values from const.h
// ---------------------------------------------------------------

// ---------------------------------------------------------------
// BunnyHop — release jump when airborne so engine auto-hops
// ---------------------------------------------------------------
static void BunnyHop(CUserCmd* pCmd, C_CSPlayerPawn* pLocalPawn)
{
	const int nFlags = pLocalPawn->GetFlags();
	if ((pCmd->nButtons.nValue & IN_JUMP) && !(nFlags & FL_ONGROUND))
	{
		// Clear jump from outer button state
		pCmd->nButtons.nValue &= ~IN_JUMP;

		// Clear jump from protobuf (what server receives) and mark dirty
		auto* pBaseCmd = pCmd->csgoUserCmd.pBaseCmd;
		if (pBaseCmd && pBaseCmd->pInButtonState)
		{
			pBaseCmd->pInButtonState->nValue &= ~IN_JUMP;
			pBaseCmd->pInButtonState->nValueChanged |= IN_JUMP;
			pBaseCmd->pInButtonState->SetBits(BUTTON_STATE_PB_BITS_BUTTONSTATE1 | BUTTON_STATE_PB_BITS_BUTTONSTATE2);
			pBaseCmd->SetBits(BASE_BITS_BUTTONPB);
		}
	}
}

// ---------------------------------------------------------------
// Advanced AutoStrafe — velocity-based with ideal rotation
// ---------------------------------------------------------------
static void AutoStrafe(CUserCmd* pCmd, C_CSPlayerPawn* pLocalPawn, float flStrafeSmooth)
{
	const int nFlags = pLocalPawn->GetFlags();
	if (nFlags & FL_ONGROUND)
		return;

	auto* pBaseCmd = pCmd->csgoUserCmd.pBaseCmd;
	if (!pBaseCmd)
		return;

	Vector3 vecVelocity = pLocalPawn->GetAbsVelocity();
	float flSpeed2D = std::sqrtf(vecVelocity.x * vecVelocity.x + vecVelocity.y * vecVelocity.y);

	// too slow — just hold forward
	if (flSpeed2D < 2.0f && pBaseCmd->flForwardMove == 0.0f && pBaseCmd->flSideMove == 0.0f)
		return;

	// calculate ideal rotation angle for air strafing
	float flIdealRotation = (flSpeed2D > 0.0f) ?
		std::min(MATH::RAD2DEG * std::asinf(std::min(15.0f / flSpeed2D, 1.0f)), 90.0f) : 90.0f;

	// alternate direction each tick
	float flSign = (pBaseCmd->nLegacyCommandNumber % 2) ? 1.0f : -1.0f;

	// determine intended movement direction from buttons
	bool bForward  = (pCmd->nButtons.nValue & IN_FORWARD) != 0;
	bool bBack     = (pCmd->nButtons.nValue & IN_BACK) != 0;
	bool bLeft     = (pCmd->nButtons.nValue & IN_MOVELEFT) != 0;
	bool bRight    = (pCmd->nButtons.nValue & IN_MOVERIGHT) != 0;

	// if moving slowly, set forward
	pBaseCmd->flForwardMove = (flSpeed2D > 0.1f) ? 0.0f : 1.0f;

	// get current view yaw
	QAngle angView = I::Input->GetViewAngles();
	float flMovementYaw = angView.y;

	// adjust movement angle based on held keys
	if (bForward)
		flMovementYaw += bLeft ? 45.0f : bRight ? -45.0f : 0.0f;
	else if (bBack)
		flMovementYaw += bLeft ? 135.0f : bRight ? -135.0f : 180.0f;
	else if (bLeft || bRight)
		flMovementYaw += bLeft ? 90.0f : -90.0f;

	float flYawDelta = std::remainderf(flMovementYaw - angView.y, 360.0f);
	float flAbsYawDelta = std::fabsf(flYawDelta);

	// set initial strafe direction based on yaw delta
	if (flYawDelta > 0.0f)
		pBaseCmd->flSideMove = -1.0f;
	else if (flYawDelta < 0.0f)
		pBaseCmd->flSideMove = 1.0f;

	// velocity-based retracking
	if (flAbsYawDelta <= flIdealRotation || flAbsYawDelta >= 30.0f)
	{
		float flVelAng = MATH::RAD2DEG * std::atan2f(vecVelocity.y, vecVelocity.x);
		float flVelDelta = std::remainderf(flMovementYaw - flVelAng, 360.0f);

		float flSmooth = std::max(flStrafeSmooth, 1.0f);
		float flRetrackSpeed = flIdealRotation * ((flSmooth / 100.0f) * 3.0f);

		if (flVelDelta <= flRetrackSpeed || flSpeed2D <= 15.0f)
		{
			if (-flRetrackSpeed <= flVelDelta || flSpeed2D <= 15.0f)
			{
				flMovementYaw += flIdealRotation * flSign;
				pBaseCmd->flSideMove = flSign;
			}
			else
			{
				flMovementYaw = flVelAng - flRetrackSpeed;
				pBaseCmd->flSideMove = 1.0f;
			}
		}
		else
		{
			flMovementYaw = flVelAng + flRetrackSpeed;
			pBaseCmd->flSideMove = -1.0f;
		}
	}

	// rotate forward/side move to match the corrected movement angle
	float flRotation = MATH::DEG2RAD * (angView.y - flMovementYaw);

	float flNewForward = std::cosf(flRotation) * pBaseCmd->flForwardMove - std::sinf(flRotation) * pBaseCmd->flSideMove;
	float flNewSide    = std::sinf(flRotation) * pBaseCmd->flForwardMove + std::cosf(flRotation) * pBaseCmd->flSideMove;

	pBaseCmd->flSideMove    = std::clamp(flNewSide * -1.0f, -1.0f, 1.0f);
	pBaseCmd->flForwardMove = std::clamp(flNewForward, -1.0f, 1.0f);

	// Mark movement fields dirty so protobuf serializer includes them
	pBaseCmd->SetBits(BASE_BITS_FORWARDMOVE | BASE_BITS_LEFTMOVE);
}

// ---------------------------------------------------------------
// MovementFix — correct movement input when view angles change
// ---------------------------------------------------------------
void F::MISC::MOVEMENT::MovementFix(CUserCmd* pCmd, const QAngle& angOldView)
{
	if (!pCmd)
		return;

	auto* pBaseCmd = pCmd->csgoUserCmd.pBaseCmd;
	if (!pBaseCmd || !pBaseCmd->pViewAngles)
		return;

	QAngle angNewView;
	angNewView.x = pBaseCmd->pViewAngles->angValue.x;
	angNewView.y = pBaseCmd->pViewAngles->angValue.y;
	angNewView.z = pBaseCmd->pViewAngles->angValue.z;

	// compute rotation from old to new view angle
	Vector3 vecForward{}, vecRight{}, vecUp{};
	Vector3 vecOldForward{}, vecOldRight{}, vecOldUp{};

	MATH::AngleVectors(angNewView, &vecForward, &vecRight, &vecUp);
	MATH::AngleVectors(angOldView, &vecOldForward, &vecOldRight, &vecOldUp);

	// zero out z for horizontal-only correction
	vecForward.z = vecRight.z = vecUp.x = vecUp.y = 0.0f;
	vecOldForward.z = vecOldRight.z = vecOldUp.x = vecOldUp.y = 0.0f;

	// normalize
	auto normalize = [](Vector3& v)
	{
		float len = std::sqrtf(v.x * v.x + v.y * v.y + v.z * v.z);
		if (len > 0.0f) { v.x /= len; v.y /= len; v.z /= len; }
	};

	normalize(vecForward); normalize(vecRight); normalize(vecUp);
	normalize(vecOldForward); normalize(vecOldRight); normalize(vecOldUp);

	// scale by current move values
	float flFwd = pBaseCmd->flForwardMove;
	float flSide = pBaseCmd->flSideMove;
	float flUp = pBaseCmd->flUpMove;

	Vector3 vecScaledFwd = vecForward * flFwd;
	Vector3 vecScaledRight = vecRight * flSide;
	Vector3 vecScaledUp = vecUp * flUp;

	// project onto old basis vectors
	auto dot = [](const Vector3& a, const Vector3& b) { return a.x * b.x + a.y * b.y + a.z * b.z; };

	float flFixedForward = dot(vecOldForward, vecScaledRight) + dot(vecOldForward, vecScaledFwd) + dot(vecOldForward, vecScaledUp);
	float flFixedSide    = dot(vecOldRight, vecScaledRight) + dot(vecOldRight, vecScaledFwd) + dot(vecOldRight, vecScaledUp);

	pBaseCmd->flForwardMove = std::clamp(flFixedForward, -1.0f, 1.0f);
	pBaseCmd->flSideMove    = std::clamp(flFixedSide, -1.0f, 1.0f);

	// Mark movement fields dirty so protobuf serializer includes them
	pBaseCmd->SetBits(BASE_BITS_FORWARDMOVE | BASE_BITS_LEFTMOVE);
}

// ---------------------------------------------------------------
// main dispatch
// ---------------------------------------------------------------
void F::MISC::MOVEMENT::OnCreateMove(CCSGOInput* pInput, CUserCmd* pCmd)
{
	if (!I::Engine || !I::Engine->IsInGame())
		return;

	if (!I::GameEntitySystem || !pInput || !pCmd)
		return;

	const bool bBhop       = C::Get<bool>(misc_bhop);
	const bool bAutoStrafe = C::Get<bool>(misc_autostrafe);

	if (!bBhop && !bAutoStrafe)
		return;

	__try
	{
		auto* pLocalController = SDK_FUNC::GetLocalPlayerController ?
			SDK_FUNC::GetLocalPlayerController(-1) : nullptr;
		if (!pLocalController)
			return;

		if (!pLocalController->IsPawnAlive())
			return;

		auto* pLocalPawn = I::GameEntitySystem->Get<C_CSPlayerPawn>(pLocalController->GetPlayerPawnHandle());
		if (!pLocalPawn || !pLocalPawn->IsAlive())
			return;

		// skip movement features on ladders and noclip
		std::uint8_t nMoveType = pLocalPawn->GetMoveType();
		if (nMoveType == MOVETYPE_LADDER || nMoveType == MOVETYPE_NOCLIP)
			return;

		if (bBhop)
			BunnyHop(pCmd, pLocalPawn);

		if (bAutoStrafe)
		{
			float flSmooth = C::Get<float>(misc_strafe_smooth);
			AutoStrafe(pCmd, pLocalPawn, flSmooth);
		}
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		static bool bLoggedEx = false;
		if (!bLoggedEx)
		{
			bLoggedEx = true;
			const DWORD dwCode = GetExceptionCode();
			L_PRINT(LOG_ERROR) << _XS("[MOVEMENT] EXCEPTION in OnCreateMove, code=0x")
				<< reinterpret_cast<void*>(static_cast<uintptr_t>(dwCode));
		}
	}
}
