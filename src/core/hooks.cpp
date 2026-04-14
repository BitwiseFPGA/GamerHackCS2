#include "hooks.h"
#include "patterns.h"
#include "interfaces.h"
#include "../sdk/interfaces/iswapchain.h"
#include "../utilities/xorstr.h"
#include "../sdk/functionlist.h"

// used: common includes, datatypes
#include "../sdk/common.h"

// used: MEM::GetVFunc, MEM::FindPattern
#include "../utilities/memory.h"

// used: input system
#include "../utilities/input.h"

// used: drawing / ImGui
#include "../utilities/render.h"

// used: feature dispatch
#include "../features/features.h"

// used: menu rendering
#include "menu.h"

// ---------------------------------------------------------------
// Forward-defined SEH helpers (must precede Setup so they are visible at call site)
// ---------------------------------------------------------------

// SEH guard for inputsystem.dll hook creation.
// inputsystem.dll is Steam DRM-protected (VMProtect may use PAGE_GUARD pages).
// MH_CreateHook reads target bytes to build a trampoline; if those bytes span into
// a guard page, MinHook faults with no SEH wrapper. Catch and skip this non-critical hook.
static bool SafeCreateIsRelMouseHook(void* pTarget, void* pDetour)
{
	__try
	{
		return H::hkIsRelativeMouseMode.Create(pTarget, pDetour);
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		L_PRINT(LOG_WARNING) << _XS("[setup] IsRelativeMouseMode hook threw during Create — DRM guard page? Skipping");
		return false;
	}
}

