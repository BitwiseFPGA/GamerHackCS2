#include "esp.h"

#include "../../core/config.h"
#include "../../core/interfaces.h"
#include "../../core/variables.h"
#include "../../sdk/datatypes/color.h"
#include "../../sdk/entity.h"
#include "../../sdk/functionlist.h"
#include "../../sdk/interfaces/cgameentitysystem.h"
#include "../../sdk/interfaces/iengineclient.h"
#include "../../utilities/bones.h"
#include "../../utilities/log.h"
#include "../../utilities/render.h"
#include "../../utilities/xorstr.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <imgui.h>
#include <limits>
#include <string>

namespace
{
	struct BBox
	{
		Vector2D min;
		Vector2D max;
		float width = 0.0f;
		float height = 0.0f;
	};

	struct BoneConnection
	{
		const char* szFrom;
		const char* szTo;
	};

	inline constexpr BoneConnection arrSkeletonConnections[] = {
		{ "head_0", "neck_0" },
		{ "neck_0", "spine_1" },
		{ "spine_1", "spine_2" },
		{ "spine_2", "spine_3" },
		{ "spine_3", "pelvis" },
		{ "neck_0", "clavicle_l" },
		{ "clavicle_l", "arm_upper_l" },
		{ "arm_upper_l", "arm_lower_l" },
		{ "arm_lower_l", "hand_l" },
		{ "neck_0", "clavicle_r" },
		{ "clavicle_r", "arm_upper_r" },
		{ "arm_upper_r", "arm_lower_r" },
		{ "arm_lower_r", "hand_r" },
		{ "pelvis", "leg_upper_l" },
		{ "leg_upper_l", "leg_lower_l" },
		{ "leg_lower_l", "ankle_l" },
		{ "pelvis", "leg_upper_r" },
		{ "leg_upper_r", "leg_lower_r" },
		{ "leg_lower_r", "ankle_r" }
	};

	static Color GetPlayerESPColor(int nTeam)
	{
		return (nTeam == TEAM_TT) ? C::Get<Color>(esp_box_color_t) : C::Get<Color>(esp_box_color_ct);
	}

	static Color GetPlayerSnaplineColor(int nTeam)
	{
		return (nTeam == TEAM_TT) ? C::Get<Color>(esp_snapline_color_t) : C::Get<Color>(esp_snapline_color_ct);
	}

	static const char* TrimWeaponClassName(const char* className)
	{
		if (!className || !className[0])
			return className;
		if (std::strncmp(className, "weapon_", 7) == 0)
			return className + 7;
		if (std::strncmp(className, "item_", 5) == 0)
			return className + 5;
		return className;
	}

