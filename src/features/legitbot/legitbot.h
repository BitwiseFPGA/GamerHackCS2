#pragma once

class CCSGOInput;
class CUserCmd;

namespace F::LEGITBOT
{
	bool Setup();
	void Destroy();
	void OnCreateMove(CCSGOInput* pInput, CUserCmd* pCmd);
}
