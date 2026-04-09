#pragma once
#include "../utilities/memory.h"
#include "../utilities/xorstr.h"

// ============================================================================
// PATTERNS.H - All game patterns, offsets, and version-dependent constants
// 
// This file is the SINGLE SOURCE OF TRUTH for all values that need updating
// when CS2 receives a game update. Update patterns here and everything works.
//
// HOW TO UPDATE:
// 1. Open CS2 in IDA Pro or Ghidra
// 2. Search for the string/xref listed in the comment for each pattern
// 3. Navigate to the function and copy the new byte signature
// 4. Update the pattern string below
// ============================================================================

namespace PATTERNS {

    // ========================================================================
    // MODULE NAMES
    // ========================================================================
    namespace MODULES {
        constexpr const char* CLIENT          = "client.dll";
        constexpr const char* ENGINE2         = "engine2.dll";
        constexpr const char* SCHEMASYSTEM    = "schemasystem.dll";
        constexpr const char* INPUTSYSTEM     = "inputsystem.dll";
        constexpr const char* TIER0           = "tier0.dll";
        constexpr const char* LOCALIZE        = "localize.dll";
        constexpr const char* MATERIALSYSTEM2 = "materialsystem2.dll";
        constexpr const char* NAVSYSTEM       = "navsystem.dll";
        constexpr const char* GAMEOVERLAY     = "gameoverlayrenderer64.dll";
        constexpr const char* SDL3            = "SDL3.dll";
        constexpr const char* NETWORKSYSTEM   = "networksystem.dll";
        constexpr const char* FILESYSTEM      = "filesystem_stdio.dll";
        constexpr const char* SCENESYSTEM     = "scenesystem.dll";
        constexpr const char* PARTICLES       = "particles.dll";
        constexpr const char* ANIMATIONSYSTEM = "animationsystem.dll";
        constexpr const char* PANORAMA        = "panorama.dll";
        constexpr const char* WORLDRENDERER   = "worldrenderer.dll";
        constexpr const char* DXGI            = "dxgi.dll";
        constexpr const char* RENDERSYSTEM    = "rendersystemdx11.dll";
    }

    // ========================================================================
    // INTERFACE VERSIONS
    // ========================================================================
    namespace INTERFACES {
        constexpr const char* SOURCE2_ENGINE_TO_CLIENT = "Source2EngineToClient001";
        constexpr const char* SCHEMA_SYSTEM           = "SchemaSystem_001";
        constexpr const char* SOURCE2_CLIENT           = "Source2Client002";
        constexpr const char* LOCALIZE                 = "Localize_001";
        constexpr const char* MATERIAL_SYSTEM2         = "VMaterialSystem2_001";
        constexpr const char* ENGINE_CVAR              = "Source2EngineToClientStringTable001";
        constexpr const char* INPUT_SYSTEM             = "InputSystemVersion001";
        constexpr const char* GAME_RESOURCE_SERVICE    = "GameResourceServiceClientV001";
        constexpr const char* NETWORK_CLIENT_SERVICE   = "NetworkClientService_001";
    }

    // ========================================================================
    // GAME FUNCTION PATTERNS
    // Each pattern has: name, module, byte pattern, search type, offset
    // Comments explain how to find the pattern in a disassembler
    // ========================================================================
    namespace FUNCTIONS {
        // --- Entity System ---
        // Search: "CGameEntitySystem::GetBaseEntity" or xref from GetHighestEntityIndex
        constexpr const char* GET_BASE_ENTITY = "81 FA ? ? 00 00 77 ? 8B C2 EB 37";

        // --- Local Player ---
        // Search: string "m_hPawn" or "GetLocalPlayerController"
        constexpr const char* GET_LOCAL_PLAYER_CONTROLLER = "48 83 EC ? E8 ? ? ? ? 48 85 C0 74 ? 8B 88";

        // --- CreateMove ---
        // Search: "cl: CreateMove clamped invalid attack history index" xref in client.dll
        constexpr const char* CREATE_MOVE = "85 D2 0F 85 ? ? ? ? 48 8B C4 44 88 40 18";

        // --- FrameStageNotify ---
        // Search: "FrameStageNotify" or FRAME_RENDER_START references
        constexpr const char* FRAME_STAGE_NOTIFY = "48 89 5C 24 ? 48 89 6C 24 ? 57 48 83 EC ? 48 8B F9 33 ED";

        // --- MouseInputEnabled ---
        // VFunc on ISource2Client, index 19

