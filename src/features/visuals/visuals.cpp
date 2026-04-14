#include "visuals.h"

#include "../../core/config.h"
#include "../../core/variables.h"
#include "../../core/interfaces.h"
#include "../../utilities/render.h"
#include "../../utilities/bones.h"
#include "../../utilities/log.h"
#include "../../utilities/xorstr.h"
#include "../../sdk/entity.h"
#include "../../sdk/interfaces/cgameentitysystem.h"
#include "../../sdk/interfaces/iengineclient.h"
#include "../../sdk/functionlist.h"

#include <imgui.h>
#include "../../sdk/interfaces/iglobalvars.h"
#include "../../sdk/datatypes/color.h"

#include <cmath>
#include <cstdio>
#include <cstring>
#include <algorithm>

// ---------------------------------------------------------------
// bounding box helper
// ---------------------------------------------------------------
struct BBox
{
	Vector2D min; // top-left  (screen)
	Vector2D max; // bottom-right (screen)
	float width  = 0.0f;
	float height = 0.0f;
};

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
		Vector2D screen;
		if (!D::WorldToScreen(pt, &screen))
			return false;

		if (screen.x < screenMinX) screenMinX = screen.x;
		if (screen.y < screenMinY) screenMinY = screen.y;
		if (screen.x > screenMaxX) screenMaxX = screen.x;
		if (screen.y > screenMaxY) screenMaxY = screen.y;
	}

	bbox.width = screenMaxX - screenMinX;
	bbox.height = screenMaxY - screenMinY;

	if (bbox.height < 2.0f || bbox.width < 2.0f)
		return false;

	bbox.min = Vector2D(screenMinX, screenMinY);
	bbox.max = Vector2D(screenMaxX, screenMaxY);
	return true;
}

static Color GetPlayerESPColor(int nTeam)
{
	return (nTeam == TEAM_TT) ? C::Get<Color>(esp_box_color_t) : C::Get<Color>(esp_box_color_ct);
}

static Color GetPlayerSkeletonColor(int nTeam)
{
	return (nTeam == TEAM_TT) ? C::Get<Color>(esp_skeleton_color_t) : C::Get<Color>(esp_skeleton_color_ct);
}

static Color GetPlayerSnaplineColor(int nTeam)
{
	return (nTeam == TEAM_TT) ? C::Get<Color>(esp_snapline_color_t) : C::Get<Color>(esp_snapline_color_ct);
}

// ---------------------------------------------------------------
// per-player drawing helpers
// ---------------------------------------------------------------
static void DrawPlayerBox(const BBox& bbox, int nTeam)
{
	const int nBoxType = C::Get<int>(esp_box_type);
	const Color colBox = GetPlayerESPColor(nTeam);

	if (nBoxType == 1) // corner box
		D::DrawCornerBox(bbox.min, Vector2D(bbox.width, bbox.height), colBox, 1.5f, 0.25f);
	else // normal box
		D::DrawRect(bbox.min, Vector2D(bbox.width, bbox.height), colBox, 0.0f, 1.5f);
}

static void DrawPlayerName(const BBox& bbox, CCSPlayerController* pController)
{
	if (!pController)
		return;

	const char* szName = pController->GetPlayerName();
	if (!szName || szName[0] == '\0')
		return;

	Vector2D namePos(bbox.min.x + bbox.width * 0.5f, bbox.min.y - 14.0f);
	D::DrawText(namePos, C::Get<Color>(esp_name_color), szName, true);
}

static void DrawPlayerHealth(const BBox& bbox, C_CSPlayerPawn* pPawn)
{
	int nHealth = pPawn->GetHealth();
	int nMaxHealth = pPawn->GetMaxHealth();
	if (nMaxHealth <= 0) nMaxHealth = 100;

	D::DrawHealthBar(bbox.min, bbox.height, nHealth, nMaxHealth);
}

