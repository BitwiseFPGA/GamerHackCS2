#pragma once
#include "../../sdk/datatypes/vector.h"

class C_CSPlayerPawn;
class CCSWeaponBaseVData;

namespace F::LEGITBOT::AUTOWALL
{
	struct PenetrationData_t
	{
		float m_flDamage = 0.0f;
		int   m_nHitGroup = 0;
		int   m_nHitbox = 0;
		bool  m_bPenetrated = true; // true = direct hit (no wall), false = through wall
	};

	// scale raw weapon damage by hitgroup, armor, etc.
	void ScaleDamage(int nHitGroup, C_CSPlayerPawn* pTarget,
	                 CCSWeaponBaseVData* pWeaponData, float& flDamage);

	// simulate a bullet from start to end, accounting for penetration
	// returns true if the bullet can reach a player and do > 0 damage
	bool FireBullet(const Vector3& vecStart, const Vector3& vecEnd,
	                C_CSPlayerPawn* pTarget, CCSWeaponBaseVData* pWeaponData,
	                PenetrationData_t& penData, bool bIsTaser = false);

	// quick check: can we damage target from our current position?
	float GetDamageToTarget(C_CSPlayerPawn* pLocalPawn, C_CSPlayerPawn* pTarget,
	                        const Vector3& vecTargetPos);

	bool Setup();
	void Destroy();
}
