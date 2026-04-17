#pragma once
#include "../sdk/datatypes/vector.h"
#include "../sdk/interfaces/cgametracemanager.h"

class C_CSPlayerPawn;
class C_BaseEntity;
class CCSGOInput;
class CCSPlayerController;
struct QAngle;

namespace TRACE
{
	bool Setup();
	void Destroy();

	// basic visibility check (point to point)
	// pTargetEntity: if provided, returns true if the trace hits this entity (even if fraction < 0.97)
	bool IsVisible(const Vector3& vecStart, const Vector3& vecEnd,
	               C_CSPlayerPawn* pSkipEntity = nullptr, C_BaseEntity* pTargetEntity = nullptr);

	// check if a specific bone on a player is visible from local player's eye
	bool IsBoneVisible(C_CSPlayerPawn* pLocalPawn, C_CSPlayerPawn* pTargetPawn,
	                   int nBoneIndex, Vector3* pOutBonePos = nullptr);

	// check if any bone on target player is visible
	bool IsPlayerVisible(C_CSPlayerPawn* pLocalPawn, CCSPlayerController* pTargetController);

	// check if line goes through smoke
	bool LineGoesThroughSmoke(const Vector3& vecStart, const Vector3& vecEnd);

	// generic trace (returns full trace result)
	bool TraceShape(const Vector3& vecStart, const Vector3& vecEnd,
	                C_CSPlayerPawn* pSkipEntity, GameTrace_t* pOutTrace);

	// entity-specific clip trace used to verify player intersection on a ray
	bool ClipRayToPlayer(const Vector3& vecStart, const Vector3& vecEnd,
	                     C_CSPlayerPawn* pSkipEntity, C_CSPlayerPawn* pTargetPawn,
	                     GameTrace_t* pOutTrace = nullptr);

	// bone names used for multi-point visibility checking
	inline constexpr int VISCHECK_BONE_HEAD   = 6;
	inline constexpr int VISCHECK_BONE_NECK   = 5;
	inline constexpr int VISCHECK_BONE_CHEST  = 4;
	inline constexpr int VISCHECK_BONE_SPINE  = 2;
	inline constexpr int VISCHECK_BONE_PELVIS = 0;

	inline constexpr int arrVisCheckBones[] = {
		6,  // head
		5,  // neck
		4,  // chest (spine3)
		2,  // spine1
		0,  // pelvis
		10, // hand_L
		15, // hand_R
		24, // ankle_L
		27  // ankle_R
	};

	inline constexpr int NUM_VISCHECK_BONES = sizeof(arrVisCheckBones) / sizeof(arrVisCheckBones[0]);
}