static void DrawPlayerArmor(const BBox& bbox, C_CSPlayerPawn* pPawn)
{
	int nArmor = pPawn->GetArmorValue();
	if (nArmor <= 0) return;

	constexpr float BAR_WIDTH = 4.0f;
	const float flFrac = static_cast<float>(nArmor) / 100.0f;
	const float flBarH = bbox.height * flFrac;
	const float flOff  = bbox.height - flBarH;
	const float flX    = bbox.max.x + 2.0f;

	D::DrawRectFilled(Vector2D(flX, bbox.min.y - 1.0f),
		Vector2D(BAR_WIDTH + 2.0f, bbox.height + 2.0f), Color(0, 0, 0, 180));
	D::DrawRectFilled(Vector2D(flX + 1.0f, bbox.min.y + flOff),
		Vector2D(BAR_WIDTH, flBarH), C::Get<Color>(esp_armor_color));
}

static void DrawPlayerWeapon(const BBox& bbox, C_CSPlayerPawn* pPawn)
{
	auto* pWeaponServices = pPawn->GetWeaponServices();
	if (!pWeaponServices) return;

	auto hActive = pWeaponServices->GetActiveWeapon();
	if (!hActive.IsValid()) return;

	auto* pWeapon = I::GameEntitySystem->Get<C_BaseEntity>(hActive);
	if (!pWeapon) return;

	auto* pId = pWeapon->GetIdentity();
	if (!pId) return;

	const char* szDesigner = pId->GetDesignerName();
	if (!szDesigner) return;

	// strip "weapon_" prefix
	const char* szDisplay = szDesigner;
	if (std::strncmp(szDisplay, "weapon_", 7) == 0)
		szDisplay += 7;

	Vector2D weaponPos(bbox.min.x + bbox.width * 0.5f, bbox.max.y + 2.0f);
	D::DrawText(weaponPos, C::Get<Color>(esp_weapon_color), szDisplay, true);
}

static void DrawPlayerSkeleton(C_CSPlayerPawn* pPawn, int nTeam)
{
	auto* pSceneNode = pPawn->GetGameSceneNode();
	if (!pSceneNode) return;

	auto* pSkeleton = pSceneNode->GetSkeletonInstance();
	if (!pSkeleton) return;

	// Force bone update once before reading any positions
	BONES::CalcWorldSpaceBones(pSkeleton, 0xFFFFF);

	const Color colBone = GetPlayerSkeletonColor(nTeam);
	Matrix2x4* pBoneCache = pSkeleton->GetBoneCache();
	if (!pBoneCache)
		return;
	const int nBoneCount = pSkeleton->GetBoneCount();

	auto GetBonePos = [&](int nBoneIndex, Vector3& outPos) -> bool
	{
		if (nBoneIndex < 0 || nBoneIndex >= nBoneCount)
			return false;

		outPos = pBoneCache->GetOrigin(nBoneIndex);
		if (!outPos.IsZero())
			return true;

		if (pSceneNode->GetBonePosition(nBoneIndex, outPos) && !outPos.IsZero())
			return true;

		return BONES::GetBonePosition(pPawn, nBoneIndex, outPos);
	};

	for (const auto& pair : BONES::arrSkeletonPairs)
	{
		Vector3 from{}, to{};
		if (!GetBonePos(pair.nParent, from) || !GetBonePos(pair.nChild, to))
			continue;

		Vector2D fromScreen, toScreen;
		if (D::WorldToScreen(from, &fromScreen) && D::WorldToScreen(to, &toScreen))
			D::DrawLine(fromScreen, toScreen, colBone, 1.5f);
	}

	Vector3 headPos{}, neckPos{};
	Vector2D headScreen;
	if (GetBonePos(6, headPos) && D::WorldToScreen(headPos, &headScreen))
	{
		float flRadius = 5.0f;
		if (GetBonePos(5, neckPos))
		{
			Vector2D neckScreen;
			if (D::WorldToScreen(neckPos, &neckScreen))
				flRadius = std::max(3.0f, std::fabs(headScreen.y - neckScreen.y) * 0.75f);
		}
		else
		{
			Vector3 headTop = headPos;
			headTop.z += 7.0f;
			Vector2D headTopScreen;
			if (D::WorldToScreen(headTop, &headTopScreen))
				flRadius = std::max(3.0f, std::fabs(headScreen.y - headTopScreen.y));
		}

		D::DrawCircle(headScreen, flRadius, colBone, 16);
	}
}

