#include "interfaces.h"
#include "patterns.h"
#include "../utilities/xorstr.h"
#include "../sdk/interfaces/iswapchain.h"
#include "../sdk/interfaces/ipvs.h"
#include "../sdk/functionlist.h"

// ============================================================================
// pattern definitions for non-CreateInterface pointers
// ============================================================================
static MEM::PatternInfo_t s_arrPatterns[] =
{
	// GlobalVars — now in client.dll
	{
		"IGlobalVars",
		PATTERNS::MODULES::CLIENT,
		PATTERNS::FUNCTIONS::GLOBAL_VARS,
		MEM::ESearchType::PTR,
		0, 0,
		"xref 'CBaseEntity::SetParent' or GlobalVars in client.dll"
	},
	// CCSGOInput — client.dll
	{
		"CCSGOInput",
		PATTERNS::MODULES::CLIENT,
		PATTERNS::FUNCTIONS::CSGO_INPUT,
		MEM::ESearchType::PTR,
		0, 0,
		"xref 'CCSGOInput' pointer in client.dll"
	},
	// ViewMatrix — client.dll
	{
		"ViewMatrix",
		PATTERNS::MODULES::CLIENT,
		PATTERNS::FUNCTIONS::VIEW_MATRIX,
		MEM::ESearchType::PTR,
		0, 0,
		"xref view matrix in CViewRender"
	},
	// CGameTraceManager — client.dll
	// Pattern starts with 4-byte prefix D9 0F 57 C0 before the 4C 8B 2D MOV instruction;
	// nOffset=4 skips that prefix so PTR resolution resolves at the correct 7-byte MOV R13,[rip+X]
	{
		"CGameTraceManager",
		PATTERNS::MODULES::CLIENT,
		PATTERNS::FUNCTIONS::GAME_TRACE_MANAGER,
		MEM::ESearchType::PTR,
		4, 0,
		"xref CGameTraceManager in TraceShape"
	},
	// ISwapChainDx11 — rendersystemdx11.dll
	// resolves "48 89 2D" (mov [rip+disp32], rbp) — standard 3-byte opcode, PTR defaults are correct
	{
		"ISwapChainDx11",
		PATTERNS::MODULES::RENDERSYSTEM,
		PATTERNS::FUNCTIONS::SWAP_CHAIN,
		MEM::ESearchType::PTR,
		0, 0,
		"xref swap chain store in CRenderDeviceDx11"
	},
	// NOTE: FirstCUserCmdArray is resolved manually after the batch scan (see below).
	// This avoids a pattern that also matches the entity system reference.
};

