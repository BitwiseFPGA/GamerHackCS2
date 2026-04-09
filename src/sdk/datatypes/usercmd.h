#pragma once
#include <cstdint>
#include <limits>
#include <string>

#include "qangle.h"
#include "vector.h"

// @source: compiled from CS2 protobuf definitions

// helper to pad struct members to match game memory layout
#define _SDK_CONCAT_IMPL(a, b) a##b
#define _SDK_CONCAT(a, b) _SDK_CONCAT_IMPL(a, b)
#define SDK_PAD(SIZE) \
private: \
	std::uint8_t _SDK_CONCAT(_pad, __COUNTER__)[SIZE]; \
public:

// ---------------------------------------------------------------
// protobuf-style repeated pointer field used by CUserCmd internals
// ---------------------------------------------------------------
template <typename T>
struct RepeatedPtrField
{
	struct Rep
	{
		int nAllocatedSize;
		T* tElements[(std::numeric_limits<int>::max() - 2 * sizeof(int)) / sizeof(void*)];
	};

	void* pArena;
	int   nCurrentSize;
	int   nTotalSize;
	Rep*  pRep;
};

// ---------------------------------------------------------------
// base protobuf message with cached has-bits
// ---------------------------------------------------------------
class CBasePB
{
public:
	SDK_PAD(0x8) // vtable
	std::uint32_t nHasBits;   // 0x8
	std::uint64_t nCachedBits; // 0xC

	void SetBits(std::uint64_t nBits) { nCachedBits |= nBits; }
};

static_assert(sizeof(CBasePB) == 0x18);

// ---------------------------------------------------------------
// protobuf message wrappers for angle/vector
// ---------------------------------------------------------------
class CMsgQAngle : public CBasePB
{
public:
	QAngle angValue; // 0x18
};

static_assert(sizeof(CMsgQAngle) == 0x28);

class CMsgVector : public CBasePB
{
public:
	Vector4 vecValue; // 0x18
};

static_assert(sizeof(CMsgVector) == 0x28);

// ---------------------------------------------------------------
// interpolation info for input history
// ---------------------------------------------------------------
class CCSGOInterpolationInfoPB : public CBasePB
{
public:
	float flFraction; // 0x18
	int   nSrcTick;   // 0x1C
	int   nDstTick;   // 0x20
};

static_assert(sizeof(CCSGOInterpolationInfoPB) == 0x28);

// ---------------------------------------------------------------
// single input history snapshot
// ---------------------------------------------------------------
class CCSGOInputHistoryEntryPB : public CBasePB
{
public:
	CMsgQAngle*                pViewAngles;              // 0x18
	CMsgVector*                pShootPosition;           // 0x20
	CMsgVector*                pTargetHeadPositionCheck; // 0x28
	CMsgVector*                pTargetAbsPositionCheck;  // 0x30
	CMsgQAngle*                pTargetAngPositionCheck;  // 0x38
	CCSGOInterpolationInfoPB* cl_interp;                // 0x40
	CCSGOInterpolationInfoPB* sv_interp0;               // 0x48
	CCSGOInterpolationInfoPB* sv_interp1;               // 0x50
	CCSGOInterpolationInfoPB* player_interp;            // 0x58
	int                        nRenderTickCount;         // 0x60
	float                      flRenderTickFraction;     // 0x64
	int                        nPlayerTickCount;         // 0x68
	float                      flPlayerTickFraction;     // 0x6C
	int                        nFrameNumber;             // 0x70
	int                        nTargetEntIndex;          // 0x74
};

static_assert(sizeof(CCSGOInputHistoryEntryPB) == 0x78);

// ---------------------------------------------------------------
// button state protobuf
// ---------------------------------------------------------------
struct CInButtonStatePB : CBasePB
{
	std::uint64_t nValue;        // current state
	std::uint64_t nValueChanged; // changed bits
	std::uint64_t nValueScroll;  // scroll-related bits
};

static_assert(sizeof(CInButtonStatePB) == 0x30);

// ---------------------------------------------------------------
// subtick move step
// ---------------------------------------------------------------
struct CSubtickMoveStep : CBasePB
{
	std::uint64_t nButton;
	bool          bPressed;
	float         flWhen;
	float         flAnalogForwardDelta;
	float         flAnalogLeftDelta;
};

static_assert(sizeof(CSubtickMoveStep) == 0x30);