// ---------------------------------------------------------------
// Hook Setup / Destroy
// ---------------------------------------------------------------
bool H::Setup()
{
	if (MH_Initialize() != MH_OK)
	{
		L_PRINT(LOG_ERROR) << _XS("failed to initialize MinHook");
		return false;
	}
	L_PRINT(LOG_INFO) << _XS("MinHook initialization completed");

	// --- D3D11/DXGI hooks ---
	// Hook via gameoverlayrenderer64.dll Present/ResizeBuffers wrappers.
	// This is the only reliable path: ISwapChainDx11 struct layout is version-dependent.
	// (Same approach used by Andromeda-CS2-Base)

	bool bD3DHooked = false;

	HMODULE hOverlay = MEM::GetModuleBaseHandle(PATTERNS::MODULES::GAMEOVERLAY);
	if (!hOverlay)
	{
		// wait a short time — overlay DLL loads very early but might not be listed yet
		for (int i = 0; i < 30 && !hOverlay; ++i)
		{
			Sleep(50);
			hOverlay = MEM::GetModuleBaseHandle(PATTERNS::MODULES::GAMEOVERLAY);
		}
	}

	if (hOverlay)
	{
		std::uintptr_t uPresent = MEM::FindPattern(PATTERNS::MODULES::GAMEOVERLAY, PATTERNS::FUNCTIONS::PRESENT_OVERLAY);
		if (uPresent)
		{
			if (hkPresent.Create(reinterpret_cast<void*>(uPresent), reinterpret_cast<void*>(&Present)))
			{
				L_PRINT(LOG_INFO) << _XS("\"Present\" hook created (gameoverlayrenderer64.dll)");
				bD3DHooked = true;
			}
			else
			{
				L_PRINT(LOG_WARNING) << _XS("MinHook failed to create Present hook");
			}
		}
		else
		{
			L_PRINT(LOG_WARNING) << _XS("Present pattern not found in gameoverlayrenderer64.dll");
		}

		// ResizeBuffers — separate pattern in overlay DLL
		std::uintptr_t uResize = MEM::FindPattern(PATTERNS::MODULES::GAMEOVERLAY, PATTERNS::FUNCTIONS::RESIZE_BUFFERS_OVERLAY);
		if (uResize)
		{
			hkResizeBuffers.Create(reinterpret_cast<void*>(uResize), reinterpret_cast<void*>(&ResizeBuffers));
			L_PRINT(LOG_INFO) << _XS("\"ResizeBuffers\" hook created (gameoverlayrenderer64.dll)");
		}
	}
	else
	{
		L_PRINT(LOG_WARNING) << _XS("gameoverlayrenderer64.dll not loaded (Steam overlay disabled?)");
	}

	// --- Fallback: hook IDXGISwapChain vtable directly ---
	// When the overlay DLL pattern fails (e.g. Steam VMProtect obfuscates the code
	// in memory), hook vtable[8]/[13] of the live IDXGISwapChain object.
	// After Steam hooks the vtable, vtable[8] already points to the overlay's wrapper —
	// so this gives us the same hook point as the pattern approach.
	// All dereferences are SEH-guarded: a bad ISwapChainDx11 layout or uninitialized
	// pDXGISwapChain field must not crash the DLL.
	if (!bD3DHooked)
	{
		L_PRINT(LOG_INFO) << _XS("[D3D] trying vtable fallback, SwapChain=")
			<< static_cast<void*>(I::SwapChain);

		IDXGISwapChain* pDXGI = nullptr;
		void** pVTable = nullptr;

		if (I::SwapChain)
		{
			__try
			{
				pDXGI = I::SwapChain->pDXGISwapChain;
			}
			__except (EXCEPTION_EXECUTE_HANDLER)
			{
				L_PRINT(LOG_WARNING) << _XS("[D3D] AV reading pDXGISwapChain — ISwapChainDx11 offset wrong?");
				pDXGI = nullptr;
			}
		}
		L_PRINT(LOG_INFO) << _XS("[D3D] pDXGISwapChain=") << static_cast<void*>(pDXGI);

		if (pDXGI)
		{
			__try
			{
				pVTable = *reinterpret_cast<void***>(pDXGI);
			}
			__except (EXCEPTION_EXECUTE_HANDLER)
			{
				L_PRINT(LOG_WARNING) << _XS("[D3D] AV reading IDXGISwapChain vtable — pDXGI corrupt?");
				pVTable = nullptr;
			}
		}
		L_PRINT(LOG_INFO) << _XS("[D3D] vtable=") << static_cast<void*>(pVTable);

		if (pVTable)
		{
			L_PRINT(LOG_INFO) << _XS("[D3D] vtable[8]=") << pVTable[8]
				<< _XS(" vtable[13]=") << pVTable[13];

			if (pVTable[8])
			{
				if (hkPresent.Create(pVTable[8], reinterpret_cast<void*>(&Present)))
				{
					L_PRINT(LOG_INFO) << _XS("\"Present\" hook created via IDXGISwapChain vtable[8]");
					bD3DHooked = true;
				}
				else
				{
					L_PRINT(LOG_WARNING) << _XS("[D3D] MinHook failed to hook vtable[8]");
				}
			}

			if (pVTable[13])
			{
				if (hkResizeBuffers.Create(pVTable[13], reinterpret_cast<void*>(&ResizeBuffers)))
					L_PRINT(LOG_INFO) << _XS("\"ResizeBuffers\" hook created via IDXGISwapChain vtable[13]");
				else
					L_PRINT(LOG_WARNING) << _XS("[D3D] MinHook failed to hook vtable[13]");
			}
		}
	}

	if (!bD3DHooked)
	{
		std::uintptr_t uCreateSwapChain = MEM::FindPattern(PATTERNS::MODULES::GAMEOVERLAY, PATTERNS::FUNCTIONS::CREATE_SWAP_CHAIN);
		if (uCreateSwapChain && hkCreateSwapChain.Create(reinterpret_cast<void*>(uCreateSwapChain), reinterpret_cast<void*>(&CreateSwapChain)))
			L_PRINT(LOG_INFO) << _XS("\"CreateSwapChain\" hook created — Present will hook lazily at swap chain creation");
		else
			L_PRINT(LOG_WARNING) << _XS("CreateSwapChain pattern also failed — D3D overlay entirely unavailable until game creates a new swap chain");
	}

	if (!bD3DHooked)
		L_PRINT(LOG_WARNING) << _XS("D3D Present hook NOT installed — menu/ESP overlay will not render");

	// --- Game hooks ---

	// @ida: #STR: cl: CreateMove clamped invalid attack history index
	if (!hkCreateMove.Create(reinterpret_cast<void*>(MEM::FindPattern(PATTERNS::MODULES::CLIENT, PATTERNS::FUNCTIONS::CREATE_MOVE)), reinterpret_cast<void*>(&CreateMove)))
		return false;
	L_PRINT(LOG_INFO) << _XS("\"CreateMove\" hook created");

	// @ida: CViewRender->OnRenderStart -> GetMatricesForView
	if (!hkGetMatrixForView.Create(reinterpret_cast<void*>(MEM::FindPattern(PATTERNS::MODULES::CLIENT, PATTERNS::FUNCTIONS::GET_MATRIX_FOR_VIEW)), reinterpret_cast<void*>(&GetMatrixForView)))
		return false;
	L_PRINT(LOG_INFO) << _XS("\"GetMatrixForView\" hook created");

	// @ida: #STR: "game_newmap" -> first function
	if (!hkLevelInit.Create(reinterpret_cast<void*>(MEM::FindPattern(PATTERNS::MODULES::CLIENT, PATTERNS::FUNCTIONS::LEVEL_INIT)), reinterpret_cast<void*>(&LevelInit)))
		return false;
	L_PRINT(LOG_INFO) << _XS("\"LevelInit\" hook created");

	// @ida: ClientModeShared -> #STR: "map_shutdown"
	if (!hkLevelShutdown.Create(reinterpret_cast<void*>(MEM::FindPattern(PATTERNS::MODULES::CLIENT, PATTERNS::FUNCTIONS::LEVEL_SHUTDOWN)), reinterpret_cast<void*>(&LevelShutdown)))
		return false;
	L_PRINT(LOG_INFO) << _XS("\"LevelShutdown\" hook created");

	// @ida: ClientModeCSNormal->OverrideView
	if (!hkOverrideView.Create(reinterpret_cast<void*>(MEM::FindPattern(PATTERNS::MODULES::CLIENT, PATTERNS::FUNCTIONS::OVERRIDE_VIEW)), reinterpret_cast<void*>(&OverrideView)))
		return false;
	L_PRINT(LOG_INFO) << _XS("\"OverrideView\" hook created");

	// --- MouseInputEnabled via pattern scan (Andromeda approach) ---
	// More reliable than vtable index which changes between game versions
	{
		std::uintptr_t uMouseInput = MEM::FindPattern(PATTERNS::MODULES::CLIENT, PATTERNS::FUNCTIONS::MOUSE_INPUT_ENABLED);
		if (uMouseInput && hkMouseInputEnabled.Create(reinterpret_cast<void*>(uMouseInput), reinterpret_cast<void*>(&MouseInputEnabled)))
			L_PRINT(LOG_INFO) << _XS("\"MouseInputEnabled\" hook created (pattern)");
		else
			L_PRINT(LOG_WARNING) << _XS("\"MouseInputEnabled\" hook FAILED");
	}

	// --- FrameStageNotify ---
	// Non-critical: visuals/misc/inventory features use this callback.
	// Without it those features are silent; game stays stable.
	{
		std::uintptr_t uFSN = MEM::FindPattern(PATTERNS::MODULES::CLIENT, PATTERNS::FUNCTIONS::FRAME_STAGE_NOTIFY);
		if (uFSN && hkFrameStageNotify.Create(reinterpret_cast<void*>(uFSN), reinterpret_cast<void*>(&FrameStageNotify)))
			L_PRINT(LOG_INFO) << _XS("\"FrameStageNotify\" hook created");
		else
			L_PRINT(LOG_WARNING) << _XS("\"FrameStageNotify\" hook FAILED — visual/misc features will not update");
	}

	// --- IsRelativeMouseMode in inputsystem.dll ---
	// Prevents SDL from keeping the mouse in relative (locked to window) mode when menu is open
	L_PRINT(LOG_INFO) << _XS("[setup] scanning inputsystem.dll for IsRelativeMouseMode...");
	{
		std::uintptr_t uRelMouse = MEM::FindPattern(PATTERNS::MODULES::INPUTSYSTEM, PATTERNS::FUNCTIONS::IS_RELATIVE_MOUSE_MODE);
		L_PRINT(LOG_INFO) << _XS("[setup] IsRelativeMouseMode scan result: ") << reinterpret_cast<void*>(uRelMouse);
		if (uRelMouse && SafeCreateIsRelMouseHook(reinterpret_cast<void*>(uRelMouse), reinterpret_cast<void*>(&IsRelativeMouseMode)))
			L_PRINT(LOG_INFO) << _XS("\"IsRelativeMouseMode\" hook created");
		else
			L_PRINT(LOG_WARNING) << _XS("\"IsRelativeMouseMode\" hook FAILED — camera may still move with menu open");
	}

	L_PRINT(LOG_INFO) << _XS("all hooks installed successfully");
	return true;
}

