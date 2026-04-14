#include "bypass.h"

#include "bypass_actions.h"
#include "bypass_create_move.h"
#include "bypass_crc.h"
#include "bypass_lifecycle.h"

bool F::BYPASS::Setup()
{
	return LIFECYCLE::Setup();
}

void F::BYPASS::Destroy()
{
	LIFECYCLE::Destroy();
}

void F::BYPASS::PreCreateMove(CCSGOInput* pInput, CUserCmd* pCmd)
{
	CREATE_MOVE::PreCreateMove(pInput, pCmd);
}

void F::BYPASS::PostCreateMove(CCSGOInput* pInput, CUserCmd* pCmd)
{
	CREATE_MOVE::PostCreateMove(pInput, pCmd);
}

void F::BYPASS::SetViewAngles(const QAngle& angView, CCSGOInput* pInput, CUserCmd* pCmd,
	bool bAddSetViewAngles, bool bOnlyInputHistory)
{
	ACTIONS::SetViewAngles(angView, pInput, pCmd, bAddSetViewAngles, bOnlyInputHistory);
}

void F::BYPASS::SetAttack(CUserCmd* pCmd, bool bAddSubTick)
{
	ACTIONS::SetAttack(pCmd, bAddSubTick);
}

void F::BYPASS::SetDontAttack(CUserCmd* pCmd, bool bAddSubTick)
{
	ACTIONS::SetDontAttack(pCmd, bAddSubTick);
}

void F::BYPASS::AddProcessSubTick(uint64_t nButton, bool bPressed)
{
	ACTIONS::AddProcessSubTick(nButton, bPressed);
}

std::string F::BYPASS::SpoofCRC()
{
	return CRC::SpoofCRC();
}
