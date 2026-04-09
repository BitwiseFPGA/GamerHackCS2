#pragma once

class CCSGOInput;
class CUserCmd;

namespace F
{
	bool Setup();
	void Destroy();

	void OnPresent();
	void OnCreateMove(CCSGOInput* pInput, CUserCmd* pCmd);
	void OnFrameStageNotify(int nStage);
	void OnOverrideView(void* pViewSetup);
	void OnLevelInit(const char* szMapName);
	void OnLevelShutdown();
}

// sub-namespace headers
#include "bypass/bypass.h"
