#include "misc_camera.h"

#include "../../core/config.h"
#include "../../core/interfaces.h"
#include "../../core/variables.h"
#include "../../sdk/entity.h"
#include "../../sdk/functionlist.h"
#include "../../sdk/interfaces/ccsgoinput.h"
#include "../../sdk/interfaces/cgameentitysystem.h"
#include "../../sdk/interfaces/iengineclient.h"
#include "../../utilities/log.h"
#include "../../utilities/trace.h"
#include "../../utilities/xorstr.h"

#include <cmath>

// CViewSetup offsets from darkside (reference code 5)
struct CViewSetup
{
	std::uint8_t _pad0[0x4D8];
	float m_flFOV;          // 0x4D8
	float m_flViewmodelFOV; // 0x4DC
	Vector3 m_vecOrigin;    // 0x4E0
	std::uint8_t _pad1[0xC];
	Vector3 m_vecAngles;    // 0x4F8
};

static Vector3 CalculateCameraPos(const Vector3& vecAnchor, float flDistance, const Vector3& vecAngles)
{
	constexpr float flDeg2Rad = 3.14159265f / 180.0f;
	float flYaw   = vecAngles.y * flDeg2Rad;
	float flPitch = vecAngles.x * flDeg2Rad;

	return {
		vecAnchor.x + flDistance * std::cosf(flYaw) * std::cosf(flPitch),
		vecAnchor.y + flDistance * std::sinf(flYaw) * std::cosf(flPitch),
		vecAnchor.z + flDistance * std::sinf(flPitch)
	};
}

void F::MISC::CAMERA::OnOverrideView(void* pViewSetup)
{
	if (!pViewSetup)
		return;

	auto* pSetup = reinterpret_cast<CViewSetup*>(pViewSetup);

	// FOV changer
	float flDesiredFOV = C::Get<float>(misc_fov_changer);
	if (flDesiredFOV != 90.0f)
		pSetup->m_flFOV = flDesiredFOV;

	// third person camera
	if (!C::Get<bool>(misc_thirdperson))
		return;

	if (!I::Engine || !I::Engine->IsInGame() || !I::GameEntitySystem)
		return;

	__try
	{
		auto* pLocalController = SDK_FUNC::GetLocalPlayerController
			? SDK_FUNC::GetLocalPlayerController(-1) : nullptr;
		if (!pLocalController || !pLocalController->IsPawnAlive())
			return;

		auto* pLocalPawn = I::GameEntitySystem->Get<C_CSPlayerPawn>(
			pLocalController->GetPlayerPawnHandle());
		if (!pLocalPawn || !pLocalPawn->IsAlive())
			return;

		Vector3 vecEyePos = pLocalPawn->GetEyePosition();
		if (vecEyePos.IsZero())
			return;

		// get view angles and negate pitch for camera calculation (darkside approach)
		Vector3 vecCamAngles = { -pSetup->m_vecAngles.x, pSetup->m_vecAngles.y, 0.0f };

		float flDist = C::Get<float>(misc_thirdperson_distance);
		Vector3 vecCamPos = CalculateCameraPos(vecEyePos, -flDist, vecCamAngles);

		// trace from eye to camera to prevent clipping through walls
		GameTrace_t trace{};
		if (TRACE::TraceShape(vecEyePos, vecCamPos, pLocalPawn, &trace))
		{
			if (trace.m_pHitEntity != nullptr)
			{
				// hit a wall — pull camera forward
				Vector3 vecDir = vecEyePos - vecCamPos;
				float flLen = std::sqrtf(vecDir.x * vecDir.x + vecDir.y * vecDir.y + vecDir.z * vecDir.z);
				if (flLen > 0.0f)
				{
					vecDir.x /= flLen; vecDir.y /= flLen; vecDir.z /= flLen;
					vecCamPos = trace.m_vecPosition + vecDir * 10.0f;
				}
			}
		}

		pSetup->m_vecOrigin = vecCamPos;

		// recalculate camera angles to look at the player
		Vector3 vecDelta = vecEyePos - vecCamPos;
		float flLen = std::sqrtf(vecDelta.x * vecDelta.x + vecDelta.y * vecDelta.y + vecDelta.z * vecDelta.z);
		if (flLen > 0.0f)
		{
			constexpr float flRad2Deg = 180.0f / 3.14159265f;
			float flPitch = -std::asinf(vecDelta.z / flLen) * flRad2Deg;
			float flYaw = std::atan2f(vecDelta.y, vecDelta.x) * flRad2Deg;
			pSetup->m_vecAngles = { flPitch, flYaw, 0.0f };
		}

		static bool bLoggedFirst = false;
		if (!bLoggedFirst)
		{
			bLoggedFirst = true;
			L_PRINT(LOG_INFO) << _XS("[CAMERA] thirdperson active — dist=") << flDist;
		}
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		static bool bLogged = false;
		if (!bLogged) { bLogged = true;
			L_PRINT(LOG_ERROR) << _XS("[CAMERA] exception in thirdperson");
		}
	}
}
