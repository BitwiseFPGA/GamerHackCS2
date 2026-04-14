#include "misc_core.h"

#include "../../utilities/log.h"
#include "../../utilities/xorstr.h"

bool F::MISC::CORE::Setup()
{
	L_PRINT(LOG_INFO) << _XS("[MISC] initialized");
	return true;
}

void F::MISC::CORE::Destroy()
{
	L_PRINT(LOG_INFO) << _XS("[MISC] destroyed");
}

void F::MISC::CORE::OnPresent()
{
}