void H::Destroy()
{
	MH_DisableHook(MH_ALL_HOOKS);
	MH_RemoveHook(MH_ALL_HOOKS);
	MH_Uninitialize();

	L_PRINT(LOG_INFO) << _XS("all hooks removed, MinHook uninitialized");
}

// ---------------------------------------------------------------
// D3D11/DXGI Hook Implementations
// ---------------------------------------------------------------
HRESULT __stdcall H::Present(IDXGISwapChain* pSwapChain, UINT uSyncInterval, UINT uFlags)
{
	const auto oPresent = hkPresent.GetOriginal();

	static bool bLoggedFirstPresent = false;
	if (!bLoggedFirstPresent)
	{
		bLoggedFirstPresent = true;
		L_PRINT(LOG_INFO) << _XS("[D3D] first Present call, SwapChain=") << static_cast<void*>(pSwapChain);
	}

	// lazy-init the render system on first Present call
	if (!D::bInitialized)
	{
		DXGI_SWAP_CHAIN_DESC desc{};
		if (SUCCEEDED(pSwapChain->GetDesc(&desc)))
		{
			ID3D11Device* pDevice = nullptr;
			pSwapChain->GetDevice(IID_PPV_ARGS(&pDevice));
			if (pDevice)
			{
				ID3D11DeviceContext* pContext = nullptr;
				pDevice->GetImmediateContext(&pContext);
				if (pContext)
				{
					L_PRINT(LOG_INFO) << _XS("[D3D] initializing renderer");
					D::Setup(desc.OutputWindow, pDevice, pContext);
					D::CreateRenderTarget(pSwapChain);
					L_PRINT(LOG_INFO) << _XS("[D3D] renderer initialized OK");
				}
				else { L_PRINT(LOG_WARNING) << _XS("[D3D] GetImmediateContext returned null"); }
			}
			else { L_PRINT(LOG_WARNING) << _XS("[D3D] GetDevice returned null"); }
		}
		else { L_PRINT(LOG_WARNING) << _XS("[D3D] GetDesc failed"); }
	}

	D::BeginFrame();

	// SEH-protect feature rendering and menu — these run on the D3D thread
	// and access game entity state that may fault during spawn transitions.
	__try
	{
		F::OnPresent();
		MENU::Render();
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		static bool bLogged = false;
		if (!bLogged)
		{
			bLogged = true;
			L_PRINT(LOG_ERROR) << _XS("[D3D] EXCEPTION in OnPresent/Menu dispatch");
		}
	}

	D::EndFrame();

	return oPresent(pSwapChain, uSyncInterval, uFlags);
}