// ============================================================================
// setup
// ============================================================================
bool I::Setup()
{
	L_PRINT(LOG_INFO) << _XS("--- capturing game interfaces ---");

	// ---- CreateInterface-based captures ----

	SchemaSystem = CaptureInterface<ISchemaSystem>(PATTERNS::MODULES::SCHEMASYSTEM, PATTERNS::INTERFACES::SCHEMA_SYSTEM);
	if (!SchemaSystem)
		return false;

	InputSystem = CaptureInterface<IInputSystem>(PATTERNS::MODULES::INPUTSYSTEM, PATTERNS::INTERFACES::INPUT_SYSTEM);
	if (!InputSystem)
		return false;

	Engine = CaptureInterface<IEngineClient>(PATTERNS::MODULES::ENGINE2, PATTERNS::INTERFACES::SOURCE2_ENGINE_TO_CLIENT);
	if (!Engine)
		return false;

	Client = CaptureInterface<ISource2Client>(PATTERNS::MODULES::CLIENT, PATTERNS::INTERFACES::SOURCE2_CLIENT);
	if (!Client)
		return false;

	// CVar — optional, don't fail if missing
	Cvar = CaptureInterface<IEngineCVar>(PATTERNS::MODULES::ENGINE2, PATTERNS::INTERFACES::ENGINE_CVAR);

	// Localize — optional
	Localize = CaptureInterface<CLocalize>(PATTERNS::MODULES::LOCALIZE, PATTERNS::INTERFACES::LOCALIZE);

	// MaterialSystem — optional
	MaterialSystem = CaptureInterface<CMaterialSystem2>(PATTERNS::MODULES::MATERIALSYSTEM2, PATTERNS::INTERFACES::MATERIAL_SYSTEM2);

	GameResourceService = CaptureInterface<IGameResourceService>(PATTERNS::MODULES::ENGINE2, PATTERNS::INTERFACES::GAME_RESOURCE_SERVICE);
	if (!GameResourceService)
		return false;

	// ---- pattern-scanned captures ----

	L_PRINT(LOG_INFO) << _XS("--- scanning for non-exported pointers ---");

	const std::size_t nPatternCount = sizeof(s_arrPatterns) / sizeof(s_arrPatterns[0]);
	const std::size_t nFound = MEM::ScanPatterns(s_arrPatterns, nPatternCount);

	// assign results from pattern scan — with safety checks
	L_PRINT(LOG_INFO) << _XS("--- resolving scanned pointers ---");

	if (s_arrPatterns[0].uResult)
	{
		GlobalVars = *reinterpret_cast<IGlobalVars**>(s_arrPatterns[0].uResult);
		L_PRINT(LOG_INFO) << _XS("  GlobalVars = ") << static_cast<const void*>(GlobalVars);
	}

	if (s_arrPatterns[1].uResult)
	{
		Input = *reinterpret_cast<CCSGOInput**>(s_arrPatterns[1].uResult);
		L_PRINT(LOG_INFO) << _XS("  Input = ") << static_cast<const void*>(Input);
	}

	if (s_arrPatterns[2].uResult)
	{
		ViewMatrixPtr = reinterpret_cast<ViewMatrix*>(s_arrPatterns[2].uResult);
		L_PRINT(LOG_INFO) << _XS("  ViewMatrixPtr = ") << static_cast<const void*>(ViewMatrixPtr);
	}

	if (s_arrPatterns[3].uResult)
	{
		GameTraceManager = *reinterpret_cast<CGameTraceManager**>(s_arrPatterns[3].uResult);
		L_PRINT(LOG_INFO) << _XS("  GameTraceManager = ") << static_cast<const void*>(GameTraceManager);
	}

	if (s_arrPatterns[4].uResult)
	{
		SwapChain = *reinterpret_cast<ISwapChainDx11**>(s_arrPatterns[4].uResult);
		L_PRINT(LOG_INFO) << _XS("  SwapChain = ") << static_cast<const void*>(SwapChain);
	}

	// ---- entity system from game resource service (resolve FIRST so we can validate CUserCmd) ----

	// ---- PVS (Potentially Visible Set) — model occlusion control ----
	{
		auto uPVS = MEM::FindPattern(PATTERNS::MODULES::ENGINE2, PATTERNS::FUNCTIONS::PVS, MEM::ESearchType::NONE);
		if (uPVS)
		{
			PVS = reinterpret_cast<CPVS*>(MEM::GetAbsoluteAddress(uPVS, 3, 7));
			L_PRINT(LOG_INFO) << _XS("  PVS = ") << static_cast<const void*>(PVS);
		}
		else
		{
			L_PRINT(LOG_WARNING) << _XS("  PVS pattern not found — model occlusion will remain enabled");
		}
	}

	if (GameResourceService)
	{
		__try
		{
			GameEntitySystem = *reinterpret_cast<CGameEntitySystem**>(
				reinterpret_cast<std::uintptr_t>(GameResourceService) + PATTERNS::OFFSETS::GAME_RESOURCE_ENTITY_SYSTEM);
		}
		__except (EXCEPTION_EXECUTE_HANDLER)
		{
			L_PRINT(LOG_ERROR) << _XS("SEH exception reading CGameEntitySystem — offset likely wrong");
			GameEntitySystem = nullptr;
		}

		if (GameEntitySystem)
			L_PRINT(LOG_INFO) << _XS("CGameEntitySystem = ") << static_cast<const void*>(GameEntitySystem);
		else
			L_PRINT(LOG_WARNING) << _XS("CGameEntitySystem is null (offset may need updating)");
	}

	// ---- FirstCUserCmdArray — try multiple patterns, validate against entity system ----
	{
		// candidate patterns for the CUserCmd** global (mov rcx, [rip+X] before GetCUserCmdArray call)
		static const char* arrCUserCmdPatterns[] = {
			"48 8B 0D ? ? ? ? E8 ? ? ? ? 48 8B CF 4C 8B F8",  // Andromeda disasm comment variant (mov r15, rax)
			"48 8B 0D ? ? ? ? E8 ? ? ? ? 48 8B CF 4C 8B F0",  // variant (mov r14, rax)
			"48 8B 0D ? ? ? ? E8 ? ? ? ? 48 8B CF 48 8B F0",  // original Andromeda code pattern (mov rsi, rax)
			"48 8B 0D ? ? ? ? E8 ? ? ? ? 48 8B CF 49 8B F0",  // variant (mov rsi, r8)
		};

		for (const auto* szPat : arrCUserCmdPatterns)
		{
			std::uintptr_t uMatch = MEM::FindPattern(PATTERNS::MODULES::CLIENT, szPat, MEM::ESearchType::PTR, 0);
			if (!uMatch)
				continue;

			void* pResolved = *reinterpret_cast<void**>(uMatch);
			L_PRINT(LOG_INFO) << _XS("  CUserCmd pattern '") << szPat << _XS("' -> ") << pResolved;

			// validate: must not be null and must not be the entity system pointer
			if (pResolved && pResolved != static_cast<void*>(GameEntitySystem))
			{
				FirstCUserCmdArray = pResolved;
				L_PRINT(LOG_INFO) << _XS("  FirstCUserCmdArray = ") << FirstCUserCmdArray << _XS(" (validated OK)");
				break;
			}
			else
			{
				L_PRINT(LOG_WARNING) << _XS("  CUserCmd pattern matched entity system or null, skipping");
			}
		}

		if (!FirstCUserCmdArray)
			L_PRINT(LOG_WARNING) << _XS("  FirstCUserCmdArray not found in pattern scan — will retry in PostResolve");
	}

	// ---- validation ----

	L_PRINT(LOG_INFO) << _XS("--- interface summary ---");
	L_PRINT(LOG_INFO) << _XS("  SchemaSystem:      ") << static_cast<const void*>(SchemaSystem);
	L_PRINT(LOG_INFO) << _XS("  InputSystem:       ") << static_cast<const void*>(InputSystem);
	L_PRINT(LOG_INFO) << _XS("  Engine:            ") << static_cast<const void*>(Engine);
	L_PRINT(LOG_INFO) << _XS("  Client:            ") << static_cast<const void*>(Client);
	L_PRINT(LOG_INFO) << _XS("  Cvar:              ") << static_cast<const void*>(Cvar);
	L_PRINT(LOG_INFO) << _XS("  GlobalVars:        ") << static_cast<const void*>(GlobalVars);
	L_PRINT(LOG_INFO) << _XS("  GameResourceSvc:   ") << static_cast<const void*>(GameResourceService);
	L_PRINT(LOG_INFO) << _XS("  GameEntitySystem:  ") << static_cast<const void*>(GameEntitySystem);
	L_PRINT(LOG_INFO) << _XS("  Input:             ") << static_cast<const void*>(Input);
	L_PRINT(LOG_INFO) << _XS("  SwapChain(raw):    ") << static_cast<const void*>(SwapChain);
	L_PRINT(LOG_INFO) << _XS("  GameTraceManager:  ") << static_cast<const void*>(GameTraceManager);
	L_PRINT(LOG_INFO) << _XS("  ViewMatrixPtr:     ") << static_cast<const void*>(ViewMatrixPtr);
	L_PRINT(LOG_INFO) << _XS("  Localize:          ") << static_cast<const void*>(Localize);
	L_PRINT(LOG_INFO) << _XS("  MaterialSystem:    ") << static_cast<const void*>(MaterialSystem);
	L_PRINT(LOG_INFO) << _XS("  PVS:               ") << static_cast<const void*>(PVS);
	L_PRINT(LOG_INFO) << _XS("  FirstCUserCmdArray:") << static_cast<const void*>(FirstCUserCmdArray);
	L_PRINT(LOG_INFO) << _XS("  (D3D Device/Context resolved lazily in Present hook)");

	// critical interfaces that must be present
	if (!SchemaSystem || !Engine || !Client || !GameResourceService)
	{
		L_PRINT(LOG_ERROR) << _XS("one or more critical interfaces failed to capture");
		return false;
	}

	L_PRINT(LOG_INFO) << _XS("--- interface capture complete ---");
	return true;
}

