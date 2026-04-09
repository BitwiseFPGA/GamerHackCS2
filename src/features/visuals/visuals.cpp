#include "visuals.h"

#include "../../core/config.h"
#include "../../core/variables.h"
#include "../../core/interfaces.h"
#include "../../utilities/render.h"
#include "../../utilities/log.h"
#include "../../utilities/xorstr.h"
#include "../../sdk/entity.h"
#include "../../sdk/interfaces/cgameentitysystem.h"
#include "../../sdk/interfaces/iengineclient.h"

#include <imgui.h>
#include "../../sdk/interfaces/iglobalvars.h"
#include "../../sdk/datatypes/color.h"

#include <cmath>
#include <cstdio>

// ---------------------------------------------------------------
// bounding box helper
// ---------------------------------------------------------------
struct BBox
{
	Vector2D min; // top-left
	Vector2D max; // bottom-right
	float width  = 0.0f;
	float height = 0.0f;
};

static bool GetBoundingBox(C_CSPlayerPawn* pPawn, BBox& bbox)
{
	auto* pSceneNode = pPawn->GetGameSceneNode();
	if (!pSceneNode)
		return false;

	Vector3 origin = pSceneNode->GetAbsOrigin();

	// use collision bounds for height, fall back to 72 (standing)
	float flHeight = 72.0f;
	CCollisionProperty* pCollision = pPawn->GetCollision();
	if (pCollision)
		flHeight = pCollision->GetMaxs().z - pCollision->GetMins().z;

	Vector3 headPos = origin + Vector3(0, 0, flHeight);

	Vector2D screenTop, screenBottom;
	if (!D::WorldToScreen(headPos, &screenTop))
		return false;
	if (!D::WorldToScreen(origin, &screenBottom))
		return false;

	bbox.height = screenBottom.y - screenTop.y;
	if (bbox.height < 2.0f)
		return false;

	bbox.width = bbox.height / 2.5f;
	float centerX = (screenTop.x + screenBottom.x) * 0.5f;

	bbox.min = Vector2D(centerX - bbox.width * 0.5f, screenTop.y);
	bbox.max = Vector2D(centerX + bbox.width * 0.5f, screenBottom.y);
	return true;
}

// ---------------------------------------------------------------
// skeleton bone pairs for CS2 player models
// ---------------------------------------------------------------
struct BonePair { int from, to; };
static constexpr BonePair arrBonePairs[] =
{
	{ 6,  5  }, // head -> neck
	{ 5,  4  }, // neck -> spine3 (chest)
	{ 4,  2  }, // spine3 -> spine1
	{ 2,  0  }, // spine1 -> pelvis
	{ 5,  8  }, // neck -> left shoulder
	{ 8,  9  }, // left shoulder -> left elbow
	{ 9,  10 }, // left elbow -> left hand
	{ 5,  13 }, // neck -> right shoulder
	{ 13, 14 }, // right shoulder -> right elbow
	{ 14, 15 }, // right elbow -> right hand
	{ 0,  22 }, // pelvis -> left hip
	{ 22, 23 }, // left hip -> left knee
	{ 23, 24 }, // left knee -> left foot
	{ 0,  25 }, // pelvis -> right hip
	{ 25, 26 }, // right hip -> right knee
	{ 26, 27 }, // right knee -> right foot
};

// ---------------------------------------------------------------
// per-player drawing helpers
// ---------------------------------------------------------------
static void DrawPlayerBox(const BBox& bbox, int nTeam)
{
	const int nBoxType = C::Get<int>(esp_box_type);
	const Color& colBox = (nTeam == TEAM_TT) ? C::Get<Color>(esp_box_color_t) : C::Get<Color>(esp_box_color_ct);

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
	D::DrawText(namePos, Color(255, 255, 255, 230), szName, true);
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
		Vector2D(BAR_WIDTH, flBarH), Color(0, 128, 255, 255));
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
	D::DrawText(weaponPos, Color(220, 220, 220, 210), szDisplay, true);
}