	static const char* GetWeaponPrettyName(const char* className)
	{
		const char* w = TrimWeaponClassName(className);
		if (!w || !w[0])
			return "Unknown";

		if (!std::strcmp(w, "ak47")) return "AK-47";
		if (!std::strcmp(w, "m4a1")) return "M4A4";
		if (!std::strcmp(w, "m4a1_silencer")) return "M4A1-S";
		if (!std::strcmp(w, "awp")) return "AWP";
		if (!std::strcmp(w, "ssg08")) return "SSG 08";
		if (!std::strcmp(w, "scar20")) return "SCAR-20";
		if (!std::strcmp(w, "g3sg1")) return "G3SG1";
		if (!std::strcmp(w, "aug")) return "AUG";
		if (!std::strcmp(w, "sg556")) return "SG 553";
		if (!std::strcmp(w, "famas")) return "FAMAS";
		if (!std::strcmp(w, "galilar")) return "Galil AR";
		if (!std::strcmp(w, "glock")) return "Glock-18";
		if (!std::strcmp(w, "hkp2000")) return "P2000";
		if (!std::strcmp(w, "usp_silencer")) return "USP-S";
		if (!std::strcmp(w, "deagle")) return "Desert Eagle";
		if (!std::strcmp(w, "revolver")) return "R8 Revolver";
		if (!std::strcmp(w, "elite")) return "Dual Berettas";
		if (!std::strcmp(w, "p250")) return "P250";
		if (!std::strcmp(w, "fiveseven")) return "Five-SeveN";
		if (!std::strcmp(w, "tec9")) return "Tec-9";
		if (!std::strcmp(w, "cz75a")) return "CZ75-Auto";
		if (!std::strcmp(w, "mac10")) return "MAC-10";
		if (!std::strcmp(w, "mp9")) return "MP9";
		if (!std::strcmp(w, "mp7")) return "MP7";
		if (!std::strcmp(w, "mp5sd")) return "MP5-SD";
		if (!std::strcmp(w, "ump45")) return "UMP-45";
		if (!std::strcmp(w, "p90")) return "P90";
		if (!std::strcmp(w, "bizon")) return "PP-Bizon";
		if (!std::strcmp(w, "nova")) return "Nova";
		if (!std::strcmp(w, "xm1014")) return "XM1014";
		if (!std::strcmp(w, "mag7")) return "MAG-7";
		if (!std::strcmp(w, "sawedoff")) return "Sawed-Off";
		if (!std::strcmp(w, "m249")) return "M249";
		if (!std::strcmp(w, "negev")) return "Negev";
		if (!std::strcmp(w, "knife") || std::strstr(w, "knife")) return "Knife";
		if (!std::strcmp(w, "hegrenade")) return "HE Grenade";
		if (!std::strcmp(w, "flashbang")) return "Flashbang";
		if (!std::strcmp(w, "smokegrenade")) return "Smoke";
		if (!std::strcmp(w, "molotov")) return "Molotov";
		if (!std::strcmp(w, "incgrenade")) return "Incendiary";
		if (!std::strcmp(w, "decoy")) return "Decoy";
		if (!std::strcmp(w, "taser")) return "Zeus x27";
		if (!std::strcmp(w, "c4")) return "C4";
		if (!std::strcmp(w, "healthshot")) return "Healthshot";
		if (!std::strcmp(w, "defuser")) return "Defuser";

		return w;
	}

	static const char* GetWeaponViewTag(const char* className)
	{
		if (!className || !className[0])
			return "WEAPON";

		if (std::strncmp(className, "item_", 5) == 0)
			return "ITEM";

		const char* w = TrimWeaponClassName(className);
		if (!w || !w[0])
			return "WEAPON";

		if (std::strstr(w, "knife")) return "KNIFE";
		if (std::strstr(w, "grenade") || !std::strcmp(w, "flashbang") || !std::strcmp(w, "decoy") || !std::strcmp(w, "molotov") || !std::strcmp(w, "incgrenade")) return "GRENADE";
		if (!std::strcmp(w, "awp") || !std::strcmp(w, "ssg08") || !std::strcmp(w, "scar20") || !std::strcmp(w, "g3sg1")) return "SNIPER";
		if (!std::strcmp(w, "deagle") || !std::strcmp(w, "usp_silencer") || !std::strcmp(w, "hkp2000") || !std::strcmp(w, "glock") || !std::strcmp(w, "p250") || !std::strcmp(w, "fiveseven") || !std::strcmp(w, "elite") || !std::strcmp(w, "cz75a") || !std::strcmp(w, "tec9") || !std::strcmp(w, "revolver")) return "PISTOL";
		if (!std::strcmp(w, "mac10") || !std::strcmp(w, "mp9") || !std::strcmp(w, "mp7") || !std::strcmp(w, "mp5sd") || !std::strcmp(w, "ump45") || !std::strcmp(w, "p90") || !std::strcmp(w, "bizon")) return "SMG";
		if (!std::strcmp(w, "nova") || !std::strcmp(w, "xm1014") || !std::strcmp(w, "mag7") || !std::strcmp(w, "sawedoff") || !std::strcmp(w, "m249") || !std::strcmp(w, "negev")) return "HEAVY";
		if (!std::strcmp(w, "c4")) return "C4";
		if (!std::strcmp(w, "healthshot") || !std::strcmp(w, "defuser")) return "ITEM";
		return "RIFLE";
	}

