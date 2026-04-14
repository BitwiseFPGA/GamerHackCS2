#include "misc.h"

#include "../../core/config.h"
#include "../../core/variables.h"
#include "../../core/interfaces.h"
#include "../../utilities/xorstr.h"
#include "../../utilities/render.h"
#include "../../utilities/log.h"
#include "../../sdk/entity.h"
#include "../../sdk/interfaces/cgameentitysystem.h"
#include "../../sdk/interfaces/ccsgoinput.h"
#include "../../sdk/interfaces/iengineclient.h"
#include "../../sdk/interfaces/iglobalvars.h"
#include "../../sdk/datatypes/usercmd.h"
#include "../../sdk/const.h"

#include "../../sdk/common.h"
#include "../../sdk/functionlist.h"

#include <cstdio>
#include <cmath>

// ---------------------------------------------------------------
// setup / destroy
// ---------------------------------------------------------------
bool F::MISC::Setup()
{
	L_PRINT(LOG_INFO) << _XS("[MISC] initialized");
	return true;
}

void F::MISC::Destroy()
{
	L_PRINT(LOG_INFO) << _XS("[MISC] destroyed");
}

// ---------------------------------------------------------------
// OnPresent — visual overlays (watermark handled by MENU system)
// ---------------------------------------------------------------
void F::MISC::OnPresent()
{
	// watermark is now handled by MENU::RenderWatermark()
}

// ---------------------------------------------------------------
// OnCreateMove — movement features
// ---------------------------------------------------------------
void F::MISC::OnCreateMove(CCSGOInput* pInput, CUserCmd* pCmd)
{
	if (!I::Engine || !I::Engine->IsInGame())
		return;

	if (!I::GameEntitySystem || !pInput || !pCmd)
		return;

	// check if any CreateMove feature is enabled before touching entities
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

		// thirdperson only needs CCSGOInput, not pawn
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

		// --- Bunny Hop ---
		if (C::Get<bool>(misc_bhop))
		{
			const int nFlags = pLocalPawn->GetFlags();
			if (pCmd->nButtons.nValue & IN_JUMP)
			{
				if (!(nFlags & FL_ONGROUND))
					pCmd->nButtons.nValue &= ~IN_JUMP;
			}
		}

		// --- Auto Strafe ---
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

		// --- Third Person ---
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

// ---------------------------------------------------------------
// OnFrameStageNotify — per-frame-stage features
// ---------------------------------------------------------------
void F::MISC::OnFrameStageNotify(int nStage)
{
	if (nStage != FRAME_NET_UPDATE_POSTDATAUPDATE_END)
		return;

	// only access entities if noflash is enabled
	if (!C::Get<bool>(misc_noflash))
		return;

	if (!I::Engine || !I::Engine->IsInGame())
		return;

	if (!I::GameEntitySystem)
		return;

	__try
	{
		auto* pLocalController = SDK_FUNC::GetLocalPlayerController ?
			SDK_FUNC::GetLocalPlayerController(-1) : nullptr;
		if (!pLocalController)
			return;

		if (!pLocalController->IsPawnAlive())
			return;

		auto* pLocalPawn = I::GameEntitySystem->Get<C_CSPlayerPawn>(pLocalController->GetPlayerPawnHandle());
		if (!pLocalPawn)
			return;

		float flAlpha = C::Get<float>(misc_noflash_alpha);
		pLocalPawn->GetFlashMaxAlpha() = flAlpha;
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		static bool bLogged = false;
		if (!bLogged)
		{
			bLogged = true;
			const DWORD dwCode = GetExceptionCode();
			L_PRINT(LOG_ERROR) << _XS("[MISC] EXCEPTION in OnFrameStageNotify, code=0x")
				<< reinterpret_cast<void*>(static_cast<uintptr_t>(dwCode));
		}
	}
}

// ---------------------------------------------------------------
// OnOverrideView — camera modifications
// ---------------------------------------------------------------
void F::MISC::OnOverrideView(void* pViewSetup)
{
	if (!pViewSetup)
		return;

	// FOV Changer
	// CViewSetup FOV field offset varies by game build.
	// The typical approach is to cast pViewSetup and modify the FOV member.
	// Example (adjust offset for current build):
	//   float* pFOV = reinterpret_cast<float*>(reinterpret_cast<std::uintptr_t>(pViewSetup) + FOV_OFFSET);
	//   *pFOV = C::Get<float>(misc_fov_changer);

	float flDesiredFOV = C::Get<float>(misc_fov_changer);
	if (flDesiredFOV != 90.0f)
	{
		// TODO: apply once CViewSetup FOV offset is resolved for current build
		(void)flDesiredFOV;
	}
}