// ============================================================================
// PostResolve — called after SDK_FUNC::Initialize() to resolve remaining ptrs
// ============================================================================
void I::PostResolve()
{
	if (FirstCUserCmdArray)
	{
		L_PRINT(LOG_INFO) << _XS("[PostResolve] FirstCUserCmdArray already resolved: ") << FirstCUserCmdArray;
		return;
	}

	L_PRINT(LOG_INFO) << _XS("[PostResolve] attempting GetCUserCmdTick body extraction...");

	// fallback: extract from GetCUserCmdTick function body
	// GetCUserCmdTick contains: 4C 8B 0D xx xx xx xx (mov r9, [rip+X]) somewhere in the prolog.
	// Scan first 32 bytes to handle any prolog variant (SUB RSP + optional PUSH regs).
	if (SDK_FUNC::GetCUserCmdTick)
	{
		__try
		{
			std::uintptr_t uFuncAddr = reinterpret_cast<std::uintptr_t>(SDK_FUNC::GetCUserCmdTick);
			bool bFound = false;
			for (int nOff = 0; nOff < 32 && !bFound; ++nOff)
			{
				if (*reinterpret_cast<std::uint8_t*>(uFuncAddr + nOff)     == 0x4C &&
					*reinterpret_cast<std::uint8_t*>(uFuncAddr + nOff + 1) == 0x8B &&
					*reinterpret_cast<std::uint8_t*>(uFuncAddr + nOff + 2) == 0x0D)
				{
					bFound = true;
					std::uintptr_t uPtrAddr = MEM::GetAbsoluteAddress(uFuncAddr + nOff, 3, 7);
					void* pResolved = *reinterpret_cast<void**>(uPtrAddr);
					L_PRINT(LOG_INFO) << _XS("  GetCUserCmdTick 4C 8B 0D found at +") << nOff
						<< _XS(", body ptr = ") << pResolved;
					if (pResolved && pResolved != static_cast<void*>(GameEntitySystem))
					{
						FirstCUserCmdArray = pResolved;
						L_PRINT(LOG_INFO) << _XS("  FirstCUserCmdArray = ") << FirstCUserCmdArray
							<< _XS(" (from GetCUserCmdTick body)");
					}
					else
					{
						L_PRINT(LOG_WARNING) << _XS("  GetCUserCmdTick body ptr matches entity system or null");
					}
				}
			}
			if (!bFound)
			{
				L_PRINT(LOG_WARNING) << _XS("  4C 8B 0D not found in first 32 bytes of GetCUserCmdTick");
				for (int i = 0; i < 16; i++)
				{
					L_PRINT(LOG_INFO) << _XS("  byte[") << i << _XS("] = 0x")
						<< std::hex << static_cast<int>(*reinterpret_cast<std::uint8_t*>(uFuncAddr + i));
				}
			}
		}
		__except (EXCEPTION_EXECUTE_HANDLER)
		{
			L_PRINT(LOG_ERROR) << _XS("  SEH exception extracting CUserCmd from GetCUserCmdTick");
		}
	}
	else
	{
		L_PRINT(LOG_WARNING) << _XS("  GetCUserCmdTick not resolved — no fallback available");
	}

	if (!FirstCUserCmdArray)
		L_PRINT(LOG_WARNING) << _XS("[PostResolve] FirstCUserCmdArray FAILED — aimbot will use SetViewAngle fallback");
}

// ============================================================================
// destroy
// ============================================================================
void I::Destroy()
{
	// release D3D COM references we acquired
	if (DeviceContext)
	{
		DeviceContext->Release();
		DeviceContext = nullptr;
	}

	if (Device)
	{
		Device->Release();
		Device = nullptr;
	}

	// null all pointers (game owns the memory, we don't free)
	SchemaSystem     = nullptr;
	InputSystem      = nullptr;
	Engine           = nullptr;
	Client           = nullptr;
	Cvar             = nullptr;
	GlobalVars       = nullptr;
	GameResourceService = nullptr;
	GameEntitySystem = nullptr;
	Input            = nullptr;
	SwapChain        = nullptr;
	GameTraceManager = nullptr;
	ViewMatrixPtr    = nullptr;
	Localize         = nullptr;
	FileSystem       = nullptr;
	MaterialSystem   = nullptr;
	EconItemSystem   = nullptr;

	L_PRINT(LOG_INFO) << _XS("interfaces destroyed");
}
