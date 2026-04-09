#include "render.h"

// used: L_PRINT
#include "log.h"
#include "xorstr.h"

// used: input system
#include "input.h"

// used: ViewMatrix for world-to-screen
#include "../sdk/datatypes/matrix.h"

// used: [ext] ImGui (fetched via CMake FetchContent)
#include <imgui.h>
#include <imgui_impl_dx11.h>
#include <imgui_impl_win32.h>

// ImGui Win32 WndProc handler (declared in imgui_impl_win32.h)
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// used: MENU::bIsOpen, SDK::ViewMatrix
#include "../core/hooks.h"

// ---------------------------------------------------------------
// Render stack internals
// ---------------------------------------------------------------
static std::vector<D::RenderStack::RenderCommand> vecBackBuffer;
static std::vector<D::RenderStack::RenderCommand> vecFrontBuffer;
static std::mutex mtxRenderStack;

// ---------------------------------------------------------------
// Setup — initialize ImGui with D3D11 + Win32 backends
// ---------------------------------------------------------------
bool D::Setup(HWND hWnd, ID3D11Device* pDev, ID3D11DeviceContext* pCtx)
{
	pDevice = pDev;
	pDeviceContext = pCtx;

	// create ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();

	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;

	// configure style
	ImGui::StyleColorsDark();
	ImGuiStyle& style = ImGui::GetStyle();
	style.WindowRounding = 6.0f;
	style.FrameRounding = 4.0f;
	style.GrabRounding = 4.0f;
	style.ScrollbarRounding = 4.0f;

	// initialize backends
	if (!ImGui_ImplWin32_Init(hWnd))
	{
		L_PRINT(LOG_ERROR) << _XS("ImGui Win32 backend init failed");
		return false;
	}

	if (!ImGui_ImplDX11_Init(pDevice, pDeviceContext))
	{
		L_PRINT(LOG_ERROR) << _XS("ImGui DX11 backend init failed");
		return false;
	}

	bInitialized = true;
	L_PRINT(LOG_INFO) << _XS("render system initialized");
	return true;
}

// ---------------------------------------------------------------
// Destroy — shutdown ImGui and release render target
// ---------------------------------------------------------------
void D::Destroy()
{
	if (!bInitialized)
		return;

	ImGui_ImplDX11_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();

	DestroyRenderTarget();

	pDevice = nullptr;
	pDeviceContext = nullptr;
	bInitialized = false;

	L_PRINT(LOG_INFO) << _XS("render system destroyed");
}

// ---------------------------------------------------------------
// Render Target
// ---------------------------------------------------------------
void D::CreateRenderTarget(IDXGISwapChain* pSwapChain)
{
	if (pSwapChain == nullptr || pDevice == nullptr)
		return;

	// release old one if it exists
	DestroyRenderTarget();

	ID3D11Texture2D* pBackBuffer = nullptr;
	if (SUCCEEDED(pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer))))
	{
		pDevice->CreateRenderTargetView(pBackBuffer, nullptr, &pRenderTargetView);
		pBackBuffer->Release();
	}
}

void D::DestroyRenderTarget()
{
	if (pRenderTargetView != nullptr)
	{
		pRenderTargetView->Release();
		pRenderTargetView = nullptr;
	}
}

// ---------------------------------------------------------------
// Frame management
// ---------------------------------------------------------------
void D::BeginFrame()
{
	if (!bInitialized)
		return;

	// swap render stack at the start of each frame
	RenderStack::Swap();

	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();
}

void D::EndFrame()
{
	if (!bInitialized)
		return;

	// render queued commands from the render stack
	RenderStack::Render();

	ImGui::Render();

	if (pRenderTargetView != nullptr && pDeviceContext != nullptr)
	{
		pDeviceContext->OMSetRenderTargets(1, &pRenderTargetView, nullptr);
		ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
	}
}

