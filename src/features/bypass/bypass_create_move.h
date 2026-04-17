#pragma once

#include "bypass.h"

namespace F::BYPASS::CREATE_MOVE
{
	void PreCreateMove(CCSGOInput* pInput, CUserCmd* pCmd);
	void PostCreateMove(CCSGOInput* pInput, CUserCmd* pCmd);
}
