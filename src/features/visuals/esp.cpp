#include "esp.h"

#include "../../core/config.h"
#include "../../core/interfaces.h"
#include "../../core/variables.h"
#include "../../sdk/datatypes/color.h"
#include "../../sdk/entity.h"
#include "../../sdk/functionlist.h"
#include "../../sdk/interfaces/cgameentitysystem.h"
#include "../../sdk/interfaces/iengineclient.h"
#include "../../utilities/log.h"
#include "../../utilities/render.h"
#include "../../utilities/xorstr.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <imgui.h>
#include <limits>

namespace
{
	struct BBox
	{
		Vector2D min;
		Vector2D max;
		float width = 0.0f;
		float height = 0.0f;
	};

	static Color GetPlayerESPColor(int nTeam)
	{
		return (nTeam == TEAM_TT) ? C::Get<Color>(esp_box_color_t) : C::Get<Color>(esp_box_color_ct);
	}

	static Color GetPlayerSnaplineColor(int nTeam)
	{
		return (nTeam == TEAM_TT) ? C::Get<Color>(esp_snapline_color_t) : C::Get<Color>(esp_snapline_color_ct);
	}

	static bool GetBoundingBox(C_CSPlayerPawn* pPawn, BBox& bbox)
	{
		auto* pSceneNode = pPawn->GetGameSceneNode();
		auto* pCollision = pPawn->GetCollision();
		if (!pSceneNode || !pCollision)
			return false;

		const CTransform& nodeToWorld = pSceneNode->GetNodeToWorld();
		const Matrix3x4 matTransform = nodeToWorld.quatOrientation.ToMatrix(nodeToWorld.vecPosition);
		const Vector3 vecMins = pCollision->GetMins();
		const Vector3 vecMaxs = pCollision->GetMaxs();

		const Vector3 points[8] =
		{
			{ vecMins.x, vecMins.y, vecMins.z },
			{ vecMins.x, vecMaxs.y, vecMins.z },
			{ vecMaxs.x, vecMaxs.y, vecMins.z },
			{ vecMaxs.x, vecMins.y, vecMins.z },
			{ vecMaxs.x, vecMaxs.y, vecMaxs.z },
			{ vecMins.x, vecMaxs.y, vecMaxs.z },
			{ vecMins.x, vecMins.y, vecMaxs.z },
			{ vecMaxs.x, vecMins.y, vecMaxs.z }
		};

		float screenMinX = std::numeric_limits<float>::max();
		float screenMinY = std::numeric_limits<float>::max();
		float screenMaxX = -std::numeric_limits<float>::max();
		float screenMaxY = -std::numeric_limits<float>::max();

		for (const Vector3& localPoint : points)
		{
			const Vector3 pt{
				localPoint.x * matTransform[0][0] + localPoint.y * matTransform[0][1] + localPoint.z * matTransform[0][2] + matTransform[0][3],
				localPoint.x * matTransform[1][0] + localPoint.y * matTransform[1][1] + localPoint.z * matTransform[1][2] + matTransform[1][3],
				localPoint.x * matTransform[2][0] + localPoint.y * matTransform[2][1] + localPoint.z * matTransform[2][2] + matTransform[2][3]
			};

			Vector2D screen{};
			if (!D::WorldToScreen(pt, &screen))
				return false;

			if (screen.x < screenMinX) screenMinX = screen.x;
			if (screen.y < screenMinY) screenMinY = screen.y;
			if (screen.x > screenMaxX) screenMaxX = screen.x;
			if (screen.y > screenMaxY) screenMaxY = screen.y;
		}

		const ImVec2 displaySize = ImGui::GetIO().DisplaySize;
		if (screenMaxX < 0.0f || screenMaxY < 0.0f || screenMinX > displaySize.x || screenMinY > displaySize.y)
			return false;

		screenMinX = std::clamp(screenMinX, 0.0f, displaySize.x);
		screenMinY = std::clamp(screenMinY, 0.0f, displaySize.y);
		screenMaxX = std::clamp(screenMaxX, 0.0f, displaySize.x);
		screenMaxY = std::clamp(screenMaxY, 0.0f, displaySize.y);

		bbox.width = screenMaxX - screenMinX;
		bbox.height = screenMaxY - screenMinY;
		if (bbox.height < 2.0f || bbox.width < 2.0f)
			return false;

		bbox.min = Vector2D(screenMinX, screenMinY);
		bbox.max = Vector2D(screenMaxX, screenMaxY);
		return true;
	}

