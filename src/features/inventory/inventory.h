#pragma once

namespace F::INVENTORY
{
	bool Setup();
	void Destroy();

	void OnFrameStageNotify(int nStage);

	void ApplySkin(/* per-weapon config — expand as needed */);

	void ForceFullUpdate();
}
