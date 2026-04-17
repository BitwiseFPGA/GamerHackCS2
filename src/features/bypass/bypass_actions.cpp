#include "bypass_actions.h"

#include "../../core/interfaces.h"
#include "../../sdk/interfaces/ccsgoinput.h"
#include "../../sdk/interfaces/iglobalvars.h"

#include <algorithm>

void F::BYPASS::ACTIONS::SetViewAngles(const QAngle& angView, CCSGOInput* pInput, CUserCmd* pCmd,
	bool bAddSetViewAngles, bool bOnlyInputHistory)
{
	if (!pInput || !pCmd)
		return;

	QAngle angClamped = angView;
	angClamped.Clamp();

	pCmd->SetSubTickAngle(angClamped);

	if (bOnlyInputHistory)
		return;

	if (bAddSetViewAngles)
		pInput->SetViewAngle(angClamped);

	CBaseUserCmdPB* pBaseCmd = pCmd->csgoUserCmd.pBaseCmd;
	if (pBaseCmd && pBaseCmd->pViewAngles)
	{
		pBaseCmd->pViewAngles->angValue = angClamped;
		pBaseCmd->SetBits(BASE_BITS_VIEWANGLES);
	}
}

void F::BYPASS::ACTIONS::SetAttack(CUserCmd* pCmd, bool bAddSubTick)
{
	if (!pCmd)
		return;

	pCmd->nButtons.nValue |= IN_ATTACK;
	pCmd->nButtons.nValueChanged |= IN_ATTACK;

	CBaseUserCmdPB* pBaseCmd = pCmd->csgoUserCmd.pBaseCmd;
	if (pBaseCmd && pBaseCmd->pInButtonState)
	{
		pBaseCmd->pInButtonState->nValue |= IN_ATTACK;
		pBaseCmd->pInButtonState->nValueChanged |= IN_ATTACK;
		pBaseCmd->pInButtonState->SetBits(BUTTON_STATE_PB_BITS_BUTTONSTATE1 | BUTTON_STATE_PB_BITS_BUTTONSTATE2);
		pBaseCmd->SetBits(BASE_BITS_BUTTONPB);
	}

	pCmd->csgoUserCmd.nAttack1StartHistoryIndex = 0;
	pCmd->csgoUserCmd.CheckAndSetBits(CSGOUSERCMD_BITS_ATTACK1START);

	if (bAddSubTick)
		AddProcessSubTick(IN_ATTACK, true);
}

void F::BYPASS::ACTIONS::SetDontAttack(CUserCmd* pCmd, bool bAddSubTick)
{
	if (!pCmd)
		return;

	pCmd->nButtons.nValue &= ~IN_ATTACK;

	CBaseUserCmdPB* pBaseCmd = pCmd->csgoUserCmd.pBaseCmd;
	if (pBaseCmd && pBaseCmd->pInButtonState)
	{
		pBaseCmd->pInButtonState->nValue &= ~IN_ATTACK;
		pBaseCmd->pInButtonState->SetBits(BUTTON_STATE_PB_BITS_BUTTONSTATE1);
		pBaseCmd->SetBits(BASE_BITS_BUTTONPB);
	}

	if (bAddSubTick)
		AddProcessSubTick(IN_ATTACK, false);
}

void F::BYPASS::ACTIONS::AddProcessSubTick(uint64_t nButton, bool bPressed)
{
	float flWhen = 0.0f;

	if (I::GlobalVars)
	{
		flWhen = I::GlobalVars->flFrameTime > 0.0f
			? std::clamp(I::GlobalVars->flFrameTime / I::GlobalVars->flIntervalPerTick, 0.0f, 1.0f)
			: 0.0f;
	}

	vecSubTicks.push_back({ nButton, bPressed, flWhen });
}