HRESULT __stdcall H::ResizeBuffers(IDXGISwapChain* pSwapChain, UINT nBufferCount, UINT nWidth, UINT nHeight, DXGI_FORMAT newFormat, UINT nFlags)
{
	const auto oResizeBuffers = hkResizeBuffers.GetOriginal();

	// destroy old render target before resize
	D::DestroyRenderTarget();

	HRESULT hResult = oResizeBuffers(pSwapChain, nBufferCount, nWidth, nHeight, newFormat, nFlags);
	if (SUCCEEDED(hResult))
		D::CreateRenderTarget(pSwapChain);

	return hResult;
}

// CreateSwapChain hook: fired by Steam overlay when D3D initializes the swap chain.
// This is our third fallback when both PRESENT_OVERLAY and vtable approaches fail.
// After the original creates the chain, we hook vtable[8]/[13] of the live object.
HRESULT WINAPI H::CreateSwapChain(IDXGIFactory* pFactory, IUnknown* pDevice, DXGI_SWAP_CHAIN_DESC* pDesc, IDXGISwapChain** ppSwapChain)
{
	const auto oCreateSwapChain = hkCreateSwapChain.GetOriginal();

	const HRESULT hr = oCreateSwapChain(pFactory, pDevice, pDesc, ppSwapChain);

	if (SUCCEEDED(hr) && ppSwapChain && *ppSwapChain)
	{
		if (!hkPresent.IsHooked())
		{
			IDXGISwapChain* pSC = *ppSwapChain;
			void** pVT = nullptr;
			__try { pVT = *reinterpret_cast<void***>(pSC); }
			__except (EXCEPTION_EXECUTE_HANDLER) { pVT = nullptr; }

			if (pVT)
			{
				if (pVT[VTABLE::D3D::PRESENT] &&
					hkPresent.Create(pVT[VTABLE::D3D::PRESENT], reinterpret_cast<void*>(&Present)))
					L_PRINT(LOG_INFO) << _XS("[D3D] Present hooked via CreateSwapChain vtable");

				if (pVT[VTABLE::D3D::RESIZEBUFFERS] && !hkResizeBuffers.IsHooked() &&
					hkResizeBuffers.Create(pVT[VTABLE::D3D::RESIZEBUFFERS], reinterpret_cast<void*>(&ResizeBuffers)))
					L_PRINT(LOG_INFO) << _XS("[D3D] ResizeBuffers hooked via CreateSwapChain vtable");
			}
		}
		else
		{
			// Swap chain recreated — invalidate stale render target
			D::DestroyRenderTarget();
		}
	}

	return hr;
}

