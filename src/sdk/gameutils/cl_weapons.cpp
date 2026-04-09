#include "cl_weapons.h"
#include "cl_players.h"
#include "../interfaces/cgameentitysystem.h"
#include "../../core/interfaces.h"

// ---------------------------------------------------------------
// GetLocalActiveWeapon
// ---------------------------------------------------------------
C_CSWeaponBase* GAMEUTIL::GetLocalActiveWeapon()
{
    C_CSPlayerPawn* pPawn = GetLocalPlayerPawn();
    if (!pPawn)
        return nullptr;

    CCSPlayer_WeaponServices* pWeaponSvc = pPawn->GetWeaponServices();
    if (!pWeaponSvc)
        return nullptr;

    CBaseHandle hWeapon = pWeaponSvc->GetActiveWeapon();
    if (!hWeapon.IsValid())
        return nullptr;

    return I::GameEntitySystem->Get<C_CSWeaponBase>(hWeapon);
}

// ---------------------------------------------------------------
// GetLocalWeaponVData
// ---------------------------------------------------------------
CCSWeaponBaseVData* GAMEUTIL::GetLocalWeaponVData()
{
    C_CSWeaponBase* pWeapon = GetLocalActiveWeapon();
    if (!pWeapon)
        return nullptr;

    C_AttributeContainer* pAttrMgr = pWeapon->GetAttributeManager();
    if (!pAttrMgr)
        return nullptr;

    C_EconItemView* pItemView = pAttrMgr->GetItem();
    if (!pItemView)
        return nullptr;

    return pItemView->GetBasePlayerWeaponVData();
}

// ---------------------------------------------------------------
// GetLocalWeaponType
// ---------------------------------------------------------------
int GAMEUTIL::GetLocalWeaponType()
{
    CCSWeaponBaseVData* pVData = GetLocalWeaponVData();
    if (!pVData)
        return static_cast<int>(CSWeaponType_t::WEAPONTYPE_UNKNOWN);

    return pVData->GetWeaponType();
}

// ---------------------------------------------------------------
// GetLocalWeaponDefinitionIndex
// ---------------------------------------------------------------
int GAMEUTIL::GetLocalWeaponDefinitionIndex()
{
    C_CSWeaponBase* pWeapon = GetLocalActiveWeapon();
    if (!pWeapon)
        return -1;

    C_AttributeContainer* pAttrMgr = pWeapon->GetAttributeManager();
    if (!pAttrMgr)
        return -1;

    C_EconItemView* pItemView = pAttrMgr->GetItem();
    if (!pItemView)
        return -1;

    return static_cast<int>(pItemView->GetItemDefinitionIndex());
}

// ---------------------------------------------------------------
// IsLocalWeaponKnife
// ---------------------------------------------------------------
bool GAMEUTIL::IsLocalWeaponKnife()
{
    return GetLocalWeaponType() == static_cast<int>(CSWeaponType_t::WEAPONTYPE_KNIFE);
}

// ---------------------------------------------------------------
// IsLocalWeaponGrenade
// ---------------------------------------------------------------
bool GAMEUTIL::IsLocalWeaponGrenade()
{
    return GetLocalWeaponType() == static_cast<int>(CSWeaponType_t::WEAPONTYPE_GRENADE);
}

// ---------------------------------------------------------------
// IsLocalWeaponPistol
// ---------------------------------------------------------------
bool GAMEUTIL::IsLocalWeaponPistol()
{
    return GetLocalWeaponType() == static_cast<int>(CSWeaponType_t::WEAPONTYPE_PISTOL);
}

// ---------------------------------------------------------------
// GetWeaponFromHandle
// ---------------------------------------------------------------
C_CSWeaponBase* GAMEUTIL::GetWeaponFromHandle(CBaseHandle hWeapon)
{
    if (!hWeapon.IsValid() || !I::GameEntitySystem)
        return nullptr;

    return I::GameEntitySystem->Get<C_CSWeaponBase>(hWeapon);
}
