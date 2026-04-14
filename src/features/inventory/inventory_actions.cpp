#include "inventory_actions.h"

#include "../../utilities/log.h"
#include "../../utilities/xorstr.h"

void F::INVENTORY::ACTIONS::ApplySkin()
{
	L_PRINT(LOG_INFO) << _XS("[INVENTORY] ApplySkin called");
}

void F::INVENTORY::ACTIONS::ForceFullUpdate()
{
	L_PRINT(LOG_INFO) << _XS("[INVENTORY] force full update requested");
}