	static void DrawPlayerBox(const BBox& bbox, int nTeam)
	{
		const int nBoxType = C::Get<int>(esp_box_type);
		const Color colBox = GetPlayerESPColor(nTeam);

		if (nBoxType == 1)
			D::DrawCornerBox(bbox.min, Vector2D(bbox.width, bbox.height), colBox, 1.5f, 0.25f);
		else
			D::DrawRect(bbox.min, Vector2D(bbox.width, bbox.height), colBox, 0.0f, 1.5f);
	}

	static void DrawPlayerName(const BBox& bbox, CCSPlayerController* pController)
	{
		if (!pController)
			return;

		const char* szName = pController->GetPlayerName();
		if (!szName || szName[0] == '\0')
			return;

		D::DrawText(Vector2D(bbox.min.x + bbox.width * 0.5f, bbox.min.y - 14.0f), C::Get<Color>(esp_name_color), szName, true);
	}

	static void DrawPlayerHealth(const BBox& bbox, C_CSPlayerPawn* pPawn)
	{
		int nHealth = pPawn->GetHealth();
		int nMaxHealth = pPawn->GetMaxHealth();
		if (nMaxHealth <= 0)
			nMaxHealth = 100;

		D::DrawHealthBar(bbox.min, bbox.height, nHealth, nMaxHealth);
	}

	static void DrawPlayerArmor(const BBox& bbox, C_CSPlayerPawn* pPawn)
	{
		int nArmor = pPawn->GetArmorValue();
		if (nArmor <= 0)
			return;

		constexpr float BAR_WIDTH = 4.0f;
		const float flFrac = static_cast<float>(nArmor) / 100.0f;
		const float flBarH = bbox.height * flFrac;
		const float flOff = bbox.height - flBarH;
		const float flX = bbox.max.x + 2.0f;

		D::DrawRectFilled(Vector2D(flX, bbox.min.y - 1.0f),
			Vector2D(BAR_WIDTH + 2.0f, bbox.height + 2.0f), Color(0, 0, 0, 180));
		D::DrawRectFilled(Vector2D(flX + 1.0f, bbox.min.y + flOff),
			Vector2D(BAR_WIDTH, flBarH), C::Get<Color>(esp_armor_color));
	}

	static void DrawPlayerWeapon(const BBox& bbox, C_CSPlayerPawn* pPawn)
	{
		auto* pWeaponServices = pPawn->GetWeaponServices();
		if (!pWeaponServices)
			return;

		auto hActive = pWeaponServices->GetActiveWeapon();
		if (!hActive.IsValid())
			return;

		auto* pWeapon = I::GameEntitySystem->Get<C_BaseEntity>(hActive);
		if (!pWeapon)
			return;

		auto* pId = pWeapon->GetIdentity();
		if (!pId)
			return;

		const char* szDesigner = pId->GetDesignerName();
		if (!szDesigner)
			return;

		const char* szDisplay = szDesigner;
		if (std::strncmp(szDisplay, "weapon_", 7) == 0)
			szDisplay += 7;

		D::DrawText(Vector2D(bbox.min.x + bbox.width * 0.5f, bbox.max.y + 2.0f), C::Get<Color>(esp_weapon_color), szDisplay, true);
	}

	static void DrawPlayerSnapline(const BBox& bbox, int nTeam)
	{
		const ImVec2 display = ImGui::GetIO().DisplaySize;
		D::DrawLine(Vector2D(display.x * 0.5f, display.y),
			Vector2D(bbox.min.x + bbox.width * 0.5f, bbox.max.y),
			GetPlayerSnaplineColor(nTeam), 1.0f);
	}
}