// ---------------------------------------------------------------
// base user command protobuf
// ---------------------------------------------------------------
class CBaseUserCmdPB : public CBasePB
{
public:
	RepeatedPtrField<CSubtickMoveStep> subtickMovesField; // 0x18
	std::string*       strMoveCrc;                         // 0x38
	CInButtonStatePB*  pInButtonState;                     // 0x40
	CMsgQAngle*        pViewAngles;                        // 0x48
	std::int32_t       nLegacyCommandNumber;               // 0x50
	std::int32_t       nClientTick;                        // 0x54
	float              flForwardMove;                      // 0x58
	float              flSideMove;                         // 0x5C
	float              flUpMove;                           // 0x60
	std::int32_t       nImpulse;                           // 0x64
	std::int32_t       nWeaponSelect;                      // 0x68
	std::int32_t       nRandomSeed;                        // 0x6C
	std::int32_t       nMousedX;                           // 0x70
	std::int32_t       nMousedY;                           // 0x74
	std::uint32_t      nConsumedServerAngleChanges;        // 0x78
	std::int32_t       nCmdFlags;                          // 0x7C
	std::uint32_t      nPawnEntityHandle;                  // 0x80 (not in base size - extended)
};

// ---------------------------------------------------------------
// CS2 user command protobuf (wraps base + history)
// ---------------------------------------------------------------
class CCSGOUserCmdPB
{
public:
	std::uint32_t nHasBits;
	std::uint64_t nCachedSize;
	RepeatedPtrField<CCSGOInputHistoryEntryPB> inputHistoryField;
	CBaseUserCmdPB*  pBaseCmd;
	bool             bLeftHandDesired;
	std::int32_t     nAttack3StartHistoryIndex;
	std::int32_t     nAttack1StartHistoryIndex;
	std::int32_t     nAttack2StartHistoryIndex;

	void CheckAndSetBits(std::uint32_t nBits)
	{
		if (!(nHasBits & nBits))
			nHasBits |= nBits;
	}
};

static_assert(sizeof(CCSGOUserCmdPB) == 0x40);

// ---------------------------------------------------------------
// runtime button state (non-protobuf)
// ---------------------------------------------------------------
struct CInButtonState
{
	SDK_PAD(0x8) // vtable
	std::uint64_t nValue;        // 0x8
	std::uint64_t nValueChanged; // 0x10
	std::uint64_t nValueScroll;  // 0x18
};

static_assert(sizeof(CInButtonState) == 0x20);

// ---------------------------------------------------------------
// high-level user command used in CreateMove
// ---------------------------------------------------------------
class CUserCmd
{
public:
	SDK_PAD(0x8)  // vtable at 0x0
	SDK_PAD(0x10) // unknown padding added after 14.08.2024 update
	CCSGOUserCmdPB csgoUserCmd; // 0x18
	CInButtonState nButtons;    // 0x58
	SDK_PAD(0x20)               // 0x78

	CCSGOInputHistoryEntryPB* GetInputHistoryEntry(int nIndex)
	{
		if (!csgoUserCmd.inputHistoryField.pRep)
			return nullptr;
		if (nIndex >= csgoUserCmd.inputHistoryField.pRep->nAllocatedSize || nIndex >= csgoUserCmd.inputHistoryField.nCurrentSize)
			return nullptr;
		return csgoUserCmd.inputHistoryField.pRep->tElements[nIndex];
	}

	void SetSubTickAngle(const QAngle& angView)
	{
		if (!csgoUserCmd.inputHistoryField.pRep)
			return;

		for (int i = 0; i < csgoUserCmd.inputHistoryField.pRep->nAllocatedSize; i++)
		{
			CCSGOInputHistoryEntryPB* pEntry = GetInputHistoryEntry(i);
			if (!pEntry || !pEntry->pViewAngles)
				continue;

			pEntry->pViewAngles->angValue = angView;
			pEntry->SetBits(0x1); // INPUT_HISTORY_BITS_VIEWANGLES
		}
	}
};

static_assert(sizeof(CUserCmd) == 0x98);

// ---------------------------------------------------------------
// protobuf bit flags for field presence tracking
// ---------------------------------------------------------------
enum ESubtickMoveStepBits : std::uint32_t
{
	MOVESTEP_BITS_BUTTON              = 0x1,
	MOVESTEP_BITS_PRESSED             = 0x2,
	MOVESTEP_BITS_WHEN                = 0x4,
	MOVESTEP_BITS_ANALOG_FORWARD_DELTA = 0x8,
	MOVESTEP_BITS_ANALOG_LEFT_DELTA   = 0x10
};

