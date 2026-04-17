#pragma once

// used: [d3d] api
#include <d3d11.h>
#include <dxgi.h>

// used: CBaseHookObject
#include "../utilities/detourhook.h"

// used: ViewMatrix
#include "../sdk/datatypes/matrix.h"

// used: MENU namespace
#include "menu.h"

// forward declarations
class CCSGOInput;
class CUserCmd;
class CViewSetup;

// ---------------------------------------------------------------
// VTable indices for hooking
// ---------------------------------------------------------------
namespace VTABLE
{
	namespace D3D
	{
		enum
		{
			PRESENT = 8U,
			RESIZEBUFFERS = 13U,
		};
	}

	namespace CLIENT
	{
		enum
		{
			MOUSEINPUTENABLED = 19U,
			CREATEMOVE = 21U,
			FRAMESTAGENOTIFY = 36U,
		};
	}
}

// ---------------------------------------------------------------
// Hook system — MinHook-based detour hooking
// ---------------------------------------------------------------
namespace H
{
	bool Setup();
	void Destroy();

	/* @section: D3D11/DXGI handlers */
	HRESULT __stdcall Present(IDXGISwapChain* pSwapChain, UINT uSyncInterval, UINT uFlags);
	HRESULT __stdcall ResizeBuffers(IDXGISwapChain* pSwapChain, UINT nBufferCount, UINT nWidth, UINT nHeight, DXGI_FORMAT newFormat, UINT nFlags);
	HRESULT WINAPI   CreateSwapChain(IDXGIFactory* pFactory, IUnknown* pDevice, DXGI_SWAP_CHAIN_DESC* pDesc, IDXGISwapChain** ppSwapChain);

	/* @section: game handlers */
	bool __fastcall CreateMove(CCSGOInput* pInput, uint32_t nSlot, char a3);
	void __fastcall FrameStageNotify(void* pThis, int nFrameStage);
	bool __fastcall MouseInputEnabled(void* pThis);
	void __fastcall IsRelativeMouseMode(void* pInputSystem, bool bActive);
	// Andromeda signature: 6 params + void return. Missing 6th param caused random crashes.
	void __fastcall GetMatrixForView(void* pRenderGameSystem, void* pViewRender, ViewMatrix* pOutWorldToView, ViewMatrix* pOutViewToProjection, ViewMatrix* pOutWorldToProjection, ViewMatrix* pOutWorldToPixels);
	void __fastcall OverrideView(void* pClientModeCSNormal, void* pViewSetup);
	void* __fastcall LevelInit(void* pClientModeShared, const char* szNewMap);
	void* __fastcall LevelShutdown(void* pClientModeShared);

	/* @section: WndProc handler */
	long CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

	/* @section: hook objects */
	inline CBaseHookObject<decltype(&Present)> hkPresent = {};
	inline CBaseHookObject<decltype(&ResizeBuffers)> hkResizeBuffers = {};
	inline CBaseHookObject<decltype(&CreateSwapChain)> hkCreateSwapChain = {};

	inline CBaseHookObject<decltype(&CreateMove)> hkCreateMove = {};
	inline CBaseHookObject<decltype(&FrameStageNotify)> hkFrameStageNotify = {};
	inline CBaseHookObject<decltype(&MouseInputEnabled)> hkMouseInputEnabled = {};
	inline CBaseHookObject<decltype(&IsRelativeMouseMode)> hkIsRelativeMouseMode = {};
	inline CBaseHookObject<decltype(&GetMatrixForView)> hkGetMatrixForView = {};
	inline CBaseHookObject<decltype(&OverrideView)> hkOverrideView = {};
	inline CBaseHookObject<decltype(&LevelInit)> hkLevelInit = {};
	inline CBaseHookObject<decltype(&LevelShutdown)> hkLevelShutdown = {};

	// game's desired relative mouse mode state (saved for restoration on menu close)
	inline bool bGameRelativeMouseActive = true;
}

// ---------------------------------------------------------------
// Shared global state (accessible from hooks + render + features)
// ---------------------------------------------------------------
namespace SDK
{
	inline ViewMatrix ViewMatrix = {};
}
