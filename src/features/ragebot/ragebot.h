#pragma once

class CCSGOInput;
class CUserCmd;

namespace F::RAGEBOT
{
	bool Setup();
	void Destroy();
	void OnCreateMove(CCSGOInput* pInput, CUserCmd* pCmd);
}