static void DrawPlayerSkeleton(C_CSPlayerPawn* pPawn, int nTeam)
{
	auto* pSceneNode = pPawn->GetGameSceneNode();
	if (!pSceneNode) return;

	auto* pSkeleton = pSceneNode->GetSkeletonInstance();
	if (!pSkeleton) return;

	const Color colBone = (nTeam == TEAM_TT) ? Color(255, 180, 50, 230) : Color(100, 180, 255, 230);

	for (const auto& pair : arrBonePairs)
	{
		Vector3 from = pSkeleton->GetBonePosition(pair.from);
		Vector3 to   = pSkeleton->GetBonePosition(pair.to);

		if (from.IsZero() || to.IsZero())
			continue;

		D::DrawLine3D(from, to, colBone, 1.5f);
	}
}

static void DrawPlayerSnapline(const BBox& bbox)
{
	ImVec2 display = ImGui::GetIO().DisplaySize;
	Vector2D screenBottom(display.x * 0.5f, display.y);
	Vector2D playerFeet(bbox.min.x + bbox.width * 0.5f, bbox.max.y);

	D::DrawLine(screenBottom, playerFeet, Color(255, 255, 255, 120), 1.0f);
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

	if (!I::GameEntitySystem)
		return;

	// get local player controller
	const int nLocalIndex = I::Engine->GetLocalPlayer();
	auto* pLocalController = I::GameEntitySystem->Get<CCSPlayerController>(nLocalIndex);
	if (!pLocalController)
		return;

	auto* pLocalPawn = I::GameEntitySystem->Get<C_CSPlayerPawn>(pLocalController->GetPawnHandle());
	if (!pLocalPawn)
		return;

	const int nLocalTeam = pLocalPawn->GetAssociatedTeam();

	// iterate players (index 1..64)
	for (int i = 1; i <= 64; i++)
	{
		if (i == nLocalIndex)
			continue;

		auto* pController = I::GameEntitySystem->Get<CCSPlayerController>(i);
		if (!pController)
			continue;

		// skip dead players
		if (!pController->IsPawnAlive())
			continue;

		auto* pPawn = I::GameEntitySystem->Get<C_CSPlayerPawn>(pController->GetPawnHandle());
		if (!pPawn || pPawn == pLocalPawn)
			continue;

		// skip dormant
		auto* pSceneNode = pPawn->GetGameSceneNode();
		if (!pSceneNode || pSceneNode->IsDormant())
			continue;

		// skip teammates
		const int nTeam = pPawn->GetAssociatedTeam();
		if (nTeam == nLocalTeam)
			continue;

		if (!pPawn->IsAlive())
			continue;

		// get bounding box
		BBox bbox;
		if (!GetBoundingBox(pPawn, bbox))
			continue;

		// draw features based on config
		if (C::Get<bool>(esp_box))
			DrawPlayerBox(bbox, nTeam);

		if (C::Get<bool>(esp_name))
			DrawPlayerName(bbox, pController);

		if (C::Get<bool>(esp_health))
			DrawPlayerHealth(bbox, pPawn);

		if (C::Get<bool>(esp_armor))
			DrawPlayerArmor(bbox, pPawn);

		if (C::Get<bool>(esp_weapon))
			DrawPlayerWeapon(bbox, pPawn);

		if (C::Get<bool>(esp_skeleton))
			DrawPlayerSkeleton(pPawn, nTeam);

		if (C::Get<bool>(esp_snaplines))
			DrawPlayerSnapline(bbox);
	}
}

// ---------------------------------------------------------------
// OnFrameStageNotify
// ---------------------------------------------------------------
void F::VISUALS::OnFrameStageNotify(int nStage)
{
	// glow and other frame-stage visuals can be applied here
	// glow requires setting glow-related schema fields on the player pawn
	// during FRAME_NET_UPDATE_POSTDATAUPDATE_END
	(void)nStage;
}
