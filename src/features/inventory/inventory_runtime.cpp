#include "inventory_runtime.h"

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

void F::INVENTORY::RUNTIME::OnFrameStageNotify(int nStage)
{
	if (nStage != FRAME_NET_UPDATE_POSTDATAUPDATE_END)
		return;

	if (!C::Get<bool>(inventory_enabled))
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

		auto* pLocalPawn = I::GameEntitySystem->Get<C_CSPlayerPawn>(pLocalController->GetPawnHandle());
		if (!pLocalPawn)
			return;

		auto* pWeaponServices = pLocalPawn->GetWeaponServices();
		if (!pWeaponServices)
			return;

		(void)pWeaponServices;
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		static bool bLogged = false;
		if (!bLogged)
		{
			bLogged = true;
			L_PRINT(LOG_ERROR) << _XS("[INVENTORY] EXCEPTION in OnFrameStageNotify");
		}
	}
}