enum EInputHistoryBits : std::uint32_t
{
	INPUT_HISTORY_BITS_VIEWANGLES              = 0x1,
	INPUT_HISTORY_BITS_SHOOTPOSITION           = 0x2,
	INPUT_HISTORY_BITS_TARGETHEADPOSITIONCHECK = 0x4,
	INPUT_HISTORY_BITS_TARGETABSPOSITIONCHECK  = 0x8,
	INPUT_HISTORY_BITS_TARGETANGCHECK          = 0x10,
	INPUT_HISTORY_BITS_CL_INTERP               = 0x20,
	INPUT_HISTORY_BITS_SV_INTERP0              = 0x40,
	INPUT_HISTORY_BITS_SV_INTERP1              = 0x80,
	INPUT_HISTORY_BITS_PLAYER_INTERP           = 0x100,
	INPUT_HISTORY_BITS_RENDERTICKCOUNT         = 0x200,
	INPUT_HISTORY_BITS_RENDERTICKFRACTION      = 0x400,
	INPUT_HISTORY_BITS_PLAYERTICKCOUNT         = 0x800,
	INPUT_HISTORY_BITS_PLAYERTICKFRACTION      = 0x1000,
	INPUT_HISTORY_BITS_FRAMENUMBER             = 0x2000,
	INPUT_HISTORY_BITS_TARGETENTINDEX          = 0x4000
};

enum EButtonStatePBBits : std::uint32_t
{
	BUTTON_STATE_PB_BITS_BUTTONSTATE1 = 0x1,
	BUTTON_STATE_PB_BITS_BUTTONSTATE2 = 0x2,
	BUTTON_STATE_PB_BITS_BUTTONSTATE3 = 0x4
};

enum EBaseCmdBits : std::uint32_t
{
	BASE_BITS_MOVE_CRC                 = 0x1,
	BASE_BITS_BUTTONPB                 = 0x2,
	BASE_BITS_VIEWANGLES               = 0x4,
	BASE_BITS_COMMAND_NUMBER           = 0x8,
	BASE_BITS_CLIENT_TICK              = 0x10,
	BASE_BITS_FORWARDMOVE              = 0x20,
	BASE_BITS_LEFTMOVE                 = 0x40,
	BASE_BITS_UPMOVE                   = 0x80,
	BASE_BITS_IMPULSE                  = 0x100,
	BASE_BITS_WEAPON_SELECT            = 0x200,
	BASE_BITS_RANDOM_SEED              = 0x400,
	BASE_BITS_MOUSEDX                  = 0x800,
	BASE_BITS_MOUSEDY                  = 0x1000,
	BASE_BITS_CONSUMED_SERVER_ANGLE    = 0x2000,
	BASE_BITS_CMD_FLAGS                = 0x4000,
	BASE_BITS_ENTITY_HANDLE            = 0x8000
};

enum ECSGOUserCmdBits : std::uint32_t
{
	CSGOUSERCMD_BITS_BASECMD       = 0x1,
	CSGOUSERCMD_BITS_LEFTHAND      = 0x2,
	CSGOUSERCMD_BITS_ATTACK3START  = 0x4,
	CSGOUSERCMD_BITS_ATTACK1START  = 0x8,
	CSGOUSERCMD_BITS_ATTACK2START  = 0x10
};

// command button flags (used in CInButtonState::nValue)
enum ECommandButtons : std::uint64_t
{
	IN_ATTACK         = 1ULL << 0,
	IN_JUMP           = 1ULL << 1,
	IN_DUCK           = 1ULL << 2,
	IN_FORWARD        = 1ULL << 3,
	IN_BACK           = 1ULL << 4,
	IN_USE            = 1ULL << 5,
	IN_LEFT           = 1ULL << 7,
	IN_RIGHT          = 1ULL << 8,
	IN_MOVELEFT       = 1ULL << 9,
	IN_MOVERIGHT      = 1ULL << 10,
	IN_SECOND_ATTACK  = 1ULL << 11,
	IN_RELOAD         = 1ULL << 13,
	IN_SPRINT         = 1ULL << 16,
	IN_JOYAUTOSPRINT  = 1ULL << 17,
	IN_SHOWSCORES     = 1ULL << 33,
	IN_ZOOM           = 1ULL << 34,
	IN_LOOKATWEAPON   = 1ULL << 35
};

#undef SDK_PAD
