#include "hooks.h"
#include "patterns.h"
#include "interfaces.h"
#include "../sdk/interfaces/iswapchain.h"
#include "../utilities/xorstr.h"

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
	// Strategy: try multiple approaches to hook Present
	// 1. rendersystemdx11.dll SWAP_CHAIN_HOOK pattern (vtable hook)
	// 2. gameoverlayrenderer64.dll Present wrapper (direct detour — most reliable)

	bool bD3DHooked = false;

	// Approach 1: rendersystemdx11.dll vtable hook via SWAP_CHAIN_HOOK pattern
	{
		HMODULE hRenderSystem = MEM::GetModuleBaseHandle(PATTERNS::MODULES::RENDERSYSTEM);
		if (hRenderSystem)
		{
			std::uintptr_t uSwapChainAddr = MEM::FindPattern(PATTERNS::MODULES::RENDERSYSTEM, PATTERNS::FUNCTIONS::SWAP_CHAIN_HOOK, MEM::ESearchType::PTR, 0);
			if (uSwapChainAddr != 0)
			{
				void* pSwapChainObj = *reinterpret_cast<void**>(uSwapChainAddr);
				if (pSwapChainObj)
				{
					IDXGISwapChain* pDXGISwapChain = *reinterpret_cast<IDXGISwapChain**>(
						reinterpret_cast<std::uintptr_t>(pSwapChainObj) + PATTERNS::OFFSETS::SWAP_CHAIN_DXGI);
					if (pDXGISwapChain)
					{
						if (hkPresent.Create(reinterpret_cast<void*>(MEM::GetVFunc(pDXGISwapChain, PATTERNS::VTABLE::D3D::PRESENT)), reinterpret_cast<void*>(&Present)))
						{
							L_PRINT(LOG_INFO) << _XS("\"Present\" hook created (via SWAP_CHAIN_HOOK)");
							hkResizeBuffers.Create(reinterpret_cast<void*>(MEM::GetVFunc(pDXGISwapChain, PATTERNS::VTABLE::D3D::RESIZE_BUFFERS)), reinterpret_cast<void*>(&ResizeBuffers));
							L_PRINT(LOG_INFO) << _XS("\"ResizeBuffers\" hook created");
							bD3DHooked = true;
						}
					}
				}
			}
		}
	}

	// Approach 2: hook Present directly in Steam overlay (gameoverlayrenderer64.dll)
	if (!bD3DHooked)
	{
		HMODULE hOverlay = MEM::GetModuleBaseHandle(PATTERNS::MODULES::GAMEOVERLAY);
		if (!hOverlay)
		{
			L_PRINT(LOG_INFO) << _XS("waiting for gameoverlayrenderer64.dll...");
			for (int i = 0; i < 100 && !hOverlay; ++i)
			{
				Sleep(100);
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
					L_PRINT(LOG_INFO) << _XS("\"Present\" hook created (via gameoverlayrenderer64.dll)");
					bD3DHooked = true;
				}
				else
				{
					L_PRINT(LOG_WARNING) << _XS("failed to create Present hook in gameoverlayrenderer64.dll");
				}
			}
			else
			{
				L_PRINT(LOG_WARNING) << _XS("Present pattern not found in gameoverlayrenderer64.dll");
			}
		}
		else
		{
			L_PRINT(LOG_WARNING) << _XS("gameoverlayrenderer64.dll not loaded (Steam overlay disabled?)");
		}
	}

	if (!bD3DHooked)
		L_PRINT(LOG_WARNING) << _XS("D3D hooks NOT installed — no swap chain or overlay found");

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

	// FrameStageNotify and MouseInputEnabled require interface pointers
	// These are hooked via VTable once interfaces are resolved
	// Example: hkFrameStageNotify.Create(MEM::GetVFunc(I::Client, PATTERNS::VTABLE::CLIENT::FRAME_STAGE_NOTIFY), ...)
	// Example: hkMouseInputEnabled.Create(MEM::GetVFunc(I::Input, PATTERNS::VTABLE::CLIENT::MOUSE_INPUT_ENABLED), ...)
	// TODO: hook these once interface system is set up

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
					D::Setup(desc.OutputWindow, pDevice, pContext);
					D::CreateRenderTarget(pSwapChain);
				}
			}
		}
	}

	D::BeginFrame();

	// dispatch to feature rendering
	F::OnPresent();

	// render menu
	MENU::Render();

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

// ---------------------------------------------------------------
// Game Hook Implementations
// ---------------------------------------------------------------
bool __fastcall H::CreateMove(CCSGOInput* pInput, int nSlot, CUserCmd* pCmd)
{
	const auto oCreateMove = hkCreateMove.GetOriginal();
	const bool bResult = oCreateMove(pInput, nSlot, pCmd);

	if (pCmd == nullptr)
		return bResult;

	// dispatch to features
	F::OnCreateMove(pInput, pCmd);

	return bResult;
}

void __fastcall H::FrameStageNotify(void* pThis, int nFrameStage)
{
	const auto oFrameStageNotify = hkFrameStageNotify.GetOriginal();

	F::OnFrameStageNotify(nFrameStage);

	oFrameStageNotify(pThis, nFrameStage);
}

bool __fastcall H::MouseInputEnabled(void* pThis)
{
	const auto oMouseInputEnabled = hkMouseInputEnabled.GetOriginal();

	// block game mouse input when menu is open
	if (MENU::bIsOpen)
		return false;

	return oMouseInputEnabled(pThis);
}

ViewMatrix* __fastcall H::GetMatrixForView(void* pRenderGameSystem, void* pViewRender, ViewMatrix* pOutWorldToView, ViewMatrix* pOutViewToProjection, ViewMatrix* pOutWorldToProjection)
{
	const auto oGetMatrixForView = hkGetMatrixForView.GetOriginal();
	ViewMatrix* pResult = oGetMatrixForView(pRenderGameSystem, pViewRender, pOutWorldToView, pOutViewToProjection, pOutWorldToProjection);

	// store the world-to-projection matrix for ESP world-to-screen
	SDK::ViewMatrix = *pOutWorldToProjection;

	return pResult;
}

void __fastcall H::OverrideView(void* pClientModeCSNormal, void* pViewSetup)
{
	const auto oOverrideView = hkOverrideView.GetOriginal();

	// dispatch for FOV / thirdperson modifications
	F::OnOverrideView(pViewSetup);

	oOverrideView(pClientModeCSNormal, pViewSetup);
}

void* __fastcall H::LevelInit(void* pClientModeShared, const char* szNewMap)
{
	const auto oLevelInit = hkLevelInit.GetOriginal();

	L_PRINT(LOG_INFO) << _XS("level init: ") << szNewMap;
	F::OnLevelInit(szNewMap);

	return oLevelInit(pClientModeShared, szNewMap);
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

	return ::CallWindowProcW(IPT::pOldWndProc, hWnd, uMsg, wParam, lParam);
}
