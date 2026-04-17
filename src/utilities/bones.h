#pragma once
#include "../sdk/datatypes/vector.h"

class C_CSPlayerPawn;
class CSkeletonInstance;

namespace BONES
{
	// get world-space position of a bone by index
	bool GetBonePosition(C_CSPlayerPawn* pPawn, int nBoneIndex, Vector3& outPos);

	// get world-space position of a bone by name (reference-code-1 style)
	Vector3 GetBonePositionByName(C_CSPlayerPawn* pPawn, const char* szBoneName);

	// calculate world-space bones (force update if needed)
	void CalcWorldSpaceBones(CSkeletonInstance* pSkeleton, uint32_t nBoneMask);

	// bone pair definitions for skeleton drawing
	struct BonePair
	{
		int nParent;
		int nChild;
	};

	// standard bone connections for drawing player skeleton
	inline constexpr BonePair arrSkeletonPairs[] = {
		{ 0,  2  }, // pelvis -> spine1
		{ 2,  4  }, // spine1 -> spine3 (chest)
		{ 4,  5  }, // spine3 -> neck
		{ 5,  6  }, // neck -> head
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

	inline constexpr int NUM_SKELETON_PAIRS = sizeof(arrSkeletonPairs) / sizeof(arrSkeletonPairs[0]);
}
