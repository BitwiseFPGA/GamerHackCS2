#include "cl_players.h"
#include "../interfaces/cgameentitysystem.h"
#include "../interfaces/iengineclient.h"
#include "../../core/interfaces.h"

// ---------------------------------------------------------------
// GetLocalPlayerController
// ---------------------------------------------------------------
CCSPlayerController* GAMEUTIL::GetLocalPlayerController()
{
    if (!I::Engine || !I::Engine->IsInGame())
        return nullptr;

    if (!I::GameEntitySystem)
        return nullptr;

    const int nIndex = I::Engine->GetLocalPlayer();
    if (nIndex <= 0)
        return nullptr;

    return I::GameEntitySystem->Get<CCSPlayerController>(nIndex);
}

// ---------------------------------------------------------------
// GetLocalPlayerPawn
// ---------------------------------------------------------------
C_CSPlayerPawn* GAMEUTIL::GetLocalPlayerPawn()
{
    CCSPlayerController* pController = GetLocalPlayerController();
    if (!pController)
        return nullptr;

    CBaseHandle hPawn = pController->GetPlayerPawnHandle();
    if (!hPawn.IsValid())
        return nullptr;

    C_CSPlayerPawn* pPawn = I::GameEntitySystem->Get<C_CSPlayerPawn>(hPawn);
    if (pPawn && pPawn->IsPlayerPawn())
        return pPawn;

    return nullptr;
}

// ---------------------------------------------------------------
// GetLocalOrigin
// ---------------------------------------------------------------
Vector3 GAMEUTIL::GetLocalOrigin()
{
    C_CSPlayerPawn* pPawn = GetLocalPlayerPawn();
    if (!pPawn)
        return {};

    return pPawn->GetSceneOrigin();
}

// ---------------------------------------------------------------
// GetLocalEyeOrigin
// ---------------------------------------------------------------
Vector3 GAMEUTIL::GetLocalEyeOrigin()
{
    C_CSPlayerPawn* pPawn = GetLocalPlayerPawn();
    if (!pPawn)
        return {};

    return pPawn->GetSceneOrigin() + pPawn->GetViewOffset();
}

// ---------------------------------------------------------------
// IsLocalPlayerAlive
// ---------------------------------------------------------------
bool GAMEUTIL::IsLocalPlayerAlive()
{
    CCSPlayerController* pController = GetLocalPlayerController();
    if (!pController)
        return false;

    return pController->IsPawnAlive();
}

// ---------------------------------------------------------------
// GetEntityByIndex (template — explicit instantiations)
// ---------------------------------------------------------------
template <typename T>
T* GAMEUTIL::GetEntityByIndex(int nIndex)
{
    if (!I::GameEntitySystem)
        return nullptr;

    return I::GameEntitySystem->Get<T>(nIndex);
}

// explicit instantiations for common types
template C_BaseEntity*        GAMEUTIL::GetEntityByIndex<C_BaseEntity>(int);
template CCSPlayerController* GAMEUTIL::GetEntityByIndex<CCSPlayerController>(int);
template C_CSPlayerPawn*      GAMEUTIL::GetEntityByIndex<C_CSPlayerPawn>(int);

// ---------------------------------------------------------------
// ForEachPlayerController
// ---------------------------------------------------------------
void GAMEUTIL::ForEachPlayerController(void(*fn)(CCSPlayerController* pController, int nIndex))
{
    if (!I::GameEntitySystem || !fn)
        return;

    for (int i = 1; i <= MAX_PLAYERS; ++i)
    {
        C_BaseEntity* pEntity = I::GameEntitySystem->Get<C_BaseEntity>(i);
        if (!pEntity || !pEntity->IsBasePlayerController())
            continue;

        fn(static_cast<CCSPlayerController*>(pEntity), i);
    }
}

// ---------------------------------------------------------------
// ForEachPlayerPawn
// ---------------------------------------------------------------
void GAMEUTIL::ForEachPlayerPawn(void(*fn)(C_CSPlayerPawn* pPawn, int nIndex))
{
    if (!I::GameEntitySystem || !fn)
        return;

    for (int i = 1; i <= MAX_PLAYERS; ++i)
    {
        C_BaseEntity* pEntity = I::GameEntitySystem->Get<C_BaseEntity>(i);
        if (!pEntity || !pEntity->IsBasePlayerController())
            continue;

        CCSPlayerController* pController = static_cast<CCSPlayerController*>(pEntity);
        CBaseHandle hPawn = pController->GetPlayerPawnHandle();
        if (!hPawn.IsValid())
            continue;

        C_CSPlayerPawn* pPawn = I::GameEntitySystem->Get<C_CSPlayerPawn>(hPawn);
        if (pPawn && pPawn->IsPlayerPawn())
            fn(pPawn, i);
    }
}
