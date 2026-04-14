#include "visuals.h"

#include "esp.h"
#include "radar.h"
#include "sniper_crosshair.h"

#include "../../utilities/log.h"
#include "../../utilities/xorstr.h"

bool F::VISUALS::Setup()
{
	L_PRINT(LOG_INFO) << _XS("[VISUALS] initialized");
	return true;
}

void F::VISUALS::Destroy()
{
	L_PRINT(LOG_INFO) << _XS("[VISUALS] destroyed");
}

void F::VISUALS::OnPresent()
{
	ESP::Render();
	SNIPER_CROSSHAIR::Render();
	RADAR::Render();
}

void F::VISUALS::OnFrameStageNotify(int nStage)
{
	(void)nStage;
}

void F::VISUALS::DrawSniperCrosshair()
{
	SNIPER_CROSSHAIR::Render();
}

void F::VISUALS::DrawRadar()
{
	RADAR::Render();
}
