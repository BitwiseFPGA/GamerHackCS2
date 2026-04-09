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
// OnPresent — watermark and visual overlays
// ---------------------------------------------------------------
void F::MISC::OnPresent()
{
	if (!C::Get<bool>(misc_watermark))
		return;

	// calculate FPS from frame time
	float flFPS = 0.0f;
	if (I::GlobalVars && I::GlobalVars->flFrameTime > 0.0f)
		flFPS = 1.0f / I::GlobalVars->flFrameTime;

	char szWatermark[128];
	snprintf(szWatermark, sizeof(szWatermark), "GamerHack v1.0.0 | %.0f fps", flFPS);

	D::DrawText(Vector2D(10.0f, 10.0f), Color(180, 140, 255, 230), szWatermark, false);
}

// ---------------------------------------------------------------
// OnCreateMove — movement features
// ---------------------------------------------------------------
void F::MISC::OnCreateMove(CCSGOInput* pInput, CUserCmd* pCmd)
{
	if (!I::Engine || !I::Engine->IsInGame())
		return;

	if (!I::GameEntitySystem)
		return;

	// get local pawn
	const int nLocalIndex = I::Engine->GetLocalPlayer();
	auto* pLocalController = I::GameEntitySystem->Get<CCSPlayerController>(nLocalIndex);
	if (!pLocalController)
		return;

	auto* pLocalPawn = I::GameEntitySystem->Get<C_CSPlayerPawn>(pLocalController->GetPawnHandle());
	if (!pLocalPawn || !pLocalPawn->IsAlive())
		return;

	const int nFlags = pLocalPawn->GetFlags();

	// --- Bunny Hop ---
	if (C::Get<bool>(misc_bhop))
	{
		// if user is holding jump but NOT on the ground, suppress jump
		// this causes jump to fire instantly when touching ground
		if (pCmd->nButtons.nValue & IN_JUMP)
		{
			if (!(nFlags & FL_ONGROUND))
				pCmd->nButtons.nValue &= ~IN_JUMP;
		}
	}

	// --- Auto Strafe ---
	if (C::Get<bool>(misc_autostrafe))
	{
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

	// --- Third Person (toggle via input) ---
	if (C::Get<bool>(misc_thirdperson))
		pInput->bInThirdPerson = true;
	else
		pInput->bInThirdPerson = false;
}

// ---------------------------------------------------------------
// OnFrameStageNotify — per-frame-stage features
// ---------------------------------------------------------------
void F::MISC::OnFrameStageNotify(int nStage)
{
	if (nStage != FRAME_NET_UPDATE_POSTDATAUPDATE_END)
		return;

	if (!I::Engine || !I::Engine->IsInGame())
		return;

	if (!I::GameEntitySystem)
		return;

	// get local pawn
	const int nLocalIndex = I::Engine->GetLocalPlayer();
	auto* pLocalController = I::GameEntitySystem->Get<CCSPlayerController>(nLocalIndex);
	if (!pLocalController)
		return;

	auto* pLocalPawn = I::GameEntitySystem->Get<C_CSPlayerPawn>(pLocalController->GetPawnHandle());
	if (!pLocalPawn)
		return;

	// --- No Flash ---
	if (C::Get<bool>(misc_noflash))
	{
		float flAlpha = C::Get<float>(misc_noflash_alpha);
		pLocalPawn->GetFlashMaxAlpha() = flAlpha;
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
