#include "misc_movement.h"

#include "../../core/config.h"
#include "../../core/interfaces.h"
#include "../../core/variables.h"
#include "../../sdk/const.h"
#include "../../sdk/entity.h"
#include "../../sdk/functionlist.h"
#include "../../sdk/interfaces/ccsgoinput.h"
#include "../../sdk/interfaces/cgameentitysystem.h"
#include "../../sdk/interfaces/iengineclient.h"
#include "../../utilities/log.h"
#include "../../utilities/xorstr.h"

void F::MISC::MOVEMENT::OnCreateMove(CCSGOInput* pInput, CUserCmd* pCmd)
{
	if (!I::Engine || !I::Engine->IsInGame())
		return;

	if (!I::GameEntitySystem || !pInput || !pCmd)
		return;

	const bool bNeedsPawn = C::Get<bool>(misc_bhop) || C::Get<bool>(misc_autostrafe);
	const bool bNeedsInput = C::Get<bool>(misc_thirdperson);
	if (!bNeedsPawn && !bNeedsInput)
		return;

	__try
	{
		auto* pLocalController = SDK_FUNC::GetLocalPlayerController ?
			SDK_FUNC::GetLocalPlayerController(-1) : nullptr;
		if (!pLocalController)
			return;

		if (bNeedsInput && !bNeedsPawn)
		{
			if (C::Get<bool>(misc_thirdperson))
				pInput->bInThirdPerson = true;
			return;
		}

		if (!pLocalController->IsPawnAlive())
			return;

		auto* pLocalPawn = I::GameEntitySystem->Get<C_CSPlayerPawn>(pLocalController->GetPlayerPawnHandle());
		if (!pLocalPawn || !pLocalPawn->IsAlive())
			return;

		if (C::Get<bool>(misc_bhop))
		{
			const int nFlags = pLocalPawn->GetFlags();
			if ((pCmd->nButtons.nValue & IN_JUMP) && !(nFlags & FL_ONGROUND))
				pCmd->nButtons.nValue &= ~IN_JUMP;
		}

		if (C::Get<bool>(misc_autostrafe))
		{
			const int nFlags = pLocalPawn->GetFlags();
			if (!(nFlags & FL_ONGROUND))
			{
				auto* pBaseCmd = pCmd->csgoUserCmd.pBaseCmd;
				if (pBaseCmd)
				{
					if (pBaseCmd->nMousedX > 0)
						pBaseCmd->flSideMove = 1.0f;
					else if (pBaseCmd->nMousedX < 0)
						pBaseCmd->flSideMove = -1.0f;
				}
			}
		}

		if (C::Get<bool>(misc_thirdperson))
			pInput->bInThirdPerson = true;
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		static bool bLoggedEx = false;
		if (!bLoggedEx)
		{
			bLoggedEx = true;
			const DWORD dwCode = GetExceptionCode();
			L_PRINT(LOG_ERROR) << _XS("[MISC] EXCEPTION in OnCreateMove, code=0x")
				<< reinterpret_cast<void*>(static_cast<uintptr_t>(dwCode));
		}
	}
}
