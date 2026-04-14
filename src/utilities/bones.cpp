#include "bones.h"

#include "memory.h"
#include "xorstr.h"
#include "../sdk/entity.h"
#include "../sdk/functionlist.h"

// ---------------------------------------------------------------
// GetBonePosition — get world-space bone position by index
// ---------------------------------------------------------------
bool BONES::GetBonePosition(C_CSPlayerPawn* pPawn, int nBoneIndex, Vector3& outPos)
{
	if (!pPawn)
		return false;

	auto* pSceneNode = pPawn->GetGameSceneNode();
	if (!pSceneNode)
		return false;

	auto* pSkeleton = pSceneNode->GetSkeletonInstance();
	if (!pSkeleton)
		return false;

	// force bone recalculation (Andromeda calls CalcWorldSpaceBones before reading)
	if (SDK_FUNC::CalcWorldSpaceBones)
		SDK_FUNC::CalcWorldSpaceBones(pSkeleton, 0xFFFFF);

	if (!pSceneNode->GetBonePosition(nBoneIndex, outPos))
		return false;

	return !outPos.IsZero();
}

// ---------------------------------------------------------------
// CalcWorldSpaceBones — force bone recalculation
// ---------------------------------------------------------------
void BONES::CalcWorldSpaceBones(CSkeletonInstance* pSkeleton, uint32_t nBoneMask)
{
	if (!pSkeleton)
		return;

	using fnCalcWorldSpaceBones = void(__fastcall*)(CSkeletonInstance*, uint32_t);
	static auto oCalcWorldSpaceBones = reinterpret_cast<fnCalcWorldSpaceBones>(
		MEM::FindPattern(_XS("client.dll"),
			_XS("48 89 5C 24 ? 55 56 57 41 54 41 55 41 56 41 57 48 81 EC ? ? ? ? 4C 8B")));

	if (oCalcWorldSpaceBones)
		oCalcWorldSpaceBones(pSkeleton, nBoneMask);
}