// ---------------------------------------------------------------
// WndProc forwarding
// ---------------------------------------------------------------
bool D::OnWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	// process input for input system
	IPT::OnWndProc(hWnd, uMsg, wParam, lParam);

	// toggle menu on INSERT key
	if (uMsg == WM_KEYDOWN && wParam == VK_INSERT)
	{
		MENU::bIsOpen = !MENU::bIsOpen;
	}

	// when menu is open, forward to ImGui
	if (MENU::bIsOpen)
	{
		ImGui_ImplWin32_WndProcHandler(hWnd, uMsg, wParam, lParam);

		// block game input for mouse/keyboard when menu is open
		switch (uMsg)
		{
		case WM_MOUSEMOVE:
		case WM_LBUTTONDOWN:
		case WM_LBUTTONUP:
		case WM_LBUTTONDBLCLK:
		case WM_RBUTTONDOWN:
		case WM_RBUTTONUP:
		case WM_RBUTTONDBLCLK:
		case WM_MBUTTONDOWN:
		case WM_MBUTTONUP:
		case WM_MBUTTONDBLCLK:
		case WM_XBUTTONDOWN:
		case WM_XBUTTONUP:
		case WM_XBUTTONDBLCLK:
		case WM_MOUSEWHEEL:
		case WM_MOUSEHWHEEL:
		case WM_KEYDOWN:
		case WM_KEYUP:
		case WM_SYSKEYDOWN:
		case WM_SYSKEYUP:
		case WM_CHAR:
			return true; // block game input
		}
	}

	return false;
}

// ---------------------------------------------------------------
// 2D Drawing API — uses ImGui background draw list
// ---------------------------------------------------------------
void D::DrawLine(const Vector2D& from, const Vector2D& to, const Color& color, float thickness)
{
	if (!bInitialized) return;
	ImGui::GetBackgroundDrawList()->AddLine(
		ImVec2(from.x, from.y), ImVec2(to.x, to.y),
		color.ToImU32(), thickness);
}

void D::DrawRect(const Vector2D& pos, const Vector2D& size, const Color& color, float rounding, float thickness)
{
	if (!bInitialized) return;
	ImGui::GetBackgroundDrawList()->AddRect(
		ImVec2(pos.x, pos.y), ImVec2(pos.x + size.x, pos.y + size.y),
		color.ToImU32(), rounding, ImDrawFlags_None, thickness);
}

void D::DrawRectFilled(const Vector2D& pos, const Vector2D& size, const Color& color, float rounding)
{
	if (!bInitialized) return;
	ImGui::GetBackgroundDrawList()->AddRectFilled(
		ImVec2(pos.x, pos.y), ImVec2(pos.x + size.x, pos.y + size.y),
		color.ToImU32(), rounding);
}

void D::DrawCircle(const Vector2D& center, float radius, const Color& color, int segments, float thickness)
{
	if (!bInitialized) return;
	ImGui::GetBackgroundDrawList()->AddCircle(
		ImVec2(center.x, center.y), radius,
		color.ToImU32(), segments, thickness);
}

void D::DrawCircleFilled(const Vector2D& center, float radius, const Color& color, int segments)
{
	if (!bInitialized) return;
	ImGui::GetBackgroundDrawList()->AddCircleFilled(
		ImVec2(center.x, center.y), radius,
		color.ToImU32(), segments);
}

void D::DrawText(const Vector2D& pos, const Color& color, const char* szText, bool bCentered)
{
	if (!bInitialized || szText == nullptr) return;

	ImVec2 vecPos(pos.x, pos.y);

	if (bCentered)
	{
		const ImVec2 vecTextSize = ImGui::CalcTextSize(szText);
		vecPos.x -= vecTextSize.x * 0.5f;
		vecPos.y -= vecTextSize.y * 0.5f;
	}

	// drop shadow for readability
	ImGui::GetBackgroundDrawList()->AddText(
		ImVec2(vecPos.x + 1.0f, vecPos.y + 1.0f),
		Color(0, 0, 0, static_cast<int>(static_cast<float>(color.a) * 0.75f)).ToImU32(), szText);

	ImGui::GetBackgroundDrawList()->AddText(
		vecPos, color.ToImU32(), szText);
}

