#pragma once
#include "../entity.h"

// ---------------------------------------------------------------
// GAMEUTIL — local player utility functions
//
// Replaces ref1's CL_Players singleton with a cleaner namespace.
// All functions operate on the current local player state.
// ---------------------------------------------------------------
namespace GAMEUTIL
{
    // get local player controller
    CCSPlayerController* GetLocalPlayerController();

    // get local player pawn
    C_CSPlayerPawn* GetLocalPlayerPawn();

    // get local player origin (scene node origin)
    Vector3 GetLocalOrigin();

    // get local player eye origin (origin + view offset)
    Vector3 GetLocalEyeOrigin();

    // is local player alive
    bool IsLocalPlayerAlive();

    // get entity by index (typed)
    template <typename T = C_BaseEntity>
    T* GetEntityByIndex(int nIndex);

    // iterate all valid player controllers (index 1..64)
    void ForEachPlayerController(void(*fn)(CCSPlayerController* pController, int nIndex));

    // iterate all valid player pawns (index 1..64)
    void ForEachPlayerPawn(void(*fn)(C_CSPlayerPawn* pPawn, int nIndex));
}
