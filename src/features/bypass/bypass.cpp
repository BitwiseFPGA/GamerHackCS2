#include "bypass.h"

#include "../../core/interfaces.h"
#include "../../utilities/log.h"
#include "../../utilities/memory.h"
#include "../../utilities/xorstr.h"
#include "../../sdk/interfaces/ccsgoinput.h"
#include "../../sdk/interfaces/iglobalvars.h"

// ---------------------------------------------------------------
// setup / destroy
// ---------------------------------------------------------------
bool F::BYPASS::Setup()
{
	vecSubTicks.clear();
	vecSubTicks.reserve(16);

	L_PRINT(LOG_INFO) << _XS("[BYPASS] initialized");
	return true;
}

void F::BYPASS::Destroy()
{
	vecSubTicks.clear();
	L_PRINT(LOG_INFO) << _XS("[BYPASS] destroyed");
}

// ---------------------------------------------------------------
// PreCreateMove — backup original command state, clear subtick queue
// ---------------------------------------------------------------
void F::BYPASS::PreCreateMove(CCSGOInput* pInput, CUserCmd* pCmd)
{
	if (!pInput || !pCmd)
		return;

	static bool bFirstPreCall = false;
	if (!bFirstPreCall) { bFirstPreCall = true; L_PRINT(LOG_INFO) << _XS("[BYPASS] PreCreateMove first call"); }

	// clear subtick queue for this tick
	vecSubTicks.clear();
}

// ---------------------------------------------------------------
// PostCreateMove — apply subtick entries, restore angles for AC
// ---------------------------------------------------------------
void F::BYPASS::PostCreateMove(CCSGOInput* pInput, CUserCmd* pCmd)
{
	if (!pInput || !pCmd)
		return;

	// apply queued subtick entries to the command
	if (!vecSubTicks.empty())
	{
		CBaseUserCmdPB* pBaseCmd = pCmd->csgoUserCmd.pBaseCmd;
		if (pBaseCmd && pBaseCmd->subtickMovesField.pRep)
		{
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
	}
}

// ---------------------------------------------------------------
// SetViewAngles — safely set view angles through input history
// ---------------------------------------------------------------
void F::BYPASS::SetViewAngles(const QAngle& angView, CCSGOInput* pInput, CUserCmd* pCmd,
                              bool bAddSetViewAngles, bool bOnlyInputHistory)
{
	if (!pInput || !pCmd)
		return;

	QAngle angClamped = angView;
	angClamped.Clamp();

	// always set view angles in input history entries for server-side consistency
	pCmd->SetSubTickAngle(angClamped);

	if (!bOnlyInputHistory)
	{
		// set via the CCSGOInput system (updates client-side prediction)
		if (bAddSetViewAngles)
			pInput->SetViewAngle(angClamped);

		// also set on the base command protobuf
		CBaseUserCmdPB* pBaseCmd = pCmd->csgoUserCmd.pBaseCmd;
		if (pBaseCmd && pBaseCmd->pViewAngles)
		{
			pBaseCmd->pViewAngles->angValue = angClamped;
			pBaseCmd->SetBits(BASE_BITS_VIEWANGLES);
		}
	}
}

// ---------------------------------------------------------------
// SetAttack — press attack button with optional subtick
// ---------------------------------------------------------------
void F::BYPASS::SetAttack(CUserCmd* pCmd, bool bAddSubTick)
{
	if (!pCmd)
		return;

	// set button state on the runtime button state
	pCmd->nButtons.nValue |= IN_ATTACK;
	pCmd->nButtons.nValueChanged |= IN_ATTACK;

	// set on protobuf button state
	CBaseUserCmdPB* pBaseCmd = pCmd->csgoUserCmd.pBaseCmd;
	if (pBaseCmd && pBaseCmd->pInButtonState)
	{
		pBaseCmd->pInButtonState->nValue |= IN_ATTACK;
		pBaseCmd->pInButtonState->nValueChanged |= IN_ATTACK;
		pBaseCmd->pInButtonState->SetBits(BUTTON_STATE_PB_BITS_BUTTONSTATE1 | BUTTON_STATE_PB_BITS_BUTTONSTATE2);
	}

	if (bAddSubTick)
		AddProcessSubTick(IN_ATTACK, true);
}

// ---------------------------------------------------------------
// SetDontAttack — release attack button with optional subtick
// ---------------------------------------------------------------
void F::BYPASS::SetDontAttack(CUserCmd* pCmd, bool bAddSubTick)
{
	if (!pCmd)
		return;

	// clear attack from runtime button state
	pCmd->nButtons.nValue &= ~IN_ATTACK;

	// clear from protobuf button state
	CBaseUserCmdPB* pBaseCmd = pCmd->csgoUserCmd.pBaseCmd;
	if (pBaseCmd && pBaseCmd->pInButtonState)
	{
		pBaseCmd->pInButtonState->nValue &= ~IN_ATTACK;
		pBaseCmd->pInButtonState->SetBits(BUTTON_STATE_PB_BITS_BUTTONSTATE1);
	}

	if (bAddSubTick)
		AddProcessSubTick(IN_ATTACK, false);
}

// ---------------------------------------------------------------
// AddProcessSubTick — queue a subtick button event
// ---------------------------------------------------------------
void F::BYPASS::AddProcessSubTick(uint64_t nButton, bool bPressed)
{
	// calculate the 'when' value based on current tick fraction
	float flWhen = 0.0f;

	if (I::GlobalVars)
	{
		// use a fraction within the current tick
		flWhen = I::GlobalVars->flFrameTime > 0.0f
			? std::clamp(I::GlobalVars->flFrameTime / I::GlobalVars->flIntervalPerTick, 0.0f, 1.0f)
			: 0.0f;
	}

	vecSubTicks.push_back({ nButton, bPressed, flWhen });
}

// ---------------------------------------------------------------
// SpoofCRC — generate valid CRC for modified protobuf data
// ---------------------------------------------------------------
std::string F::BYPASS::SpoofCRC()
{
	// CRC spoofing for SerializePartialToArray
	// The game computes a CRC over the serialized protobuf command data.
	// After modifying command fields, we need to recompute this CRC
	// to match what the server expects.

	// pattern scan for the CRC function
	static auto fnComputeMoveCrc = reinterpret_cast<const char*(__fastcall*)(void*)>(
		MEM::FindPattern(_XS("client.dll"),
			_XS("48 89 5C 24 ? 48 89 6C 24 ? 48 89 74 24 ? 57 41 56 41 57 48 83 EC ? 48 8B E9 48 8D 35")));

	if (!fnComputeMoveCrc)
		return {};

	// TODO: implement full CRC spoofing once protobuf serialization is hooked
	return {};
}
