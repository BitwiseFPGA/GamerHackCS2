#pragma once

#include "../../sdk/datatypes/qangle.h"
#include "../../sdk/datatypes/vector.h"

class C_CSPlayerPawn;

namespace F::LEGITBOT::TARGETING
{
	struct AimTarget
	{
		C_CSPlayerPawn* pPawn = nullptr;
		Vector3 vecBonePos = {};
		float flFOV = 999.0f;
		float flDistance = 999999.0f;
		int nHealth = 999;
	};

	bool GetBestTarget(const Vector3& vecEyePos, const QAngle& angView, float flMaxFOV,
		int nTargetBone, C_CSPlayerPawn* pLocalPawn, AimTarget& bestOut);
}
