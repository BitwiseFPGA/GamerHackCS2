#include "legitbot_aim.h"

#include "legitbot_math.h"
#include "legitbot_targeting.h"

#include "../../core/config.h"
#include "../../core/interfaces.h"
#include "../../core/variables.h"
#include "../../sdk/entity.h"
#include "../../sdk/functionlist.h"
#include "../../sdk/interfaces/ccsgoinput.h"
#include "../../sdk/interfaces/cgameentitysystem.h"
#include "../../sdk/interfaces/iengineclient.h"
#include "../bypass/bypass.h"

namespace
{
	bool IsAimActivationActive()
	{
		if (C::Get<bool>(aimbot_always_on))
			return true;

		const int nKey = C::Get<int>(aimbot_key);
		return nKey == 0 || (GetAsyncKeyState(nKey) & 0x8000) != 0;
	}
}

void F::LEGITBOT::AIM::OnCreateMove(CCSGOInput* pInput, CUserCmd* pCmd)
{
	if (!C::Get<bool>(aimbot_enabled))
		return;

	if (!pInput || !I::Engine || !I::Engine->IsInGame() || !I::GameEntitySystem)
		return;

	if (!IsAimActivationActive())
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

	const QAngle angView = pInput->GetViewAngles();

	TARGETING::AimTarget target{};
	if (!TARGETING::GetBestTarget(vecEyePos, angView, C::Get<float>(aimbot_fov),
		C::Get<int>(aimbot_bone), pLocalPawn, target))
		return;

	QAngle angDesired = MATH::CalcAngle(vecEyePos, target.vecBonePos);
	if (C::Get<bool>(aimbot_rcs))
	{
		const QAngle angPunch = pLocalPawn->GetAimPunchAngle();
		angDesired.x -= angPunch.x * 2.0f;
		angDesired.y -= angPunch.y * 2.0f;
	}

	angDesired.Normalize().Clamp();

	const float flSmooth = C::Get<float>(aimbot_smooth);
	const QAngle angOutput = MATH::SmoothAngle(angView, angDesired, flSmooth);

	if (pCmd)
	{
		F::BYPASS::SetViewAngles(angOutput, pInput, pCmd);
		return;
	}

	QAngle angWrite = angOutput;
	pInput->SetViewAngle(angWrite);
}