void F::VISUALS::ESP::Render()
{
	if (!C::Get<bool>(esp_enabled))
		return;

	if (!I::Engine || !I::Engine->IsInGame())
		return;

	if (!I::GameEntitySystem)
		return;

	static bool bLoggedInGame = false;
	if (!bLoggedInGame)
	{
		bLoggedInGame = true;
		L_PRINT(LOG_INFO) << _XS("[VISUALS] IsInGame true — first ESP frame");
	}

	static int nFrame = 0;
	nFrame++;
	const bool bDiag = (nFrame <= 3) || (nFrame % 300 == 0);

	__try
	{
		CCSPlayerController* pLocalController = SDK_FUNC::GetLocalPlayerController ?
			SDK_FUNC::GetLocalPlayerController(-1) : nullptr;
		if (!pLocalController)
			return;

		const bool bLocalAlive = pLocalController->IsPawnAlive();

		C_CSPlayerPawn* pLocalPawn = nullptr;
		CBaseHandle hPawn = pLocalController->GetPlayerPawnHandle();
		if (hPawn.IsValid())
			pLocalPawn = I::GameEntitySystem->Get<C_CSPlayerPawn>(hPawn);

		const int nLocalTeam = pLocalPawn ?
			static_cast<int>(pLocalPawn->GetTeam()) :
			static_cast<int>(pLocalController->GetTeam());

		if (bDiag && nFrame <= 3)
			L_PRINT(LOG_INFO) << _XS("[ESP] localPawn=") << static_cast<void*>(pLocalPawn)
			<< _XS(" alive=") << bLocalAlive
			<< _XS(" team=") << nLocalTeam;

		int nFound = 0;
		int nDrawn = 0;

		for (int i = 1; i <= 64; i++)
		{
			__try
			{
				auto* pController = I::GameEntitySystem->Get<CCSPlayerController>(i);
				if (!pController)
					continue;

				nFound++;

				if (!pController->IsPawnAlive())
					continue;

				if (pController == pLocalController)
					continue;

				CBaseHandle hEntPawn = pController->GetPlayerPawnHandle();
				if (!hEntPawn.IsValid())
					continue;

				auto* pPawn = I::GameEntitySystem->Get<C_CSPlayerPawn>(hEntPawn);
				if (!pPawn || pPawn == pLocalPawn)
					continue;

				auto* pSceneNode = pPawn->GetGameSceneNode();
				if (!pSceneNode)
					continue;

				const int nTeam = static_cast<int>(pPawn->GetTeam());
				if (nTeam == nLocalTeam && !C::Get<bool>(esp_team))
					continue;

				if (!pPawn->IsAlive())
					continue;

				const bool bNeedsBBox =
					C::Get<bool>(esp_box) ||
					C::Get<bool>(esp_name) ||
					C::Get<bool>(esp_health) ||
					C::Get<bool>(esp_armor) ||
					(C::Get<bool>(esp_snaplines) && bLocalAlive) ||
					C::Get<bool>(esp_weapon);

				if (!bNeedsBBox)
				{
					nDrawn++;
					continue;
				}

				BBox bbox{};
				if (!GetBoundingBox(pPawn, bbox))
				{
					if (bDiag)
					{
						Vector3 org = pSceneNode->GetAbsOrigin();
						L_PRINT(LOG_INFO) << _XS("[ESP] ent[") << i << _XS("] bbox fail origin=(")
							<< org.x << _XS(",") << org.y << _XS(",") << org.z << _XS(")");
					}
					continue;
				}

				nDrawn++;

				if (C::Get<bool>(esp_box))
					DrawPlayerBox(bbox, nTeam);

				if (C::Get<bool>(esp_name))
					DrawPlayerName(bbox, pController);

				if (C::Get<bool>(esp_health))
					DrawPlayerHealth(bbox, pPawn);

				if (C::Get<bool>(esp_armor))
					DrawPlayerArmor(bbox, pPawn);

				if (C::Get<bool>(esp_snaplines) && bLocalAlive)
					DrawPlayerSnapline(bbox, nTeam);

				if (C::Get<bool>(esp_weapon))
				{
					__try
					{
						DrawPlayerWeapon(bbox, pPawn);
					}
					__except (EXCEPTION_EXECUTE_HANDLER)
					{
						static bool bLoggedWeaponFault = false;
						if (!bLoggedWeaponFault)
						{
							bLoggedWeaponFault = true;
							L_PRINT(LOG_WARNING) << _XS("[ESP] weapon draw faulted");
						}
					}
				}
			}
			__except (EXCEPTION_EXECUTE_HANDLER)
			{
				if (bDiag)
					L_PRINT(LOG_WARNING) << _XS("[ESP] exception on entity ") << i;
				continue;
			}
		}

		if (bDiag)
			L_PRINT(LOG_INFO) << _XS("[ESP] found=") << nFound << _XS(" drawn=") << nDrawn;
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		static bool bLoggedEx = false;
		if (!bLoggedEx)
		{
			bLoggedEx = true;
			L_PRINT(LOG_ERROR) << _XS("[ESP] EXCEPTION in ESP render");
		}
	}
}
