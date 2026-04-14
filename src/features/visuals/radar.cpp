#include "radar.h"

#include "../../core/interfaces.h"
#include "../../core/variables.h"
#include "../../sdk/entity.h"
#include "../../sdk/functionlist.h"
#include "../../sdk/interfaces/ccsgoinput.h"
#include "../../sdk/interfaces/cgameentitysystem.h"
#include "../../sdk/interfaces/iengineclient.h"
#include "../../utilities/render.h"

#include <algorithm>
#include <cmath>
#include <imgui.h>

void F::VISUALS::RADAR::Render()
{
	if (!C::Get<bool>(misc_radar_enabled))
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
		if (!pLocalPawn || !pLocalPawn->IsAlive())
			return;

		auto* pLocalSceneNode = pLocalPawn->GetGameSceneNode();
		if (!pLocalSceneNode)
			return;

		const Vector3 vecLocalOrigin = pLocalSceneNode->GetAbsOrigin();

		QAngle angLocal = pLocalPawn->GetEyeAngles();
		if (!angLocal.IsValid() && I::Input)
			angLocal = I::Input->GetViewAngles();

		const float flRadarSize = C::Get<float>(misc_radar_size);
		const float flRadarRange = C::Get<float>(misc_radar_range);
		const float flRadius = flRadarSize * 0.5f;
		const ImVec2 display = ImGui::GetIO().DisplaySize;
		const float flMaxLeft = std::max(0.0f, display.x - flRadarSize);
		const float flMaxTop = std::max(0.0f, display.y - flRadarSize);
		const float flRadarLeft = std::clamp((C::Get<float>(misc_radar_pos_x) * 0.01f) * flMaxLeft, 0.0f, flMaxLeft);
		const float flRadarTop = std::clamp((C::Get<float>(misc_radar_pos_y) * 0.01f) * flMaxTop, 0.0f, flMaxTop);
		const float cx = flRadarLeft + flRadius;
		const float cy = flRadarTop + flRadius;

		const float flYawRad = angLocal.y * (3.14159265f / 180.0f);

		D::DrawCircleFilled(Vector2D(cx, cy), flRadius, Color(0, 0, 0, 150), 48);
		D::DrawCircle(Vector2D(cx, cy), flRadius, Color(100, 100, 100, 180), 48, 1.0f);
		D::DrawLine(Vector2D(cx - flRadius, cy), Vector2D(cx + flRadius, cy), Color(60, 60, 60, 120), 1.0f);
		D::DrawLine(Vector2D(cx, cy - flRadius), Vector2D(cx, cy + flRadius), Color(60, 60, 60, 120), 1.0f);
		D::DrawCircleFilled(Vector2D(cx, cy), 3.0f, Color(255, 255, 255, 230), 8);

		const int nLocalTeam = static_cast<int>(pLocalPawn->GetTeam());

		for (int i = 1; i <= 64; i++)
		{
			__try
			{
				auto* pController = I::GameEntitySystem->Get<CCSPlayerController>(i);
				if (!pController || !pController->IsPawnAlive() || pController == pLocalController)
					continue;

				auto* pPawn = I::GameEntitySystem->Get<C_CSPlayerPawn>(pController->GetPlayerPawnHandle());
				if (!pPawn || pPawn == pLocalPawn || !pPawn->IsAlive())
					continue;

				auto* pSceneNode = pPawn->GetGameSceneNode();
				if (!pSceneNode)
					continue;

				const bool bDormant = pSceneNode->IsDormant();
				const int nTeam = static_cast<int>(pPawn->GetTeam());
				const bool bEnemy = (nTeam != nLocalTeam);

				Vector3 vecEnemyOrigin = pSceneNode->GetAbsOrigin();
				float dx = vecEnemyOrigin.x - vecLocalOrigin.x;
				float dy = vecEnemyOrigin.y - vecLocalOrigin.y;

				const float rotatedX = dx * std::cosf(-flYawRad) - dy * std::sinf(-flYawRad);
				const float rotatedY = dx * std::sinf(-flYawRad) + dy * std::cosf(-flYawRad);

				float scale = flRadius / flRadarRange;
				float radarX = -rotatedY * scale;
				float radarY = -rotatedX * scale;

				float px = cx + radarX;
				float py = cy + radarY;

				float distFromCenter = std::sqrtf((px - cx) * (px - cx) + (py - cy) * (py - cy));
				if (distFromCenter > flRadius - 3.0f)
				{
					float clampScale = (flRadius - 3.0f) / distFromCenter;
					px = cx + (px - cx) * clampScale;
					py = cy + (py - cy) * clampScale;
				}

				const Color dotColor = bEnemy ?
					(bDormant ? Color(255, 80, 80, 100) : Color(255, 40, 40, 230)) :
					(bDormant ? Color(80, 80, 255, 100) : Color(80, 80, 255, 200));

				D::DrawCircleFilled(Vector2D(px, py), 3.0f, dotColor, 8);
			}
			__except (EXCEPTION_EXECUTE_HANDLER)
			{
				continue;
			}
		}

		D::DrawText(Vector2D(cx, cy + flRadius + 2.0f), Color(180, 180, 180, 160), "RADAR", true);
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
	}
}