	static std::string BuildWeaponLabel(const char* className)
	{
		const char* szPretty = GetWeaponPrettyName(className);
		return std::string(szPretty ? szPretty : "Unknown");
	}

	static bool GetBoundingBox(C_BaseEntity* pEntity, BBox& bbox)
	{
		if (!pEntity)
			return false;

		auto* pSceneNode = pEntity->GetGameSceneNode();
		auto* pCollision = pEntity->GetCollision();
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

		const std::string label = BuildWeaponLabel(szDesigner);
		D::DrawText(Vector2D(bbox.min.x + bbox.width * 0.5f, bbox.max.y + 2.0f), C::Get<Color>(esp_weapon_color), label.c_str(), true);
	}

	static void DrawPlayerSnapline(const BBox& bbox, int nTeam)
	{
		const ImVec2 display = ImGui::GetIO().DisplaySize;
		D::DrawLine(Vector2D(display.x * 0.5f, display.y),
			Vector2D(bbox.min.x + bbox.width * 0.5f, bbox.max.y),
			GetPlayerSnaplineColor(nTeam), 1.0f);
	}

	static bool GetBoneScreenPosition(C_CSPlayerPawn* pPawn, const char* szBoneName, Vector2D& vecScreen)
	{
		const Vector3 vecBone = BONES::GetBonePositionByName(pPawn, szBoneName);
		return !vecBone.IsZero() && D::WorldToScreen(vecBone, &vecScreen);
	}

	static void DrawSkeleton(C_CSPlayerPawn* pPawn, int nTeam)
	{
		if (!pPawn)
			return;

		Color col{ 255, 255, 255, 255 };

		if (nTeam == TEAM_TT)
			col = C::Get<Color>(esp_skeleton_color_t);
		else if (nTeam == TEAM_CT)
			col = C::Get<Color>(esp_skeleton_color_ct);

		for (const auto& connection : arrSkeletonConnections)
		{
			Vector2D vecFromScreen{};
			Vector2D vecToScreen{};
			if (!GetBoneScreenPosition(pPawn, connection.szFrom, vecFromScreen) ||
				!GetBoneScreenPosition(pPawn, connection.szTo, vecToScreen))
				continue;

			D::DrawLine(vecFromScreen, vecToScreen, col, 1.5f);
		}

		Vector2D vecHeadScreen{};
		Vector2D vecNeckScreen{};
		if (GetBoneScreenPosition(pPawn, "head_0", vecHeadScreen) &&
			GetBoneScreenPosition(pPawn, "neck_0", vecNeckScreen))
		{
			const float flDx = vecHeadScreen.x - vecNeckScreen.x;
			const float flDy = vecHeadScreen.y - vecNeckScreen.y;
			const float flRadius = std::max(3.0f, std::sqrt(flDx * flDx + flDy * flDy) * 0.75f);
			D::DrawCircle(vecHeadScreen, flRadius, col, 24, 1.5f);
		}
	}

	static bool GetEntityLabelAnchor(C_BaseEntity* pEntity, Vector2D& vecAnchor)
	{
		BBox bbox{};
		if (GetBoundingBox(pEntity, bbox))
		{
			vecAnchor = Vector2D(bbox.min.x + bbox.width * 0.5f, bbox.min.y - 14.0f);
			return true;
		}

		auto* pSceneNode = pEntity->GetGameSceneNode();
		if (!pSceneNode)
			return false;

		Vector3 vecOrigin = pSceneNode->GetAbsOrigin();
		vecOrigin.z += 8.0f;
		return D::WorldToScreen(vecOrigin, &vecAnchor);
	}