static void DrawPlayerSnapline(const BBox& bbox, int nTeam)
{
	ImVec2 display = ImGui::GetIO().DisplaySize;
	Vector2D screenBottom(display.x * 0.5f, display.y);
	Vector2D playerFeet(bbox.min.x + bbox.width * 0.5f, bbox.max.y);

	D::DrawLine(screenBottom, playerFeet, GetPlayerSnaplineColor(nTeam), 1.0f);
}

// ---------------------------------------------------------------
// setup / destroy
// ---------------------------------------------------------------
bool F::VISUALS::Setup()
{
	L_PRINT(LOG_INFO) << _XS("[VISUALS] initialized");
	return true;
}

void F::VISUALS::Destroy()
{
	L_PRINT(LOG_INFO) << _XS("[VISUALS] destroyed");
}

// ---------------------------------------------------------------
// OnPresent — called each frame from the render thread
// ---------------------------------------------------------------
void F::VISUALS::OnPresent()
{
	if (!C::Get<bool>(esp_enabled))
		return;

	if (!I::Engine || !I::Engine->IsInGame())
		return;

	static bool bLoggedInGame = false;
	if (!bLoggedInGame)
	{
		bLoggedInGame = true;
		L_PRINT(LOG_INFO) << _XS("[VISUALS] IsInGame true — first OnPresent in-game");
	}

	if (!I::GameEntitySystem)
	{
		static bool bLoggedNoES = false;
		if (!bLoggedNoES) { bLoggedNoES = true; L_PRINT(LOG_ERROR) << _XS("[VISUALS] GameEntitySystem is null"); }
		return;
	}

	// periodic diagnostic logging (first 3 frames, then every 300)
	static int nFrame = 0;
	nFrame++;
	const bool bDiag = (nFrame <= 3) || (nFrame % 300 == 0);

	__try
	{
		// Use SDK_FUNC::GetLocalPlayerController for reliable local player resolution
		CCSPlayerController* pLocalController = nullptr;
		if (SDK_FUNC::GetLocalPlayerController)
		{
			pLocalController = SDK_FUNC::GetLocalPlayerController(-1);
		}

		if (!pLocalController)
			return;

		// ESP should work even when dead (spectating) — only skip snaplines when dead
		const bool bLocalAlive = pLocalController->IsPawnAlive();

		// resolve local pawn (may be null when dead)
		C_CSPlayerPawn* pLocalPawn = nullptr;
		CBaseHandle hPawn = pLocalController->GetPlayerPawnHandle();
		if (hPawn.IsValid())
			pLocalPawn = I::GameEntitySystem->Get<C_CSPlayerPawn>(hPawn);

		// get local team — from pawn if available, otherwise from controller
		int nLocalTeam = 0;
		if (pLocalPawn)
			nLocalTeam = static_cast<int>(pLocalPawn->GetTeam());
		else
			nLocalTeam = static_cast<int>(pLocalController->GetTeam());

		if (bDiag && nFrame <= 3)
			L_PRINT(LOG_INFO) << _XS("[ESP] localPawn=") << static_cast<void*>(pLocalPawn)
			<< _XS(" alive=") << bLocalAlive
			<< _XS(" team=") << nLocalTeam;

		int nFound = 0, nDrawn = 0;

		for (int i = 1; i <= 64; i++)
		{
			__try
			{
				auto* pController = I::GameEntitySystem->Get<CCSPlayerController>(i);
				if (!pController)
					continue;

				nFound++;

				if (!pController->IsPawnAlive())
				{
					if (bDiag && nFrame <= 3)
						L_PRINT(LOG_INFO) << _XS("[ESP] ent[") << i << _XS("] dead");
					continue;
				}

				// skip if this IS the local player controller
				if (pController == pLocalController)
					continue;

				CBaseHandle hEntPawn = pController->GetPlayerPawnHandle();
				if (!hEntPawn.IsValid())
				{
					if (bDiag && nFrame <= 3)
						L_PRINT(LOG_INFO) << _XS("[ESP] ent[") << i << _XS("] bad handle raw=") << hEntPawn.GetRawIndex();
					continue;
				}

				auto* pPawn = I::GameEntitySystem->Get<C_CSPlayerPawn>(hEntPawn);
				if (!pPawn || pPawn == pLocalPawn)
				{
					if (bDiag && nFrame <= 3)
						L_PRINT(LOG_INFO) << _XS("[ESP] ent[") << i << _XS("] pawn null/self idx=") << hEntPawn.GetEntryIndex();
					continue;
				}

				auto* pSceneNode = pPawn->GetGameSceneNode();
				if (!pSceneNode)
				{
					if (bDiag)
						L_PRINT(LOG_INFO) << _XS("[ESP] ent[") << i << _XS("] no scene node");
					continue;
				}

				// NOTE: dormant check removed — reference code (Andromeda) does not filter
				// dormant entities for ESP. Entities go dormant after initial network sync
				// but should still be drawn.

				const int nTeam = static_cast<int>(pPawn->GetTeam());
				const bool bIsTeammate = (nTeam == nLocalTeam);
				if (bIsTeammate && !C::Get<bool>(esp_team))
				{
					if (bDiag && nFrame <= 3)
						L_PRINT(LOG_INFO) << _XS("[ESP] ent[") << i << _XS("] teammate filtered team=") << nTeam;
					continue;
				}

				if (!pPawn->IsAlive())
				{
					if (bDiag)
						L_PRINT(LOG_INFO) << _XS("[ESP] ent[") << i << _XS("] pawn dead");
					continue;
				}

				if (C::Get<bool>(esp_skeleton))
				{
					__try
					{
						DrawPlayerSkeleton(pPawn, nTeam);
					}
					__except (EXCEPTION_EXECUTE_HANDLER)
					{
						static bool bLoggedSkeletonFault = false;
						if (!bLoggedSkeletonFault)
						{
							bLoggedSkeletonFault = true;
							L_PRINT(LOG_WARNING) << _XS("[ESP] skeleton draw faulted");
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

				BBox bbox;
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
				// per-entity exception — skip this entity, continue others
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
			L_PRINT(LOG_ERROR) << _XS("[ESP] EXCEPTION in OnPresent — schema offsets likely wrong");
		}
	}
}

// ---------------------------------------------------------------
// OnFrameStageNotify
// ---------------------------------------------------------------
void F::VISUALS::OnFrameStageNotify(int nStage)
{
	// glow and other frame-stage visuals can be applied here
	(void)nStage;
}

// ---------------------------------------------------------------
// Sniper Crosshair — draw crosshair when holding unscoped sniper
// ---------------------------------------------------------------
static bool IsSniperWeapon(const char* szDesignerName)
{
	if (!szDesignerName)
		return false;

	return (std::strcmp(szDesignerName, "weapon_awp") == 0 ||
		std::strcmp(szDesignerName, "weapon_ssg08") == 0 ||
		std::strcmp(szDesignerName, "weapon_scar20") == 0 ||
		std::strcmp(szDesignerName, "weapon_g3sg1") == 0);
}

void F::VISUALS::DrawSniperCrosshair()
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
		if (!pLocalPawn || !pLocalPawn->IsAlive())
			return;

		// if player is scoped, game already draws crosshair
		if (pLocalPawn->IsScoped())
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

		const char* szDesigner = pId->GetDesignerName();
		if (!IsSniperWeapon(szDesigner))
			return;

		// draw crosshair at screen center
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
		// silently ignore
	}
}

// ---------------------------------------------------------------
// Custom Radar — draw a minimap radar with enemy positions
// ---------------------------------------------------------------
void F::VISUALS::DrawRadar()
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

		// local view angles for yaw rotation
		QAngle angLocal{};
		if (SDK_FUNC::GetViewAngles && I::Input)
		{
			QAngle* pAng = SDK_FUNC::GetViewAngles(I::Input, 0);
			if (pAng) angLocal = *pAng;
		}

		const float flRadarSize = C::Get<float>(misc_radar_size);
		const float flRadarRange = C::Get<float>(misc_radar_range);
		const float flRadius = flRadarSize * 0.5f;

		// radar position: top-left corner with margin
		constexpr float margin = 16.0f;
		const float cx = margin + flRadius;
		const float cy = margin + flRadius;

		// yaw in radians (rotate so forward = up on radar)
		const float flYawRad = angLocal.y * (3.14159265f / 180.0f);
		const float cosY = std::cosf(flYawRad);
		const float sinY = std::sinf(flYawRad);

		// draw radar background
		D::DrawCircleFilled(Vector2D(cx, cy), flRadius, Color(0, 0, 0, 150), 48);
		D::DrawCircle(Vector2D(cx, cy), flRadius, Color(100, 100, 100, 180), 48, 1.0f);

		// draw crosshair lines on radar
		D::DrawLine(Vector2D(cx - flRadius, cy), Vector2D(cx + flRadius, cy), Color(60, 60, 60, 120), 1.0f);
		D::DrawLine(Vector2D(cx, cy - flRadius), Vector2D(cx, cy + flRadius), Color(60, 60, 60, 120), 1.0f);

		// draw local player as white dot at center
		D::DrawCircleFilled(Vector2D(cx, cy), 3.0f, Color(255, 255, 255, 230), 8);

		const int nLocalTeam = static_cast<int>(pLocalPawn->GetTeam());

		for (int i = 1; i <= 64; i++)
		{
			__try
			{
				auto* pController = I::GameEntitySystem->Get<CCSPlayerController>(i);
				if (!pController || !pController->IsPawnAlive())
					continue;
				if (pController == pLocalController)
					continue;

				auto* pPawn = I::GameEntitySystem->Get<C_CSPlayerPawn>(pController->GetPlayerPawnHandle());
				if (!pPawn || pPawn == pLocalPawn || !pPawn->IsAlive())
					continue;

				auto* pSceneNode = pPawn->GetGameSceneNode();
				if (!pSceneNode)
					continue;

				// dormant enemies shown as faded dots
				const bool bDormant = pSceneNode->IsDormant();

				const int nTeam = static_cast<int>(pPawn->GetTeam());
				const bool bEnemy = (nTeam != nLocalTeam);

				Vector3 vecEnemyOrigin = pSceneNode->GetAbsOrigin();
				float dx = vecEnemyOrigin.x - vecLocalOrigin.x;
				float dy = vecEnemyOrigin.y - vecLocalOrigin.y;

				// rotate by negative yaw so forward = up
				float rx = dx * cosY + dy * sinY;
				float ry = -dx * sinY + dy * cosY;

				// scale to radar
				float scale = flRadius / flRadarRange;
				float px = cx + rx * scale;
				float py = cy - ry * scale; // screen Y is inverted

				// clamp to radar circle
				float distFromCenter = std::sqrtf((px - cx) * (px - cx) + (py - cy) * (py - cy));
				if (distFromCenter > flRadius - 3.0f)
				{
					float clampScale = (flRadius - 3.0f) / distFromCenter;
					px = cx + (px - cx) * clampScale;
					py = cy + (py - cy) * clampScale;
				}

				Color dotColor;
				if (bEnemy)
					dotColor = bDormant ? Color(255, 80, 80, 100) : Color(255, 40, 40, 230);
				else
					dotColor = bDormant ? Color(80, 80, 255, 100) : Color(80, 80, 255, 200);

				D::DrawCircleFilled(Vector2D(px, py), 3.0f, dotColor, 8);
			}
			__except (EXCEPTION_EXECUTE_HANDLER)
			{
				continue;
			}
		}

		// "RADAR" label
		D::DrawText(Vector2D(cx, cy + flRadius + 2.0f), Color(180, 180, 180, 160), "RADAR", true);
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		// silently ignore
	}
}
