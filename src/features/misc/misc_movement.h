#pragma once

class CCSGOInput;
class CUserCmd;
struct QAngle;

namespace F::MISC::MOVEMENT
{
	void OnCreateMove(CCSGOInput* pInput, CUserCmd* pCmd);

	// correct movement input after view angle changes (call after aimbot)
	void MovementFix(CUserCmd* pCmd, const QAngle& angOldView);
}