        // --- GetMatrixForView ---
        // Search: CViewRender->OnRenderStart -> GetMatricesForView
        constexpr const char* GET_MATRIX_FOR_VIEW = "40 53 48 81 EC ? ? ? ? 49 8B D9";

        // --- OverrideView ---
        // Search: "ClientModeCSNormal::OverrideView" in client.dll
        constexpr const char* OVERRIDE_VIEW = "48 89 5C 24 ? 48 89 6C 24 ? 48 89 74 24 ? 57 41 56 41 57 48 83 EC ? 48 8B FA E8";

        // --- LevelInit ---
        // Search: "game_newmap" string xref -> first function
        constexpr const char* LEVEL_INIT = "48 89 5C 24 ? 57 48 83 EC ? 48 8B 0D ? ? ? ? 48 8B FA";

        // --- LevelShutdown ---
        // Search: "map_shutdown" string in ClientModeShared
        constexpr const char* LEVEL_SHUTDOWN = "48 83 EC ? 48 8B 0D ? ? ? ? 48 8D 15 ? ? ? ? 45 33 C9 45 33 C0 48 8B 01 FF 50 ? 48 85 C0 74 15";

        // --- Input/View Angles ---
        // Search: "SetViewAngles" or viewangle references
        constexpr const char* GET_VIEW_ANGLES = "4C 8B C1 83 FA ? 74 ? 48 63 C2";
        constexpr const char* SET_VIEW_ANGLES = "F2 41 0F 10 00 48 63 CA 48 C1 E1 06";

        // --- GlobalVars ---
        // Search: "CBaseEntity::SetParent" or GlobalVars xref (now in client.dll)
        constexpr const char* GLOBAL_VARS = "48 8B 05 ? ? ? ? 0F 57 C0 8B 48";

        // --- CCSGOInput ---
        // Search: xref "CCSGOInput" pointer in client.dll
        constexpr const char* CSGO_INPUT = "48 8B 0D ? ? ? ? 48 8B 01 FF 50 ? 8B DF";

        // --- CGameTraceManager ---
        // Search: xref CGameTraceManager in TraceShape
        constexpr const char* GAME_TRACE_MANAGER = "4C 8B 3D ? ? ? ? 24 ? 0C ? 66 0F 7F";

        // --- SwapChain ---
        // Search: xref swap chain store in CRenderDeviceDx11 (rendersystemdx11.dll)
        // resolves the "48 89 2D" (mov [rip+disp32], rbp) which stores the swap chain ptr
        constexpr const char* SWAP_CHAIN = "48 89 2D ? ? ? ? 66 0F 7F 05";

        // --- SwapChain (for hooks) ---
        // Search: swap chain object load with null check in rendersystemdx11.dll
        constexpr const char* SWAP_CHAIN_HOOK = "48 8B 0D ? ? ? ? 48 85 C9 74 ? 8B 41";

        // --- Present (gameoverlayrenderer64.dll) ---
        // Hook the Steam overlay's Present wrapper directly
        constexpr const char* PRESENT_OVERLAY = "48 89 5C 24 ? 48 89 6C 24 ? 56 57 41 54 41 56 41 57 48 83 EC ? 41 8B E8";

        // --- ViewMatrix (world-to-projection) ---
        // Search: lea rcx, [rip+ViewMatrix] followed by shl rax, 6
        constexpr const char* VIEW_MATRIX = "48 8D 0D ? ? ? ? 48 C1 E0 06";

        // --- Entity Events ---
        constexpr const char* FIRE_EVENT_CLIENT_SIDE = "40 53 41 54 41 56 48 83 EC ? 4C 8B F2";

        // --- Inventory/Economy ---
        // Search: "EquipItemInLoadout" or "CCSInventoryManager"
        constexpr const char* INVENTORY_MANAGER = "48 8D 0D ? ? ? ? E8 ? ? ? ? 48 85 C0 74 ? 48 89 44 24 68";
        constexpr const char* EQUIP_ITEM_IN_LOADOUT = "48 89 5C 24 ? 48 89 74 24 ? 55 57 41 56 48 8D AC 24 80";

        // --- Subtick/Bypass ---
        constexpr const char* CREATE_SUBTICK_MOVE_STEP = "48 89 5C 24 ? 48 89 6C 24 ? 56 57 41 56 48 83 EC ? 49 8B D8 48 8D";
        constexpr const char* PROTOBUF_ADD_REPEATED_PTR = "48 89 5C 24 ? 48 89 74 24 ? 57 48 83 EC ? 48 8B F9 48 8B F2 48 8B 49 38";

