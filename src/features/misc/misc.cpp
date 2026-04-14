#include "misc.h"

#include "misc_camera.h"
#include "misc_core.h"
#include "misc_effects.h"
#include "misc_movement.h"

bool F::MISC::Setup()
{
	return CORE::Setup();
}

void F::MISC::Destroy()
{
	CORE::Destroy();
}

void F::MISC::OnPresent()
{
	CORE::OnPresent();
}

void F::MISC::OnCreateMove(CCSGOInput* pInput, CUserCmd* pCmd)
{
	MOVEMENT::OnCreateMove(pInput, pCmd);
}

void F::MISC::OnFrameStageNotify(int nStage)
{
	EFFECTS::OnFrameStageNotify(nStage);
}

void F::MISC::OnOverrideView(void* pViewSetup)
{
	CAMERA::OnOverrideView(pViewSetup);
}