// ---------------------------------------------------------------
// Game Hook Implementations
// ---------------------------------------------------------------

// SEH helper: reading pArr+0x5910 can fault if the tick-array pointer is stale.
// Must be a separate function because __try/__except cannot coexist with C++ object
// unwinding in the same function scope.
static uint32_t SafeReadSeqNum(void* pArr)
{
	__try
	{
		return *reinterpret_cast<uint32_t*>(
			reinterpret_cast<uint8_t*>(pArr) + PATTERNS::OFFSETS::USERCMD_SEQUENCE_NUMBER);
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		return 0;
	}
}

// SEH helper: acquire CUserCmd from the pattern-scanned function chain.
// Any of these SDK_FUNC calls can fault if the pattern resolved to a wrong address.
static CUserCmd* SafeAcquireCUserCmd(CCSGOInput* pInput, bool bDiag)
{
	__try
	{
		if (!I::FirstCUserCmdArray
			|| !SDK_FUNC::GetLocalPlayerController
			|| !SDK_FUNC::GetCUserCmdTick
			|| !SDK_FUNC::GetCUserCmdArray
			|| !SDK_FUNC::GetCUserCmdBySequenceNumber)
		{
			if (bDiag)
				L_PRINT(LOG_WARNING) << _XS("[CM] prerequisites missing");
			return nullptr;
		}

		CCSPlayerController* pLocal = SDK_FUNC::GetLocalPlayerController(-1);
		if (bDiag)
			L_PRINT(LOG_INFO) << _XS("[CM] pLocal=") << static_cast<void*>(pLocal)
			<< _XS(" FirstCUserCmdArray=") << I::FirstCUserCmdArray;

		if (!pLocal)
			return nullptr;

		int32_t tick = 0;
		SDK_FUNC::GetCUserCmdTick(pLocal, &tick);
		const int32_t queryTick = (tick == -1) ? -1 : (tick - 1);

		void* pArr = SDK_FUNC::GetCUserCmdArray(I::FirstCUserCmdArray, queryTick);
		if (bDiag)
			L_PRINT(LOG_INFO) << _XS("[CM] tick=") << tick
			<< _XS(" queryTick=") << queryTick
			<< _XS(" pArr=") << pArr;

		if (!pArr)
			return nullptr;

		const uint32_t seqNum = SafeReadSeqNum(pArr);
		if (!seqNum)
		{
			if (bDiag)
				L_PRINT(LOG_WARNING) << _XS("[CM] SafeReadSeqNum returned 0 (bad pArr or stale offset)");
			return nullptr;
		}

		CUserCmd* pCmd = SDK_FUNC::GetCUserCmdBySequenceNumber(pLocal, seqNum);
		if (bDiag)
			L_PRINT(LOG_INFO) << _XS("[CM] seqNum=") << seqNum
			<< _XS(" pCmd=") << static_cast<void*>(pCmd);
		return pCmd;
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		static bool bLogged = false;
		if (!bLogged) { bLogged = true; L_PRINT(LOG_ERROR) << _XS("[CM] EXCEPTION acquiring CUserCmd — pattern mismatch?"); }
		return nullptr;
	}
}