        // --- Bones ---
        constexpr const char* CALC_WORLD_SPACE_BONES = "48 89 5C 24 ? 55 56 57 41 54 41 55 41 56 41 57 48 81 EC ? ? ? ? 4C 8B";
        constexpr const char* GET_BONE_ID_BY_NAME = "48 89 5C 24 ? 48 89 6C 24 ? 48 89 74 24 ? 57 48 83 EC ? 49 8B F0 48 8B EA 48 8B D9 E8";

        // --- Visual ---
        constexpr const char* SCREEN_TRANSFORM = "48 89 5C 24 ? 48 89 6C 24 ? 48 89 74 24 ? 57 48 83 EC ? 49 8B E8 48 8B DA";
        constexpr const char* LINE_GOES_THROUGH_SMOKE = "48 89 5C 24 ? 48 89 6C 24 ? 48 89 74 24 ? 57 41 56 41 57 48 83 EC ? 44 0F B6";

        // --- Weapon ---
        constexpr const char* UPDATE_SUBCLASS = "48 89 5C 24 ? 57 48 83 EC ? 48 8B D9 E8 ? ? ? ? 48 8B F8 48 85 C0 75";
        constexpr const char* UPDATE_SKIN = "48 89 74 24 ? 48 89 7C 24 ? 41 56 48 83 EC ? 48 8B 31";

        // --- Trace ---
        constexpr const char* TRACE_SHAPE = "48 89 5C 24 ? 48 89 6C 24 ? 48 89 74 24 ? 57 41 54 41 55 41 56 41 57 48 81 EC ? ? ? ? 4C 8B 71";
        constexpr const char* TRACE_FILTER_CTOR = "48 89 5C 24 ? 48 89 4C 24 ? 55 56 57 41 54 41 55 41 56 41 57 48 8D AC 24 40";

        // --- HUD ---
        constexpr const char* FIND_HUD_ELEMENT = "48 89 5C 24 ? 48 89 74 24 ? 48 89 7C 24 ? 55 48 8D AC 24 ? ? ? ? 48 81 EC ? ? ? ? 49 8B F0 48 8B DA 48 8B F9 E8";
        constexpr const char* SET_LOCAL_PLAYER_READY = "48 83 EC ? 48 8B 05 ? ? ? ? 0F B6 D1";

        // --- CRC Spoofing (SerializePartialToArray) ---
        constexpr const char* SERIALIZE_PARTIAL_TO_ARRAY = "48 89 5C 24 ? 55 56 57 48 81 EC ? ? ? ? 48 8B 05 ? ? ? ? 48 33 C4";

        // --- KeyValues ---
        constexpr const char* LOAD_KV3 = "48 89 5C 24 ? 48 89 6C 24 ? 48 89 74 24 ? 48 89 7C 24 ? 41 54 41 56 41 57 48 81 EC ? ? ? ? 48 8B C2 41 0F B6";
    }

    // ========================================================================
    // VTABLE INDICES
    // ========================================================================
    namespace VTABLE {
        namespace D3D {
            constexpr unsigned int PRESENT        = 8;
            constexpr unsigned int RESIZE_BUFFERS = 13;
        }
        namespace CLIENT {
            constexpr unsigned int MOUSE_INPUT_ENABLED  = 19;
            constexpr unsigned int CREATE_MOVE          = 21;
            constexpr unsigned int FRAME_STAGE_NOTIFY   = 36;
        }
        namespace SCHEMA {
            constexpr unsigned int GLOBAL_TYPE_SCOPE   = 11;
            constexpr unsigned int FIND_TYPE_SCOPE      = 13;
            constexpr unsigned int CLASS_INFO            = 44;
        }
        namespace ENTITY_SYSTEM {
            constexpr unsigned int GET_BASE_ENTITY      = 8;
            constexpr unsigned int GET_HIGHEST_INDEX    = 34;
            constexpr unsigned int ON_ADD_ENTITY        = 14;
            constexpr unsigned int ON_REMOVE_ENTITY     = 15;
        }
        namespace TRACE {
            constexpr unsigned int TRACE_SHAPE          = 5;
        }
    }

    // ========================================================================
    // STATIC OFFSETS (from reverse engineering)
    // These are byte offsets into game structures
    // ========================================================================
    namespace OFFSETS {
        // Schema hash table offset in CSchemaSystemTypeScope
        constexpr std::uintptr_t SCHEMA_HASH_TABLE = 0x5C0;
        constexpr int SCHEMA_BLOCK_SIZE = 256;

        // CCollisionProperty
        constexpr std::uintptr_t COLLISION_UNKNOWN_MASK = 0x38;

        // Protobuf message offset
        constexpr std::uintptr_t PROTOBUF_MSG = 0x30;

