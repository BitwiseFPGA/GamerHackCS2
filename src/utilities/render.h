#pragma once

// used: [d3d] api
#include <d3d11.h>

// used: std::string, std::vector, std::mutex
#include <string>
#include <vector>
#include <mutex>

// used: Color, Vector2D, Vector3
#include "../sdk/datatypes/color.h"
#include "../sdk/datatypes/vector.h"

/*
 * DRAW / RENDER SYSTEM
 * - ImGui backend for D3D11 overlay rendering
 * - double-buffered render command stack for thread safety
 * - immediate-mode drawing API via ImGui draw lists
 */
namespace D
{
	/* @section: lifecycle */
	/// initialize ImGui, D3D11 backend, fonts, styles
	bool Setup(HWND hWnd, ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	/// shutdown ImGui and release resources
	void Destroy();

	/* @section: frame management (called from Present hook) */
	void BeginFrame();
	void EndFrame();

	/* @section: render target management */
	void CreateRenderTarget(IDXGISwapChain* pSwapChain);
	void DestroyRenderTarget();

	/* @section: WndProc forwarding */
	/// forward window messages to ImGui
	/// @returns: true if ImGui consumed the message (block game input)
	bool OnWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

	/* @section: 2D drawing API (use between BeginFrame/EndFrame) */
	void DrawLine(const Vector2D& from, const Vector2D& to, const Color& color, float thickness = 1.0f);
	void DrawRect(const Vector2D& pos, const Vector2D& size, const Color& color, float rounding = 0.0f, float thickness = 1.0f);
	void DrawRectFilled(const Vector2D& pos, const Vector2D& size, const Color& color, float rounding = 0.0f);
	void DrawCircle(const Vector2D& center, float radius, const Color& color, int segments = 32, float thickness = 1.0f);
	void DrawCircleFilled(const Vector2D& center, float radius, const Color& color, int segments = 32);
	void DrawText(const Vector2D& pos, const Color& color, const char* szText, bool bCentered = false);
	void DrawCornerBox(const Vector2D& pos, const Vector2D& size, const Color& color, float thickness = 1.0f, float cornerLength = 0.25f);
	void DrawHealthBar(const Vector2D& pos, float height, int health, int maxHealth = 100);

	/* @section: 3D to 2D projection helpers */
	void DrawLine3D(const Vector3& from, const Vector3& to, const Color& color, float thickness = 1.0f);
	void DrawCircle3D(const Vector3& center, float radius, const Color& color, int segments = 32);

	/* @section: world-to-screen */
	/// project 3D world position to 2D screen coordinates
	/// @param[out] pScreenOut receives the screen position
	/// @returns: true if the point is on-screen
	bool WorldToScreen(const Vector3& vecWorld, Vector2D* pScreenOut);

	/* @section: render command stack (thread-safe deferred rendering) */
	namespace RenderStack
	{
		struct RenderCommand
		{
			enum EType : std::uint8_t
			{
				LINE,
				RECT,
				RECT_FILLED,
				CIRCLE,
				CIRCLE_FILLED,
				TEXT,
				CORNER_BOX,
				HEALTH_BAR
			};

			EType type = LINE;
			Vector2D pos1 = {};
			Vector2D pos2 = {};
			Color color = {};
			float thickness = 1.0f;
			float extra = 0.0f;   // radius, rounding, cornerLength, etc.
			std::string text = {};
			bool centered = false;
			int segments = 32;
			int health = 100;
			int maxHealth = 100;
		};

		/// add a command to the back buffer (game thread)
		void Push(const RenderCommand& cmd);
		/// swap back buffer to front buffer (call at start of present)
		void Swap();
		/// render all commands from front buffer
		void Render();
		/// clear both buffers
		void Clear();
	}

	/* @section: state */
	inline bool bInitialized = false;
	inline ID3D11Device* pDevice = nullptr;
	inline ID3D11DeviceContext* pDeviceContext = nullptr;
	inline ID3D11RenderTargetView* pRenderTargetView = nullptr;
}
