#pragma once
#include "../utilities/memory.h"
#include "../core/patterns.h"
#include <cstdint>

// ---------------------------------------------------------------
// forward declarations
// ---------------------------------------------------------------
class CGameEntitySystem;
class CCSPlayerController;
class CCSPlayerInventory;
class CCSInventoryManager;
class C_CSPlayerPawn;
class C_BaseEntity;
class C_BaseModelEntity;
class C_EconItemView;
class CCSWeaponBaseVData;
class CEconItemDefinition;
class CEconItem;
class CSkeletonInstance;
class CGameSceneNode;
class CCSGOInput;
class CTraceFilter;
class CGameTrace;
class IGameEvent;
class KeyValues;
class CBaseHandle;
class C_CSWeaponBase;
class C_CSWeaponBaseGun;
class CSOEconItem;
struct Vector3;
struct QAngle;
struct Ray_t;

// ---------------------------------------------------------------
// SDK_FUNC — pattern-resolved function pointers
//
// All game function addresses are resolved once at init time via
// MEM::FindPattern. The DECLARE_SDK_FUNCTION macro creates both
// the function-pointer type alias and the inline storage.
// ---------------------------------------------------------------

#define DECLARE_SDK_FUNCTION(ReturnType, Name, ...) \
    using fn_##Name = ReturnType(__fastcall*)(__VA_ARGS__); \
    inline fn_##Name Name = nullptr;

namespace SDK_FUNC {
    // --- Entity System ---
    DECLARE_SDK_FUNCTION(void*, GetBaseEntity, CGameEntitySystem*, int);
    DECLARE_SDK_FUNCTION(CCSPlayerController*, GetLocalPlayerController, int);

    // --- Inventory ---
    DECLARE_SDK_FUNCTION(CCSInventoryManager*, CCSInventoryManager_Get);
    DECLARE_SDK_FUNCTION(bool, EquipItemInLoadout, CCSInventoryManager*, int, int, uint64_t);
    DECLARE_SDK_FUNCTION(C_EconItemView*, GetItemInLoadout, CCSPlayerInventory*, int, int);

    // --- Econ ---
    DECLARE_SDK_FUNCTION(CEconItem*, CreateSharedObjectSubclassEconItem);
    DECLARE_SDK_FUNCTION(void*, CEconItemSchema_GetAttributeDefinitionInterface, void*, int);
    DECLARE_SDK_FUNCTION(void, CEconItem_SetDynamicAttributeValueUint, CEconItem*, void*, void*);
    DECLARE_SDK_FUNCTION(void*, CEconItem_SerializeToProtoBufItem, CEconItem*, CSOEconItem*);

    // --- Econ Views ---
    DECLARE_SDK_FUNCTION(CEconItemDefinition*, C_EconItemView_GetStaticData, C_EconItemView*);
    DECLARE_SDK_FUNCTION(CCSWeaponBaseVData*, C_EconItemView_GetBasePlayerWeaponVData, C_EconItemView*);
    DECLARE_SDK_FUNCTION(int, C_EconItemView_GetCustomPaintKitIndex, C_EconItemView*);

    // --- Bones ---
    DECLARE_SDK_FUNCTION(void, CalcWorldSpaceBones, CSkeletonInstance*, uint32_t);
    DECLARE_SDK_FUNCTION(int, GetBoneIdByName, C_BaseEntity*, const char*);

    // --- Entity Utilities ---
    DECLARE_SDK_FUNCTION(bool, ComputeHitboxSurroundingBox, C_BaseEntity*, Vector3*, Vector3*);
    DECLARE_SDK_FUNCTION(void, SetBodyGroup, C_BaseEntity*, int, unsigned int);

    // --- Input/View ---
    DECLARE_SDK_FUNCTION(QAngle*, GetViewAngles, CCSGOInput*, int32_t);
    DECLARE_SDK_FUNCTION(void, SetViewAngles, CCSGOInput*, int32_t, QAngle*);

    // --- Visual ---
    DECLARE_SDK_FUNCTION(bool, ScreenTransform, const Vector3&, Vector3&);
    DECLARE_SDK_FUNCTION(float, LineGoesThroughSmoke, const Vector3&, const Vector3&, int64_t);

    // --- Tracing ---
    DECLARE_SDK_FUNCTION(void, TraceShape, void*, const Ray_t*, const Vector3*, const Vector3*, const CTraceFilter*, CGameTrace*);
    DECLARE_SDK_FUNCTION(void, CTraceFilter_Constructor, CTraceFilter*, void*, uint64_t, int, uint16_t);

    // --- Weapon ---
    DECLARE_SDK_FUNCTION(void, UpdateSubclass, C_CSWeaponBase*);
    DECLARE_SDK_FUNCTION(void, UpdateSkin, C_CSWeaponBase*, bool);
    DECLARE_SDK_FUNCTION(void, UpdateCompositeMaterial, void*, bool);
    DECLARE_SDK_FUNCTION(float, GetInaccuracy, C_CSWeaponBaseGun*, float*, float*);
    DECLARE_SDK_FUNCTION(float, GetSpread, C_CSWeaponBaseGun*);

    // --- Model ---
    DECLARE_SDK_FUNCTION(void, SetModel, C_BaseModelEntity*, const char*);
    DECLARE_SDK_FUNCTION(void, SetMeshGroupMask, CGameSceneNode*, uint64_t);

    // --- HUD ---
    DECLARE_SDK_FUNCTION(void*, FindHudElement, const char*);
    DECLARE_SDK_FUNCTION(bool, SetLocalPlayerReady, void*, const char*);

    // --- Subtick/Bypass ---
    DECLARE_SDK_FUNCTION(void*, CreateSubtickMoveStep, void*);
    DECLARE_SDK_FUNCTION(void*, ProtobufAddToRepeatedPtrElement, void*, void*);

    // --- Game Events ---
    DECLARE_SDK_FUNCTION(const char*, IGameEvent_GetName, IGameEvent*);
    DECLARE_SDK_FUNCTION(int64_t, IGameEvent_GetInt64, IGameEvent*, const char*);
    DECLARE_SDK_FUNCTION(CCSPlayerController*, IGameEvent_GetPlayerController, IGameEvent*, void*);
    DECLARE_SDK_FUNCTION(const char*, IGameEvent_GetString, IGameEvent*, void*, void*);
    DECLARE_SDK_FUNCTION(const char*, IGameEvent_SetString, IGameEvent*, void*, const char*);

    // --- GC Cache ---
    DECLARE_SDK_FUNCTION(void*, CGCCache_CreateBaseTypeCache, void*, int);
    DECLARE_SDK_FUNCTION(void*, CGCCache_FindTypeCache, void*, int);

    // --- Initialize all function pointers ---
    bool Initialize();
}

#undef DECLARE_SDK_FUNCTION
