#include "bypass_create_move.h"

#include "../../utilities/log.h"
#include "../../utilities/xorstr.h"

void F::BYPASS::CREATE_MOVE::PreCreateMove(CCSGOInput* pInput, CUserCmd* pCmd)
{
	if (!pInput || !pCmd)
		return;

	static bool bFirstPreCall = false;
	if (!bFirstPreCall)
	{
		bFirstPreCall = true;
		L_PRINT(LOG_INFO) << _XS("[BYPASS] PreCreateMove first call");
	}

	vecSubTicks.clear();
}

void F::BYPASS::CREATE_MOVE::PostCreateMove(CCSGOInput* pInput, CUserCmd* pCmd)
{
	if (!pInput || !pCmd)
		return;

	if (vecSubTicks.empty())
		return;

	CBaseUserCmdPB* pBaseCmd = pCmd->csgoUserCmd.pBaseCmd;
	if (!pBaseCmd || !pBaseCmd->subtickMovesField.pRep)
		return;

	for (const auto& entry : vecSubTicks)
	{
		const int nCurrentSize = pBaseCmd->subtickMovesField.nCurrentSize;
		if (nCurrentSize >= pBaseCmd->subtickMovesField.pRep->nAllocatedSize)
			break;

		CSubtickMoveStep* pStep = pBaseCmd->subtickMovesField.pRep->tElements[nCurrentSize];
		if (!pStep)
			continue;

		pStep->nButton = entry.nButton;
		pStep->bPressed = entry.bPressed;
		pStep->flWhen = entry.flWhen;
		pStep->SetBits(MOVESTEP_BITS_BUTTON | MOVESTEP_BITS_PRESSED | MOVESTEP_BITS_WHEN);

		pBaseCmd->subtickMovesField.nCurrentSize++;
	}
}