// SEH helper: dispatch feature callbacks from CreateMove.
static void SafeCallFeaturesCreateMove(CCSGOInput* pInput, CUserCmd* pCmd)
{
	__try
	{
		F::OnCreateMove(pInput, pCmd);
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		static bool bLogged = false;
		if (!bLogged) { bLogged = true; L_PRINT(LOG_ERROR) << _XS("[CM] EXCEPTION in feature dispatch"); }
	}
}

bool __fastcall H::CreateMove(CCSGOInput* pInput, uint32_t nSlot, char a3)
{
	const auto oCreateMove = hkCreateMove.GetOriginal();

	static bool bFirstCall = true;
	if (bFirstCall)
	{
		bFirstCall = false;
		L_PRINT(LOG_INFO) << _XS("[hook] CreateMove first call pInput=") << static_cast<void*>(pInput);
	}

	const bool bResult = oCreateMove(pInput, nSlot, a3);

	static int nCMFrame = 0;
	nCMFrame++;
	const bool bCMDiag = (nCMFrame <= 5) || (nCMFrame % 500 == 0);

	CUserCmd* pCmd = SafeAcquireCUserCmd(pInput, bCMDiag);

	SafeCallFeaturesCreateMove(pInput, pCmd);

	return bResult;
}

// SEH wrapper so a crash in F::OnFrameStageNotify doesn't kill the game thread.
static void SafeCallFeaturesFSN(int nFrameStage)
{
	__try
	{
		F::OnFrameStageNotify(nFrameStage);
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		// Feature dispatch faulted — swallow and continue to original
	}
}

// SEH guard for inputsystem.dll hook creation — defined before Setup(), see top of file.

// SEH helper: the original FrameStageNotify may fault on the first call after injection
// (the CS2 render thread can race against hook setup). Must be a separate function because
// __try/__except cannot coexist with C++ object unwinding in the same scope.
static void SafeCallOriginalFSN(void(__fastcall* fn)(void*, int), void* pThis, int nFrameStage)
{
	__try
	{
		fn(pThis, nFrameStage);
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		// Swallow — logged in the caller if needed
	}
}

void __fastcall H::FrameStageNotify(void* pThis, int nFrameStage)
{
	const auto oFrameStageNotify = hkFrameStageNotify.GetOriginal();

	static bool bFirstCall = true;
	if (bFirstCall)
	{
		bFirstCall = false;
		L_PRINT(LOG_INFO) << _XS("[hook] FrameStageNotify first call, stage=") << nFrameStage << _XS(" pThis=") << pThis;
	}

	SafeCallFeaturesFSN(nFrameStage);

	SafeCallOriginalFSN(oFrameStageNotify, pThis, nFrameStage);
}