        // Composite material
        constexpr std::uintptr_t COMPOSITE_MATERIAL = 0x608;

        // CEconItemSchema offsets
        constexpr std::uintptr_t ECON_SCHEMA_SORTED_ITEM_DEF_MAP = 0x128;
        constexpr std::uintptr_t ECON_SCHEMA_PAINT_KITS           = 0x2F0;
        constexpr std::uintptr_t ECON_SCHEMA_MUSIC_KIT_DEFS       = 0x4D8;

        // CEconItemDefinition
        constexpr std::uintptr_t ECON_ITEM_DEF_LOADOUT_SLOT = 0x338;

        // CCSInventoryManager
        constexpr std::uintptr_t INV_MANAGER_GET_LOCAL_INVENTORY = 0x3F540;
        constexpr std::uintptr_t INV_PLAYER_GC_CACHE = 0x68;

        // CCSGOInput
        constexpr std::uintptr_t INPUT_THIRD_PERSON = 0x229;
        constexpr std::uintptr_t INPUT_MOVES = 0xB58;
        constexpr std::uintptr_t INPUT_MOVES_VIEW_ANGLES = 0x430;

        // CUserCmdArray
        constexpr std::uintptr_t USERCMD_SEQUENCE_NUMBER = 0x5910;

        // CViewSetup
        constexpr std::uintptr_t VIEW_SETUP_ORIGIN = 0x04A0;
        constexpr std::uintptr_t VIEW_SETUP_ANGLES = 0x04B8;
        constexpr std::uintptr_t VIEW_SETUP_FOV    = 0x0200;

        // CGameEntitySystem
        constexpr std::uintptr_t ENTITY_SYSTEM_HIGHEST_INDEX = 0x20A0;

        // CGlowProperty
        constexpr std::uintptr_t GLOW_OWNER = 0x18;

        // C_EconItemView
        constexpr std::uintptr_t ECON_ITEM_DESCRIPTION = 0x200;

        // CModel bone data
        constexpr std::uintptr_t MODEL_BONE_NAMES = 0x168; // 0x130 + 0x38

        // CModelState
        constexpr std::uintptr_t MODEL_STATE_BONES = 0x80;

        // ISwapChainDx11 -> IDXGISwapChain*
        constexpr std::uintptr_t SWAP_CHAIN_DXGI = 0x170;

        // IGameResourceService -> CGameEntitySystem*
        constexpr std::uintptr_t GAME_RESOURCE_ENTITY_SYSTEM = 0x58;
    }

    // ========================================================================
    // ECON ITEM ATTRIBUTE INDICES
    // ========================================================================
    namespace ECON_ATTRIBUTES {
        constexpr std::uint32_t PAINT_KIT           = 6;
        constexpr std::uint32_t PAINT_SEED          = 7;
        constexpr std::uint32_t PAINT_WEAR          = 8;
        constexpr std::uint32_t STAT_TRAK           = 80;
        constexpr std::uint32_t STAT_TRAK_TYPE      = 81;
        constexpr std::uint32_t STICKER_ID          = 113;
        constexpr std::uint32_t STICKER_WEAR        = 114;
        constexpr std::uint32_t STICKER_SCALE       = 115;
        constexpr std::uint32_t STICKER_ROTATION    = 116;
        constexpr std::uint32_t STICKER_ROTATION_X  = 278;
        constexpr std::uint32_t STICKER_ROTATION_Y  = 279;
        constexpr std::uint32_t MUSIC_ID            = 166;
        constexpr std::uint32_t KEYCHAIN_SLOT_ID_0  = 299;
    }

    // ========================================================================
    // BONE INDICES (common bone names for targeting)
    // ========================================================================
    namespace BONES {
        constexpr int HEAD      = 6;
        constexpr int NECK      = 5;
        constexpr int SPINE_1   = 4;
        constexpr int SPINE_2   = 2;
        constexpr int PELVIS    = 0;

        // Bone names for lookup via GetBoneIdByName
        constexpr const char* NAME_HEAD      = "head_0";
        constexpr const char* NAME_NECK      = "neck_0";
        constexpr const char* NAME_SPINE_1   = "spine_1";
        constexpr const char* NAME_SPINE_2   = "spine_2";
        constexpr const char* NAME_PELVIS    = "pelvis";
        constexpr const char* NAME_ARM_L     = "hand_L";
        constexpr const char* NAME_ARM_R     = "hand_R";
        constexpr const char* NAME_LEG_L     = "ankle_L";
        constexpr const char* NAME_LEG_R     = "ankle_R";
    }
}