	static bool IsDroppedItemEntity(C_BaseEntity* pEntity, const char*& szDesignerName)
	{
		szDesignerName = nullptr;

		if (!pEntity || pEntity->IsBasePlayerController() || pEntity->IsPlayerPawn() || pEntity->IsObserverPawn() || pEntity->IsCS2HudModelWeapon() || pEntity->IsGrenadeProjectile())
			return false;

		auto* pIdentity = pEntity->GetIdentity();
		if (!pIdentity)
			return false;

		szDesignerName = pIdentity->GetDesignerName();
		if (!szDesignerName || !szDesignerName[0])
			return false;

		const bool bLooksLikeLoot =
			std::strncmp(szDesignerName, "weapon_", 7) == 0 ||
			std::strncmp(szDesignerName, "item_", 5) == 0 ||
			pEntity->IsBasePlayerWeapon() ||
			pEntity->IsC4();

		if (!bLooksLikeLoot)
			return false;

		// if entity has any owner at all, it's equipped, not dropped
		const CBaseHandle hOwner = pEntity->GetOwnerHandle();
		if (hOwner.IsValid())
		{
			auto* pOwner = I::GameEntitySystem->Get<C_BaseEntity>(hOwner);
			if (pOwner)
				return false;
		}

		// double-check: scan all alive players' weapon services
		for (int p = 1; p <= 64; ++p)
		{
			__try
			{
				auto* pCtrl = I::GameEntitySystem->Get<CCSPlayerController>(p);
				if (!pCtrl || !pCtrl->IsPawnAlive())
					continue;

				auto* pPawn = I::GameEntitySystem->Get<C_CSPlayerPawn>(pCtrl->GetPlayerPawnHandle());
				if (!pPawn)
					continue;

				auto* pWeaponSvc = pPawn->GetWeaponServices();
				if (!pWeaponSvc)
					continue;

				CBaseHandle hEntity = pEntity->GetRefEHandle();

				if (pWeaponSvc->GetActiveWeapon().IsValid() &&
					pWeaponSvc->GetActiveWeapon().GetRawIndex() == hEntity.GetRawIndex())
					return false;

				auto& weapons = pWeaponSvc->GetMyWeapons();
				for (int w = 0; w < weapons.Count(); ++w)
				{
					if (weapons[w].IsValid() && weapons[w].GetRawIndex() == hEntity.GetRawIndex())
						return false;
				}
			}
			__except (EXCEPTION_EXECUTE_HANDLER)
			{
				continue;
			}
		}

		return true;
	}

	static void DrawDroppedItem(C_BaseEntity* pEntity, const char* szDesignerName)
	{
		Vector2D vecAnchor{};
		if (!GetEntityLabelAnchor(pEntity, vecAnchor))
			return;

		const std::string label = BuildWeaponLabel(szDesignerName);
		D::DrawText(vecAnchor, C::Get<Color>(esp_dropped_item_color), label.c_str(), true);
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
	const bool bSkeletonEnabled = C::Get<bool>(esp_skeleton);

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

				if (bSkeletonEnabled)
				{
					__try
					{
						DrawSkeleton(pPawn, nTeam);
					}
					__except (EXCEPTION_EXECUTE_HANDLER)
					{
						static int nSkeletonFaultLogs = 0;
						if (nSkeletonFaultLogs < 12)
						{
							++nSkeletonFaultLogs;
							L_PRINT(LOG_WARNING) << _XS("[ESP] skeleton fault on entity ") << i;
						}
					}
				}

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
					continue;

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

		if (C::Get<bool>(esp_dropped_items))
		{
			const int nHighestEntityIndex = std::clamp(I::GameEntitySystem->GetHighestEntityIndex(), 0, MAX_TOTAL_ENTITIES - 1);
			for (int i = 1; i <= nHighestEntityIndex; ++i)
			{
				__try
				{
					auto* pEntity = I::GameEntitySystem->Get<C_BaseEntity>(i);
					if (!pEntity)
						continue;

					const char* szDesignerName = nullptr;
					if (!IsDroppedItemEntity(pEntity, szDesignerName))
						continue;

					DrawDroppedItem(pEntity, szDesignerName);
				}
				__except (EXCEPTION_EXECUTE_HANDLER)
				{
					if (bDiag)
						L_PRINT(LOG_WARNING) << _XS("[ESP] dropped-item exception on entity ") << i;
				}
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
