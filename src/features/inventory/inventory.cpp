#include "inventory.h"

#include "inventory_actions.h"
#include "inventory_runtime.h"
#include "inventory_state.h"

bool F::INVENTORY::Setup()
{
	return STATE::Setup();
}

void F::INVENTORY::Destroy()
{
	STATE::Destroy();
}

void F::INVENTORY::OnFrameStageNotify(int nStage)
{
	RUNTIME::OnFrameStageNotify(nStage);
}

void F::INVENTORY::ApplySkin()
{
	ACTIONS::ApplySkin();
}

void F::INVENTORY::ForceFullUpdate()
{
	ACTIONS::ForceFullUpdate();
}
