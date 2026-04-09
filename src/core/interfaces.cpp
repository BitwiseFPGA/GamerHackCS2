#include "interfaces.h"
#include "patterns.h"
#include "../utilities/xorstr.h"
#include "../sdk/interfaces/iswapchain.h"

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
	{
		"CGameTraceManager",
		PATTERNS::MODULES::CLIENT,
		PATTERNS::FUNCTIONS::GAME_TRACE_MANAGER,
		MEM::ESearchType::PTR,
		0, 0,
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

	// D3D11 Device and DeviceContext are resolved lazily in the Present hook callback.
	// The ISwapChainDx11 struct layout may be wrong, so we don't attempt to dereference it here.
	// The Present hook (via gameoverlayrenderer64.dll or vtable) receives IDXGISwapChain* directly.

	// ---- entity system from game resource service ----

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
