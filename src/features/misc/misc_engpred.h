#pragma once

class CCSGOInput;
class CUserCmd;

namespace F::MISC::ENGPRED
{
	bool Setup();
	void Destroy();

	// call before feature dispatch in CreateMove
	void Run(CCSGOInput* pInput, CUserCmd* pCmd);

	// call after feature dispatch in CreateMove
	void End(CCSGOInput* pInput, CUserCmd* pCmd);
}
