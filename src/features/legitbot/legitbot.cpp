#include "legitbot.h"

#include "../../core/config.h"
#include "../../core/variables.h"
#include "../../core/interfaces.h"
#include "../../utilities/log.h"
#include "../../utilities/input.h"
#include "../../utilities/render.h"
#include "../../utilities/trace.h"
#include "../../utilities/bones.h"
#include "../../utilities/xorstr.h"
#include "../../sdk/entity.h"
#include "../../sdk/interfaces/cgameentitysystem.h"
#include "../../sdk/interfaces/ccsgoinput.h"
#include "../../sdk/interfaces/iengineclient.h"
#include "../../sdk/datatypes/usercmd.h"
#include "../bypass/bypass.h"

#include <cmath>
#include <numbers>
#include <algorithm>

// ---------------------------------------------------------------
// local math helpers (avoids math.h type conflicts with SDK types)
// ---------------------------------------------------------------
static QAngle CalcAngle(const Vector3& vecSrc, const Vector3& vecDst)
{
	Vector3 delta = vecDst - vecSrc;
	float hyp = std::sqrtf(delta.x * delta.x + delta.y * delta.y);

	QAngle angles;
	angles.x = -std::atan2f(delta.z, hyp) * (180.0f / std::numbers::pi_v<float>);
	angles.y =  std::atan2f(delta.y, delta.x) * (180.0f / std::numbers::pi_v<float>);
	angles.z = 0.0f;
	return angles;
}

static float GetFOVDistance(const QAngle& angView, const QAngle& angAim)
{
	QAngle delta;
	delta.x = std::remainderf(angAim.x - angView.x, 360.0f);
	delta.y = std::remainderf(angAim.y - angView.y, 360.0f);
	delta.z = 0.0f;
	return std::sqrtf(delta.x * delta.x + delta.y * delta.y);
}

static QAngle SmoothAngle(const QAngle& angSrc, const QAngle& angDst, float flFactor)
{
	if (flFactor <= 0.0f)
		return angDst;

	QAngle delta;
	delta.x = std::remainderf(angDst.x - angSrc.x, 360.0f);
	delta.y = std::remainderf(angDst.y - angSrc.y, 360.0f);
	delta.z = 0.0f;

	QAngle result;
	result.x = angSrc.x + delta.x / (flFactor + 1.0f);
	result.y = angSrc.y + delta.y / (flFactor + 1.0f);
	result.z = 0.0f;
	return result.Clamp();
}

// ---------------------------------------------------------------
// bone position helper — delegates to BONES utility
// ---------------------------------------------------------------
static bool GetBonePosition(C_CSPlayerPawn* pPawn, int nBone, Vector3& vecOut)
{
	return BONES::GetBonePosition(pPawn, nBone, vecOut);
}

// ---------------------------------------------------------------
// target selection
// ---------------------------------------------------------------
struct AimTarget
{
	C_CSPlayerPawn* pPawn = nullptr;
	Vector3 vecBonePos    = {};
	float   flFOV         = 999.0f;
	float   flDistance     = 999999.0f;
	int     nHealth       = 999;
};

static bool GetBestTarget(const Vector3& vecEyePos, const QAngle& angView, float flMaxFOV,
	int nTargetBone, C_CSPlayerPawn* pLocalPawn, AimTarget& bestOut)
{
	if (!I::GameEntitySystem)
		return false;

	const int nLocalTeam = static_cast<int>(pLocalPawn->GetTeam());
	const int nFilter = C::Get<int>(aimbot_target_filter);

	float flBestFOV = flMaxFOV;
	float flBestDist = 999999.0f;
	int   nBestHP = 99999;
	bool bFound = false;

	for (int i = 1; i <= 64; i++)
	{
		__try
		{
			auto* pController = I::GameEntitySystem->Get<CCSPlayerController>(i);
			if (!pController || !pController->IsPawnAlive())
				continue;

			auto* pPawn = I::GameEntitySystem->Get<C_CSPlayerPawn>(pController->GetPlayerPawnHandle());
			if (!pPawn || pPawn == pLocalPawn)
				continue;

			if (static_cast<int>(pPawn->GetTeam()) == nLocalTeam)
				continue;

			if (!pPawn->IsAlive())
				continue;

			auto* pSceneNode = pPawn->GetGameSceneNode();
			if (!pSceneNode)
				continue;

			if (C::Get<bool>(aimbot_visible_only))
			{
				if (!TRACE::IsBoneVisible(pLocalPawn, pPawn, nTargetBone))
					continue;
			}

			Vector3 vecBone;
			if (!GetBonePosition(pPawn, nTargetBone, vecBone))
				continue;

			QAngle angToTarget = CalcAngle(vecEyePos, vecBone);
			float flFOV = GetFOVDistance(angView, angToTarget);

			// always check FOV cone first
			if (flFOV > flMaxFOV)
				continue;

			bool bBetter = false;
			switch (nFilter)
			{
			default:
			case 0: // closest angle
				if (flFOV < flBestFOV)
				{
					flBestFOV = flFOV;
					bBetter = true;
				}
				break;

			case 1: // lowest health
			{
				int hp = pPawn->GetHealth();
				if (hp < nBestHP || (hp == nBestHP && flFOV < flBestFOV))
				{
					nBestHP = hp;
					flBestFOV = flFOV;
					bBetter = true;
				}
				break;
			}

			case 2: // closest distance
			{
				Vector3 delta = vecBone - vecEyePos;
				float dist = std::sqrtf(delta.x * delta.x + delta.y * delta.y + delta.z * delta.z);
				if (dist < flBestDist || (dist == flBestDist && flFOV < flBestFOV))
				{
					flBestDist = dist;
					flBestFOV = flFOV;
					bBetter = true;
				}
				break;
			}
			}

			if (bBetter)
			{
				bestOut.pPawn     = pPawn;
				bestOut.vecBonePos = vecBone;
				bestOut.flFOV     = flFOV;
				bestOut.nHealth   = pPawn->GetHealth();
				bFound = true;
			}
		}
		__except (EXCEPTION_EXECUTE_HANDLER)
		{
			continue;
		}
	}

	return bFound;
}

