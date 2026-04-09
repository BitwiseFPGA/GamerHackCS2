#include "features.h"
#include "visuals/visuals.h"
#include "legitbot/legitbot.h"
#include "misc/misc.h"
#include "inventory/inventory.h"
#include "bypass/bypass.h"
#include "../utilities/log.h"
#include "../utilities/trace.h"
#include "../utilities/xorstr.h"

// ---------------------------------------------------------------
// setup / destroy
// ---------------------------------------------------------------
bool F::Setup()
{
	L_PRINT(LOG_INFO) << _XS("[F] initializing features...");

	if (!BYPASS::Setup())
	{
		L_PRINT(LOG_WARNING) << _XS("[F] bypass setup failed");
		return false;
	}

	if (!TRACE::Setup())
	{
		L_PRINT(LOG_WARNING) << _XS("[F] trace setup failed");
		return false;
	}

	if (!VISUALS::Setup())
	{
		L_PRINT(LOG_WARNING) << _XS("[F] visuals setup failed");
		return false;
	}

	if (!LEGITBOT::Setup())
	{
		L_PRINT(LOG_WARNING) << _XS("[F] legitbot setup failed");
		return false;
	}

	if (!MISC::Setup())
	{
		L_PRINT(LOG_WARNING) << _XS("[F] misc setup failed");
		return false;
	}

	if (!INVENTORY::Setup())
	{
		L_PRINT(LOG_WARNING) << _XS("[F] inventory setup failed");
		return false;
	}

	L_PRINT(LOG_INFO) << _XS("[F] all features initialized");
	return true;
}

void F::Destroy()
{
	L_PRINT(LOG_INFO) << _XS("[F] destroying features...");

	INVENTORY::Destroy();
	MISC::Destroy();
	LEGITBOT::Destroy();
	VISUALS::Destroy();
	TRACE::Destroy();
	BYPASS::Destroy();

	L_PRINT(LOG_INFO) << _XS("[F] all features destroyed");
}

// ---------------------------------------------------------------
// dispatch — called from hooks
// ---------------------------------------------------------------
void F::OnPresent()
{
	VISUALS::OnPresent();
	MISC::OnPresent();
}

void F::OnCreateMove(CCSGOInput* pInput, CUserCmd* pCmd)
{
	if (!pInput || !pCmd)
		return;

	// bypass pre-processing (backup state)
	BYPASS::PreCreateMove(pInput, pCmd);

	// feature processing
	LEGITBOT::OnCreateMove(pInput, pCmd);
	MISC::OnCreateMove(pInput, pCmd);

	// bypass post-processing (apply subticks, restore angles)
	BYPASS::PostCreateMove(pInput, pCmd);
}

void F::OnFrameStageNotify(int nStage)
{
	VISUALS::OnFrameStageNotify(nStage);
	MISC::OnFrameStageNotify(nStage);
	INVENTORY::OnFrameStageNotify(nStage);
}

void F::OnOverrideView(void* pViewSetup)
{
	MISC::OnOverrideView(pViewSetup);
}

void F::OnLevelInit(const char* szMapName)
{
	L_PRINT(LOG_INFO) << _XS("[F] level init: ") << (szMapName ? szMapName : _XS("null"));
}

void F::OnLevelShutdown()
{
	L_PRINT(LOG_INFO) << _XS("[F] level shutdown");
}
