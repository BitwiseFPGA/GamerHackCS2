#pragma once
#include "../entity.h"

// ---------------------------------------------------------------
// GAMEUTIL — weapon utility functions
//
// Replaces ref1's CL_Weapons singleton with a cleaner namespace.
// All functions operate on the current local player's weapon state.
// ---------------------------------------------------------------
namespace GAMEUTIL
{
    // get active weapon of local player
    C_CSWeaponBase* GetLocalActiveWeapon();

    // get weapon VData for local player's active weapon
    CCSWeaponBaseVData* GetLocalWeaponVData();

    // get weapon type of local player's active weapon
    int GetLocalWeaponType();

    // get weapon definition index
    int GetLocalWeaponDefinitionIndex();

    // is local weapon a knife
    bool IsLocalWeaponKnife();

    // is local weapon a grenade
    bool IsLocalWeaponGrenade();

    // is local weapon a pistol
    bool IsLocalWeaponPistol();

    // get weapon from entity handle
    C_CSWeaponBase* GetWeaponFromHandle(CBaseHandle hWeapon);
}
