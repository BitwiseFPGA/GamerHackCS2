#pragma once

class CCSGOInput;
class CUserCmd;

namespace F::VISUALS
{
	bool Setup();
	void Destroy();
	void OnPresent();
	void OnFrameStageNotify(int nStage);
}
