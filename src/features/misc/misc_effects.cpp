#include "misc_effects.h"

#include "../../core/config.h"
#include "../../core/interfaces.h"
#include "../../core/variables.h"
#include "../../sdk/common.h"
#include "../../sdk/entity.h"
#include "../../sdk/functionlist.h"
#include "../../sdk/interfaces/cgameentitysystem.h"
#include "../../sdk/interfaces/iengineclient.h"
#include "../../utilities/log.h"
#include "../../utilities/xorstr.h"

void F::MISC::EFFECTS::OnFrameStageNotify(int nStage)
{
	if (nStage != FRAME_NET_UPDATE_POSTDATAUPDATE_END)
		return;

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
		if (!pLocalController || !pLocalController->IsPawnAlive())
			return;

		auto* pLocalPawn = I::GameEntitySystem->Get<C_CSPlayerPawn>(pLocalController->GetPlayerPawnHandle());
		if (!pLocalPawn)
			return;

		pLocalPawn->GetFlashMaxAlpha() = C::Get<float>(misc_noflash_alpha);
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
