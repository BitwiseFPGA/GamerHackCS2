#include "bypass_lifecycle.h"

#include "bypass.h"

#include "../../utilities/log.h"
#include "../../utilities/xorstr.h"

bool F::BYPASS::LIFECYCLE::Setup()
{
	vecSubTicks.clear();
	vecSubTicks.reserve(16);

	L_PRINT(LOG_INFO) << _XS("[BYPASS] initialized");
	return true;
}

void F::BYPASS::LIFECYCLE::Destroy()
{
	vecSubTicks.clear();
	L_PRINT(LOG_INFO) << _XS("[BYPASS] destroyed");
}
