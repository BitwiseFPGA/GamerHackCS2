#include "legitbot.h"

#include "legitbot_aim.h"
#include "legitbot_core.h"

bool F::LEGITBOT::Setup()
{
	return CORE::Setup();
}

void F::LEGITBOT::Destroy()
{
	CORE::Destroy();
}

void F::LEGITBOT::OnCreateMove(CCSGOInput* pInput, CUserCmd* pCmd)
{
	AIM::OnCreateMove(pInput, pCmd);
}