bool __fastcall H::MouseInputEnabled(void* pThis)
{
	const auto oMouseInputEnabled = hkMouseInputEnabled.GetOriginal();

	// block game mouse input when menu is open
	if (MENU::bIsOpen)
		return false;

	return oMouseInputEnabled(pThis);
}

void __fastcall H::IsRelativeMouseMode(void* pInputSystem, bool bActive)
{
	const auto oIsRelativeMouseMode = hkIsRelativeMouseMode.GetOriginal();
	// save game's desired relative mouse state
	bGameRelativeMouseActive = bActive;
	// When menu is open, force mouse out of relative (locked) mode
	oIsRelativeMouseMode(pInputSystem, MENU::bIsOpen ? false : bActive);
}

void __fastcall H::GetMatrixForView(void* pRenderGameSystem, void* pViewRender, ViewMatrix* pOutWorldToView, ViewMatrix* pOutViewToProjection, ViewMatrix* pOutWorldToProjection, ViewMatrix* pOutWorldToPixels)
{
	const auto oGetMatrixForView = hkGetMatrixForView.GetOriginal();

	static bool bFirstCall = true;
	if (bFirstCall) { bFirstCall = false; L_PRINT(LOG_INFO) << _XS("[hook] GetMatrixForView first call"); }

	// Forward ALL 6 parameters to the original (Andromeda signature).
	// Previously only 5 were forwarded — the missing pWorldToPixels caused
	// the original function to read garbage from the stack and crash.
	oGetMatrixForView(pRenderGameSystem, pViewRender, pOutWorldToView, pOutViewToProjection, pOutWorldToProjection, pOutWorldToPixels);

	// store the world-to-projection matrix for ESP world-to-screen
	// null check: pointer can be null during level transitions
	if (pOutWorldToProjection)
		SDK::ViewMatrix = *pOutWorldToProjection;
}

void __fastcall H::OverrideView(void* pClientModeCSNormal, void* pViewSetup)
{
	const auto oOverrideView = hkOverrideView.GetOriginal();

	static bool bFirstCall = true;
	if (bFirstCall) { bFirstCall = false; L_PRINT(LOG_INFO) << _XS("[hook] OverrideView first call"); }

	// SEH-protect feature dispatch — if MISC::OnOverrideView faults, game continues
	__try
	{
		F::OnOverrideView(pViewSetup);
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		static bool bLogged = false;
		if (!bLogged) { bLogged = true; L_PRINT(LOG_ERROR) << _XS("[hook] OverrideView feature dispatch faulted"); }
	}

	oOverrideView(pClientModeCSNormal, pViewSetup);
}

void* __fastcall H::LevelInit(void* pClientModeShared, const char* szNewMap)
{
	const auto oLevelInit = hkLevelInit.GetOriginal();

	L_PRINT(LOG_INFO) << _XS("[hook] LevelInit called: ") << (szNewMap ? szNewMap : "null");
	F::OnLevelInit(szNewMap);

	L_PRINT(LOG_INFO) << _XS("[hook] LevelInit: calling original...");

	void* ret = oLevelInit(pClientModeShared, szNewMap);

	L_PRINT(LOG_INFO) << _XS("[hook] LevelInit: original returned");
	return ret;
}

void* __fastcall H::LevelShutdown(void* pClientModeShared)
{
	const auto oLevelShutdown = hkLevelShutdown.GetOriginal();

	L_PRINT(LOG_INFO) << _XS("level shutdown");
	F::OnLevelShutdown();

	return oLevelShutdown(pClientModeShared);
}

// ---------------------------------------------------------------
// WndProc Hook — dispatches input to ImGui and input system
// ---------------------------------------------------------------
long CALLBACK H::WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	// let the draw system handle ImGui wndproc
	if (D::OnWndProc(hWnd, uMsg, wParam, lParam))
		return 1L;

	return static_cast<long>(::CallWindowProcW(IPT::pOldWndProc, hWnd, uMsg, wParam, lParam));
}
