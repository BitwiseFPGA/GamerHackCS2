#include "legitbot_aim.h"

#include "legitbot_math.h"
#include "legitbot_targeting.h"

#include "../../core/config.h"
#include "../../core/interfaces.h"
#include "../../core/variables.h"
#include "../../sdk/entity.h"
#include "../../sdk/interfaces/ccsgoinput.h"
#include "../../sdk/interfaces/cgameentitysystem.h"
#include "../../sdk/interfaces/iengineclient.h"
#include "../../utilities/log.h"
#include "../../utilities/xorstr.h"
#include "../bypass/bypass.h"

void F::LEGITBOT::AIM::OnCreateMove(CCSGOInput* pInput, CUserCmd* pCmd)
{
	if (!C::Get<bool>(aimbot_enabled))
		return;

	if (!I::Engine || !I::Engine->IsInGame())
		return;

	if (!I::GameEntitySystem || !pInput)
		return;

	static bool bFirstCall = true;
	if (bFirstCall)
	{
		bFirstCall = false;
		L_PRINT(LOG_INFO) << _XS("[LEGITBOT] OnCreateMove first call — pInput=") << static_cast<void*>(pInput)
			<< _XS(" pCmd=") << static_cast<void*>(pCmd);
	}

	if (!C::Get<bool>(aimbot_always_on))
	{
		const int nAimKey = C::Get<int>(aimbot_key);
		if (nAimKey > 0 && !(GetAsyncKeyState(nAimKey) & 0x8000))
			return;
	}

	__try
	{
		auto* pLocalController = SDK_FUNC::GetLocalPlayerController ?
			SDK_FUNC::GetLocalPlayerController(-1) : nullptr;
		if (!pLocalController || !pLocalController->IsPawnAlive())
			return;

		CBaseHandle hPawn = pLocalController->GetPlayerPawnHandle();
		if (!hPawn.IsValid())
			return;

		auto* pLocalPawn = I::GameEntitySystem->Get<C_CSPlayerPawn>(hPawn);
		if (!pLocalPawn || !pLocalPawn->IsAlive())
			return;

		Vector3 vecEyePos = pLocalPawn->GetSceneOrigin() + pLocalPawn->GetViewOffset();
		QAngle angView = pInput->GetViewAngles();

		const float flMaxFOV = C::Get<float>(aimbot_fov);
		const int nTargetBone = C::Get<int>(aimbot_bone);
		const float flSmooth = C::Get<float>(aimbot_smooth);

		TARGETING::AimTarget target;
		if (!TARGETING::GetBestTarget(vecEyePos, angView, flMaxFOV, nTargetBone, pLocalPawn, target))
			return;

		QAngle angAim = MATH::CalcAngle(vecEyePos, target.vecBonePos);
		if (C::Get<bool>(aimbot_rcs))
		{
			QAngle punch = pLocalPawn->GetAimPunchAngle();
			angAim.x -= punch.x * 2.0f;
			angAim.y -= punch.y * 2.0f;
		}

		angAim.Normalize();

		QAngle angFinal = MATH::SmoothAngle(angView, angAim, flSmooth);
		if (pCmd)
			F::BYPASS::SetViewAngles(angFinal, pInput, pCmd);
		else
			pInput->SetViewAngle(angFinal);
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		static bool bLoggedEx = false;
		if (!bLoggedEx)
		{
			bLoggedEx = true;
			L_PRINT(LOG_ERROR) << _XS("[LEGITBOT] EXCEPTION in OnCreateMove — entity access likely failed");
		}
	}
}
