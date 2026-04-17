#pragma once

#include "bypass.h"

namespace F::BYPASS::ACTIONS
{
	void SetViewAngles(const QAngle& angView, CCSGOInput* pInput, CUserCmd* pCmd,
		bool bAddSetViewAngles = true, bool bOnlyInputHistory = false);
	void SetAttack(CUserCmd* pCmd, bool bAddSubTick = false);
	void SetDontAttack(CUserCmd* pCmd, bool bAddSubTick = false);
	void AddProcessSubTick(uint64_t nButton, bool bPressed);
}
