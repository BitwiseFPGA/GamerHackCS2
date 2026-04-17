#include "bones.h"

#include "memory.h"
#include "log.h"
#include "xorstr.h"
#include "../sdk/entity.h"
#include "../sdk/functionlist.h"

namespace
{
	int FindBoneIndexByName(CSkeletonInstance* pSkeleton, const char* szBoneName)
	{
		if (!pSkeleton || !szBoneName || szBoneName[0] == '\0')
			return -1;

		CModel* pModel = static_cast<CModel*>(pSkeleton->GetModelState().GetModel());
		if (!pModel || !pModel->m_szBoneNames)
			return -1;

		for (std::uint32_t i = 0; i < pModel->m_nBoneCount; ++i)
		{
			const char* szCurrentBoneName = pModel->m_szBoneNames[i];
			if (szCurrentBoneName && std::strcmp(szCurrentBoneName, szBoneName) == 0)
				return static_cast<int>(i);
		}

		return -1;
	}
}

// ---------------------------------------------------------------
// GetBonePosition — get world-space bone position by index
// ---------------------------------------------------------------
bool BONES::GetBonePosition(C_CSPlayerPawn* pPawn, int nBoneIndex, Vector3& outPos)
{
	outPos = {};

	if (!pPawn || nBoneIndex == -1)
		return false;

	auto* pSceneNode = pPawn->GetGameSceneNode();
	if (!pSceneNode)
		return false;

	auto* pSkeleton = pSceneNode->GetSkeletonInstance();
	if (!pSkeleton)
		return false;

	pSkeleton->CalcWorldSpaceBones(FLAG_ALL_BONE_FLAGS);

	const auto& hModel = pSkeleton->GetModelState().GetModel();
	if (!hModel.IsValid())
		return false;

	if (!pSkeleton->GetBonePosition(nBoneIndex, outPos))
		return false;

	return !outPos.IsZero();
}

Vector3 BONES::GetBonePositionByName(C_CSPlayerPawn* pPawn, const char* szBoneName)
{
	Vector3 vecBonePosition{};
	static int nBoneDiagCount = 0;

	if (!pPawn || !szBoneName || szBoneName[0] == '\0')
		return vecBonePosition;

	auto* pSceneNode = pPawn->GetGameSceneNode();
	if (!pSceneNode)
		return vecBonePosition;

	if (auto* pSkeleton = pSceneNode->GetSkeletonInstance(); pSkeleton)
	{
		pSkeleton->CalcWorldSpaceBones(FLAG_ALL_BONE_FLAGS);

		const auto& hModel = pSkeleton->GetModelState().GetModel();
		if (!hModel.IsValid())
		{
			if (nBoneDiagCount < 20)
			{
				++nBoneDiagCount;
				L_PRINT(LOG_WARNING) << _XS("[BONES] invalid model for ") << szBoneName
					<< _XS(" pawn=") << static_cast<void*>(pPawn)
					<< _XS(" sceneNode=") << static_cast<void*>(pSceneNode)
					<< _XS(" skeleton=") << static_cast<void*>(pSkeleton);
			}
			return vecBonePosition;
		}

		const int nBoneIndex = FindBoneIndexByName(pSkeleton, szBoneName);
		const bool bGotPosition = (nBoneIndex != -1 && pSkeleton->GetBonePosition(nBoneIndex, vecBonePosition));
		if (bGotPosition)
			return vecBonePosition;

		if (nBoneDiagCount < 20)
		{
			++nBoneDiagCount;
			L_PRINT(LOG_WARNING) << _XS("[BONES] lookup failed name=") << szBoneName
				<< _XS(" idx=") << nBoneIndex
				<< _XS(" pawn=") << static_cast<void*>(pPawn)
				<< _XS(" skeleton=") << static_cast<void*>(pSkeleton)
				<< _XS(" gotPos=") << bGotPosition
				<< _XS(" pos=(") << vecBonePosition.x << _XS(",") << vecBonePosition.y << _XS(",") << vecBonePosition.z << _XS(")");
		}
	}

	return vecBonePosition;
}

// ---------------------------------------------------------------
// CalcWorldSpaceBones — force bone recalculation
// ---------------------------------------------------------------
void BONES::CalcWorldSpaceBones(CSkeletonInstance* pSkeleton, uint32_t nBoneMask)
{
	if (!pSkeleton)
		return;

	if (SDK_FUNC::CalcWorldSpaceBones)
	{
		pSkeleton->CalcWorldSpaceBones(nBoneMask);
		return;
	}

	static bool bLoggedMissingCalcBones = false;
	if (!bLoggedMissingCalcBones)
	{
		bLoggedMissingCalcBones = true;
		L_PRINT(LOG_WARNING) << _XS("[BONES] CalcWorldSpaceBones unresolved");
	}
}
