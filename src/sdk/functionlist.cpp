#include "functionlist.h"
#include "../utilities/log.h"
#include "../utilities/xorstr.h"

// ---------------------------------------------------------------
// helper: resolve a pattern and assign to a function pointer
// ---------------------------------------------------------------
template <typename T>
static bool ResolveFunction(T& fnOut, const char* szName, const char* szModule,
                            const char* szPattern, MEM::ESearchType eType = MEM::ESearchType::NONE,
                            std::int32_t nOffset = 0)
{
    const auto uAddr = MEM::FindPattern(szModule, szPattern, eType, nOffset);
    fnOut = reinterpret_cast<T>(uAddr);

    if (!fnOut)
    {
        L_PRINT(LOG_WARNING) << _XS("[SDK_FUNC] FAILED: ") << szName;
        return false;
    }

    L_PRINT(LOG_INFO) << _XS("[SDK_FUNC] ") << szName << _XS(" = 0x") << reinterpret_cast<const void*>(uAddr);
    return true;
}

// ---------------------------------------------------------------
// initialize all SDK function pointers
// ---------------------------------------------------------------
bool SDK_FUNC::Initialize()
{
    L_PRINT(LOG_INFO) << _XS("--- resolving SDK function pointers ---");

    bool bAllCritical = true;
    int nResolved = 0;
    int nTotal = 0;

    // Macro for concise pattern resolution
    #define RESOLVE(FN, MODULE, PATTERN, ...) \
        { ++nTotal; if (ResolveFunction(FN, #FN, MODULE, PATTERN, ##__VA_ARGS__)) ++nResolved; }

    #define RESOLVE_CRITICAL(FN, MODULE, PATTERN, ...) \
        { ++nTotal; if (ResolveFunction(FN, #FN, MODULE, PATTERN, ##__VA_ARGS__)) ++nResolved; else bAllCritical = false; }

    const char* CLIENT = PATTERNS::MODULES::CLIENT;

    // --- Entity System ---
    RESOLVE(GetBaseEntity, CLIENT, PATTERNS::FUNCTIONS::GET_BASE_ENTITY);
    RESOLVE(GetLocalPlayerController, CLIENT, PATTERNS::FUNCTIONS::GET_LOCAL_PLAYER_CONTROLLER);

    // --- Inventory ---
    RESOLVE(CCSInventoryManager_Get, CLIENT,
        "E8 ? ? ? ? 48 8B D8 E8 ? ? ? ? 8B 70",
        MEM::ESearchType::CALL);
    RESOLVE(EquipItemInLoadout, CLIENT, PATTERNS::FUNCTIONS::EQUIP_ITEM_IN_LOADOUT);
    RESOLVE(GetItemInLoadout, CLIENT,
        "40 55 48 83 EC ? 49 63 E8");

    // --- Econ ---
    RESOLVE(CreateSharedObjectSubclassEconItem, CLIENT,
        "48 83 EC ? B9 ? ? ? ? E8 ? ? ? ? 48 85 C0 74 ? 48 8D 0D ? ? ? ? C7 40");
    RESOLVE(CEconItemSchema_GetAttributeDefinitionInterface, CLIENT,
        "E8 ? ? ? ? 48 85 C0 74 ? E8 ? ? ? ? 0F B7 14 3B 48 8B C8 E8 ? ? ? ? 0F B6 48",
        MEM::ESearchType::CALL);
    RESOLVE(CEconItem_SetDynamicAttributeValueUint, CLIENT,
        "E9 ? ? ? ? CC CC CC CC CC CC CC CC CC CC CC CC CC CC CC 49 8B C0 48 8B CA 48 8B D0",
        MEM::ESearchType::CALL);
    RESOLVE(CEconItem_SerializeToProtoBufItem, CLIENT,
        "40 55 56 48 83 EC ? 48 8B 41 ? 48 8B F2");

    // --- Econ Views ---
    RESOLVE(C_EconItemView_GetStaticData, CLIENT,
        "40 56 48 83 EC ? 48 89 5C 24 ? 48 8B F1 48 8B 1D");
    RESOLVE(C_EconItemView_GetBasePlayerWeaponVData, CLIENT,
        "48 81 EC ? ? ? ? 48 85 C9 75 ? 33 C0 48 81 C4 ? ? ? ? C3 48 89 9C 24");
    RESOLVE(C_EconItemView_GetCustomPaintKitIndex, CLIENT,
        "48 89 5C 24 ? 57 48 83 EC ? 8B 15 ? ? ? ? 48 8B F9 65 48 8B 04 25");

    // --- Bones ---
    RESOLVE(CalcWorldSpaceBones, CLIENT, PATTERNS::FUNCTIONS::CALC_WORLD_SPACE_BONES);
    RESOLVE(GetBoneIdByName, CLIENT, PATTERNS::FUNCTIONS::GET_BONE_ID_BY_NAME);

    // --- Entity Utilities ---
    RESOLVE(ComputeHitboxSurroundingBox, CLIENT,
        "48 89 5C 24 10 48 89 6C 24 18 56 57 41 56 B8 A0");
    RESOLVE(SetBodyGroup, CLIENT,
        "85 D2 0F 88 5C");

    // --- Input/View ---
    RESOLVE(GetViewAngles, CLIENT, PATTERNS::FUNCTIONS::GET_VIEW_ANGLES);
    RESOLVE(SetViewAngles, CLIENT, PATTERNS::FUNCTIONS::SET_VIEW_ANGLES);

    // --- Visual ---
    RESOLVE(ScreenTransform, CLIENT, PATTERNS::FUNCTIONS::SCREEN_TRANSFORM);
    RESOLVE(LineGoesThroughSmoke, CLIENT, PATTERNS::FUNCTIONS::LINE_GOES_THROUGH_SMOKE);

    // --- Tracing ---
    RESOLVE(TraceShape, CLIENT, PATTERNS::FUNCTIONS::TRACE_SHAPE);
    RESOLVE(CTraceFilter_Constructor, CLIENT, PATTERNS::FUNCTIONS::TRACE_FILTER_CTOR);

    // --- Weapon ---
    RESOLVE(UpdateSubclass, CLIENT, PATTERNS::FUNCTIONS::UPDATE_SUBCLASS);
    RESOLVE(UpdateSkin, CLIENT, PATTERNS::FUNCTIONS::UPDATE_SKIN);
    RESOLVE(UpdateCompositeMaterial, CLIENT,
        "E8 ? ? ? ? 48 8D 8B ? ? ? ? 48 89 BC 24",
        MEM::ESearchType::CALL);
    RESOLVE(GetInaccuracy, CLIENT,
        "48 89 5C 24 10 55 56 57 48 81 EC ? ? ? ? 44 0F 29 84 24 80 00 00 00");
    RESOLVE(GetSpread, CLIENT,
        "48 83 EC ? 48 63 91 ? ? ? ? 48 8B 81 ? ? ? ? 0F 29 74 24 ? 85 D2");

    // --- Model ---
    RESOLVE(SetModel, CLIENT,
        "40 53 48 83 EC ? 48 8B D9 4C 8B C2 48 8B 0D ? ? ? ? 48 8D 54 24 40");
    RESOLVE(SetMeshGroupMask, CLIENT,
        "48 89 5C 24 ? 48 89 74 24 ? 57 48 83 EC ? 48 8D 99 ? ? ? ? 48 8B 71");

    // --- HUD ---
    RESOLVE(FindHudElement, CLIENT, PATTERNS::FUNCTIONS::FIND_HUD_ELEMENT);
    RESOLVE(SetLocalPlayerReady, CLIENT, PATTERNS::FUNCTIONS::SET_LOCAL_PLAYER_READY);

    // --- Subtick/Bypass ---
    RESOLVE(CreateSubtickMoveStep, CLIENT, PATTERNS::FUNCTIONS::CREATE_SUBTICK_MOVE_STEP);
    RESOLVE(ProtobufAddToRepeatedPtrElement, CLIENT, PATTERNS::FUNCTIONS::PROTOBUF_ADD_REPEATED_PTR);

    // --- Game Events ---
    RESOLVE(IGameEvent_GetName, CLIENT,
        "8B 41 14 0F BA E0 1E 73 05 48 8D 41 18 C3");
    RESOLVE(IGameEvent_GetInt64, CLIENT,
        "48 89 5C 24 ? 48 89 6C 24 ? 48 89 74 24 ? 57 48 83 EC ? 48 8B 01 41 8B F0");
    RESOLVE(IGameEvent_GetPlayerController, CLIENT,
        "48 83 EC 38 8B 02 4C 8D 44 24 20");
    RESOLVE(IGameEvent_GetString, CLIENT,
        "48 83 EC 38 8B 02 48 83 C1 58 89 44 24 20 8B 42 04 89 44 24 24 48 8B 42 08 48 8D 54 24 20 48 89 44 24 28 E8 ? ? ? ? 48 83 C4 38 C3 CC CC CC 33 C9");
    RESOLVE(IGameEvent_SetString, CLIENT,
        "48 83 EC 38 8B 02 48 83 C1 58 89 44 24 20 41 B1 1A");

    // --- GC Cache ---
    RESOLVE(CGCCache_CreateBaseTypeCache, CLIENT,
        "E8 ? ? ? ? 41 8B D5 49 8B CD",
        MEM::ESearchType::CALL);
    RESOLVE(CGCCache_FindTypeCache, CLIENT,
        "4C 8B 49 18 44 8B D2 4C 63 41 10 4F 8D 1C C1 49 8B C3");

    #undef RESOLVE
    #undef RESOLVE_CRITICAL

    L_PRINT(LOG_INFO) << _XS("[SDK_FUNC] resolved ") << nResolved << _XS("/") << nTotal << _XS(" functions");

    return true; // non-fatal — features check individual pointers
}
