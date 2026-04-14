#include "misc_camera.h"

#include "../../core/config.h"
#include "../../core/variables.h"

void F::MISC::CAMERA::OnOverrideView(void* pViewSetup)
{
	if (!pViewSetup)
		return;

	float flDesiredFOV = C::Get<float>(misc_fov_changer);
	if (flDesiredFOV != 90.0f)
		(void)flDesiredFOV;
}
