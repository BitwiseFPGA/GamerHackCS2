#pragma once

class CCSGOInput;
class CUserCmd;

namespace F::MISC
{
	bool Setup();
	void Destroy();
	void OnPresent();
	void OnCreateMove(CCSGOInput* pInput, CUserCmd* pCmd);
	void OnFrameStageNotify(int nStage);
	void OnOverrideView(void* pViewSetup);
}
