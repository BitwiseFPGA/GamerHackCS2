#include "legitbot_targeting.h"

#include "legitbot_math.h"

#include "../../core/config.h"
#include "../../core/interfaces.h"
#include "../../core/variables.h"
#include "../../sdk/entity.h"
#include "../../sdk/interfaces/cgameentitysystem.h"
#include "../../utilities/bones.h"
#include "../../utilities/trace.h"

#include <cmath>

bool F::LEGITBOT::TARGETING::GetBestTarget(const Vector3& vecEyePos, const QAngle& angView, float flMaxFOV,
	int nTargetBone, C_CSPlayerPawn* pLocalPawn, AimTarget& bestOut)
{
	if (!I::GameEntitySystem || !pLocalPawn)
		return false;

	const int nLocalTeam = static_cast<int>(pLocalPawn->GetTeam());
	const int nFilter = C::Get<int>(aimbot_target_filter);

	float flBestFOV = flMaxFOV;
	float flBestDist = 999999.0f;
	int nBestHP = 99999;
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

			if (C::Get<bool>(aimbot_visible_only) && !TRACE::IsBoneVisible(pLocalPawn, pPawn, nTargetBone))
				continue;

			Vector3 vecBone{};
			if (!BONES::GetBonePosition(pPawn, nTargetBone, vecBone))
				continue;

			QAngle angToTarget = MATH::CalcAngle(vecEyePos, vecBone);
			float flFOV = MATH::GetFOVDistance(angView, angToTarget);
			if (flFOV > flMaxFOV)
				continue;

			bool bBetter = false;
			switch (nFilter)
			{
			default:
			case 0:
				if (flFOV < flBestFOV)
				{
					flBestFOV = flFOV;
					bBetter = true;
				}
				break;

			case 1:
			{
				const int hp = pPawn->GetHealth();
				if (hp < nBestHP || (hp == nBestHP && flFOV < flBestFOV))
				{
					nBestHP = hp;
					flBestFOV = flFOV;
					bBetter = true;
				}
				break;
			}

			case 2:
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
				bestOut.pPawn = pPawn;
				bestOut.vecBonePos = vecBone;
				bestOut.flFOV = flFOV;
				bestOut.flDistance = flBestDist;
				bestOut.nHealth = pPawn->GetHealth();
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
