#include "sniper_crosshair.h"

#include "../../core/interfaces.h"
#include "../../core/variables.h"
#include "../../sdk/entity.h"
#include "../../sdk/functionlist.h"
#include "../../sdk/interfaces/cgameentitysystem.h"
#include "../../sdk/interfaces/iengineclient.h"
#include "../../utilities/render.h"

#include <cstring>
#include <imgui.h>

namespace
{
	static bool IsSniperWeapon(const char* szDesignerName)
	{
		if (!szDesignerName)
			return false;

		return (std::strcmp(szDesignerName, "weapon_awp") == 0 ||
			std::strcmp(szDesignerName, "weapon_ssg08") == 0 ||
			std::strcmp(szDesignerName, "weapon_scar20") == 0 ||
			std::strcmp(szDesignerName, "weapon_g3sg1") == 0);
	}
}

void F::VISUALS::SNIPER_CROSSHAIR::Render()
{
	if (!C::Get<bool>(misc_sniper_crosshair))
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
		if (!pLocalPawn || !pLocalPawn->IsAlive() || pLocalPawn->IsScoped())
			return;

		auto* pWeaponServices = pLocalPawn->GetWeaponServices();
		if (!pWeaponServices)
			return;

		CBaseHandle hWeapon = pWeaponServices->GetActiveWeapon();
		if (!hWeapon.IsValid())
			return;

		auto* pWeapon = I::GameEntitySystem->Get<C_BaseEntity>(hWeapon);
		if (!pWeapon)
			return;

		auto* pId = pWeapon->GetIdentity();
		if (!pId)
			return;

		if (!IsSniperWeapon(pId->GetDesignerName()))
			return;

		const ImVec2 display = ImGui::GetIO().DisplaySize;
		const float cx = display.x * 0.5f;
		const float cy = display.y * 0.5f;
		constexpr float sz = 6.0f;
		constexpr float gap = 2.0f;

		const Color col(0, 255, 0, 230);
		D::DrawLine(Vector2D(cx - sz - gap, cy), Vector2D(cx - gap, cy), col, 1.5f);
		D::DrawLine(Vector2D(cx + gap, cy), Vector2D(cx + sz + gap, cy), col, 1.5f);
		D::DrawLine(Vector2D(cx, cy - sz - gap), Vector2D(cx, cy - gap), col, 1.5f);
		D::DrawLine(Vector2D(cx, cy + gap), Vector2D(cx, cy + sz + gap), col, 1.5f);
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
	}
}