void D::DrawCornerBox(const Vector2D& pos, const Vector2D& size, const Color& color, float thickness, float cornerLength)
{
	if (!bInitialized) return;

	const float flCornerW = size.x * cornerLength;
	const float flCornerH = size.y * cornerLength;

	auto* pDrawList = ImGui::GetBackgroundDrawList();
	const ImU32 col = color.ToImU32();

	const float x = pos.x;
	const float y = pos.y;
	const float w = size.x;
	const float h = size.y;

	// top-left
	pDrawList->AddLine(ImVec2(x, y), ImVec2(x + flCornerW, y), col, thickness);
	pDrawList->AddLine(ImVec2(x, y), ImVec2(x, y + flCornerH), col, thickness);

	// top-right
	pDrawList->AddLine(ImVec2(x + w, y), ImVec2(x + w - flCornerW, y), col, thickness);
	pDrawList->AddLine(ImVec2(x + w, y), ImVec2(x + w, y + flCornerH), col, thickness);

	// bottom-left
	pDrawList->AddLine(ImVec2(x, y + h), ImVec2(x + flCornerW, y + h), col, thickness);
	pDrawList->AddLine(ImVec2(x, y + h), ImVec2(x, y + h - flCornerH), col, thickness);

	// bottom-right
	pDrawList->AddLine(ImVec2(x + w, y + h), ImVec2(x + w - flCornerW, y + h), col, thickness);
	pDrawList->AddLine(ImVec2(x + w, y + h), ImVec2(x + w, y + h - flCornerH), col, thickness);
}

void D::DrawHealthBar(const Vector2D& pos, float height, int health, int maxHealth)
{
	if (!bInitialized) return;

	constexpr float BAR_WIDTH = 4.0f;
	const float flHealthFraction = static_cast<float>(health) / static_cast<float>(maxHealth);
	const float flBarHeight = height * flHealthFraction;
	const float flOffset = height - flBarHeight;

	// background
	DrawRectFilled(Vector2D(pos.x - BAR_WIDTH - 2.0f, pos.y - 1.0f),
		Vector2D(BAR_WIDTH + 2.0f, height + 2.0f), Color(0, 0, 0, 180));

	// health color: green at 100, yellow at 50, red at 0
	const std::uint8_t r = static_cast<std::uint8_t>((1.0f - flHealthFraction) * 255.0f);
	const std::uint8_t g = static_cast<std::uint8_t>(flHealthFraction * 255.0f);
	const Color colHealth(r, g, static_cast<std::uint8_t>(0), static_cast<std::uint8_t>(255));

	DrawRectFilled(Vector2D(pos.x - BAR_WIDTH - 1.0f, pos.y + flOffset),
		Vector2D(BAR_WIDTH, flBarHeight), colHealth);
}

// ---------------------------------------------------------------
// World-to-Screen
// ---------------------------------------------------------------
bool D::WorldToScreen(const Vector3& vecWorld, Vector2D* pScreenOut)
{
	const auto& mat = SDK::ViewMatrix;

	const float w = mat.arrData[3][0] * vecWorld.x + mat.arrData[3][1] * vecWorld.y +
		mat.arrData[3][2] * vecWorld.z + mat.arrData[3][3];

	if (w < 0.001f)
		return false;

	const float flInvW = 1.0f / w;
	const float x = mat.arrData[0][0] * vecWorld.x + mat.arrData[0][1] * vecWorld.y +
		mat.arrData[0][2] * vecWorld.z + mat.arrData[0][3];
	const float y = mat.arrData[1][0] * vecWorld.x + mat.arrData[1][1] * vecWorld.y +
		mat.arrData[1][2] * vecWorld.z + mat.arrData[1][3];

	const ImVec2 vecDisplaySize = ImGui::GetIO().DisplaySize;

	pScreenOut->x = (vecDisplaySize.x * 0.5f) + (x * flInvW) * (vecDisplaySize.x * 0.5f);
	pScreenOut->y = (vecDisplaySize.y * 0.5f) - (y * flInvW) * (vecDisplaySize.y * 0.5f);

	return true;
}