// ---------------------------------------------------------------
// setup / destroy
// ---------------------------------------------------------------
bool F::LEGITBOT::Setup()
{
	L_PRINT(LOG_INFO) << _XS("[LEGITBOT] initialized");
	return true;
}

void F::LEGITBOT::Destroy()
{
	L_PRINT(LOG_INFO) << _XS("[LEGITBOT] destroyed");
}

// ---------------------------------------------------------------
// OnCreateMove — main aimbot logic
// ---------------------------------------------------------------
void F::LEGITBOT::OnCreateMove(CCSGOInput* pInput, CUserCmd* pCmd)
{
	if (!C::Get<bool>(aimbot_enabled))
		return;

	if (!I::Engine || !I::Engine->IsInGame())
		return;

	if (!I::GameEntitySystem)
		return;

	static bool bFirstCall = true;
	if (bFirstCall)
	{
		bFirstCall = false;
		L_PRINT(LOG_INFO) << _XS("[LEGITBOT] OnCreateMove first call — pInput=") << static_cast<void*>(pInput)
			<< _XS(" pCmd=") << static_cast<void*>(pCmd);
	}

	// check aim key — skip if always-on toggle is set
	if (!C::Get<bool>(aimbot_always_on))
	{
		const int nAimKey = C::Get<int>(aimbot_key);
		if (nAimKey > 0 && !(GetAsyncKeyState(nAimKey) & 0x8000))
			return;
	}

	__try
	{
		// get local player controller via SDK function (Andromeda passes -1 for local)
		auto* pLocalController = SDK_FUNC::GetLocalPlayerController ?
			SDK_FUNC::GetLocalPlayerController(-1) : nullptr;
		if (!pLocalController || !pLocalController->IsPawnAlive())
			return;

		// Use m_hPlayerPawn (CCSPlayerController-specific) — more reliable than m_hPawn
		CBaseHandle hPawn = pLocalController->GetPlayerPawnHandle();
		if (!hPawn.IsValid())
			return;

		auto* pLocalPawn = I::GameEntitySystem->Get<C_CSPlayerPawn>(hPawn);
		if (!pLocalPawn || !pLocalPawn->IsAlive())
			return;

		// eye position = scene origin + view offset
		Vector3 vecEyePos = pLocalPawn->GetSceneOrigin() + pLocalPawn->GetViewOffset();

		// current view angles
		QAngle angView = pInput->GetViewAngles();

		// find best target
		const float flMaxFOV    = C::Get<float>(aimbot_fov);
		const int   nTargetBone = C::Get<int>(aimbot_bone);
		const float flSmooth    = C::Get<float>(aimbot_smooth);

		AimTarget target;
		if (!GetBestTarget(vecEyePos, angView, flMaxFOV, nTargetBone, pLocalPawn, target))
			return;

		// calculate aim angle
		QAngle angAim = CalcAngle(vecEyePos, target.vecBonePos);

		// apply recoil compensation
		if (C::Get<bool>(aimbot_rcs))
		{
			QAngle punch = pLocalPawn->GetAimPunchAngle();
			angAim.x -= punch.x * 2.0f;
			angAim.y -= punch.y * 2.0f;
		}

		angAim.Normalize();

		// apply smoothing
		QAngle angFinal = SmoothAngle(angView, angAim, flSmooth);

		// set view angles via bypass system (anti-cheat safe path)
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
