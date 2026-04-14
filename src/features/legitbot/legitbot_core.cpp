#include "legitbot_core.h"

#include "../../utilities/log.h"
#include "../../utilities/xorstr.h"

bool F::LEGITBOT::CORE::Setup()
{
	L_PRINT(LOG_INFO) << _XS("[LEGITBOT] initialized");
	return true;
}

void F::LEGITBOT::CORE::Destroy()
{
	L_PRINT(LOG_INFO) << _XS("[LEGITBOT] destroyed");
}