// ---------------------------------------------------------------
// 3D Drawing helpers
// ---------------------------------------------------------------
void D::DrawLine3D(const Vector3& from, const Vector3& to, const Color& color, float thickness)
{
	Vector2D screenFrom, screenTo;
	if (WorldToScreen(from, &screenFrom) && WorldToScreen(to, &screenTo))
		DrawLine(screenFrom, screenTo, color, thickness);
}

void D::DrawCircle3D(const Vector3& center, float radius, const Color& color, int segments)
{
	constexpr float PI = 3.14159265358979323846f;
	const float flStep = (2.0f * PI) / static_cast<float>(segments);

	Vector2D prevScreen;
	bool bPrevValid = false;

	for (int i = 0; i <= segments; i++)
	{
		const float flAngle = static_cast<float>(i) * flStep;
		const Vector3 vecPoint(
			center.x + radius * std::cosf(flAngle),
			center.y + radius * std::sinf(flAngle),
			center.z);

		Vector2D curScreen;
		const bool bCurValid = WorldToScreen(vecPoint, &curScreen);

		if (bCurValid && bPrevValid)
			DrawLine(prevScreen, curScreen, color, 1.0f);

		prevScreen = curScreen;
		bPrevValid = bCurValid;
	}
}

// ---------------------------------------------------------------
// Render Stack — double-buffered command queue
// ---------------------------------------------------------------
void D::RenderStack::Push(const RenderCommand& cmd)
{
	std::lock_guard<std::mutex> lock(mtxRenderStack);
	vecBackBuffer.push_back(cmd);
}

void D::RenderStack::Swap()
{
	std::lock_guard<std::mutex> lock(mtxRenderStack);
	vecFrontBuffer.swap(vecBackBuffer);
	vecBackBuffer.clear();
}

void D::RenderStack::Render()
{
	for (const auto& cmd : vecFrontBuffer)
	{
		switch (cmd.type)
		{
		case RenderCommand::LINE:
			D::DrawLine(cmd.pos1, cmd.pos2, cmd.color, cmd.thickness);
			break;
		case RenderCommand::RECT:
			D::DrawRect(cmd.pos1, cmd.pos2, cmd.color, cmd.extra, cmd.thickness);
			break;
		case RenderCommand::RECT_FILLED:
			D::DrawRectFilled(cmd.pos1, cmd.pos2, cmd.color, cmd.extra);
			break;
		case RenderCommand::CIRCLE:
			D::DrawCircle(cmd.pos1, cmd.extra, cmd.color, cmd.segments, cmd.thickness);
			break;
		case RenderCommand::CIRCLE_FILLED:
			D::DrawCircleFilled(cmd.pos1, cmd.extra, cmd.color, cmd.segments);
			break;
		case RenderCommand::TEXT:
			D::DrawText(cmd.pos1, cmd.color, cmd.text.c_str(), cmd.centered);
			break;
		case RenderCommand::CORNER_BOX:
			D::DrawCornerBox(cmd.pos1, cmd.pos2, cmd.color, cmd.thickness, cmd.extra);
			break;
		case RenderCommand::HEALTH_BAR:
			D::DrawHealthBar(cmd.pos1, cmd.pos2.y, cmd.health, cmd.maxHealth);
			break;
		}
	}
}

void D::RenderStack::Clear()
{
	std::lock_guard<std::mutex> lock(mtxRenderStack);
	vecFrontBuffer.clear();
	vecBackBuffer.clear();
}
