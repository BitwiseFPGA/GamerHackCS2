#pragma once
#include <cmath>
#include <cstdint>
#include <algorithm>

// SDK types — single source of truth for Vector3, Vector2D, QAngle
#include "../sdk/datatypes/vector.h"
#include "../sdk/datatypes/vector2.h"
#include "../sdk/datatypes/qangle.h"

// ViewMatrix: raw 4x4 used for world-to-screen (separate from SDK ViewMatrix struct)
using ViewMatrix_t = float[4][4];

namespace MATH
{
	// ========================================================================
	// constants
	// ========================================================================
	inline constexpr float PI       = 3.14159265358979323846f;
	inline constexpr float PI_2     = PI * 2.0f;
	inline constexpr float PI_HALF  = PI / 2.0f;
	inline constexpr float DEG2RAD  = PI / 180.0f;
	inline constexpr float RAD2DEG  = 180.0f / PI;

	// ========================================================================
	// interpolation
	// ========================================================================

	/// linear interpolation: a + (b - a) * t
	[[nodiscard]] inline constexpr float Lerp(float a, float b, float t) noexcept
	{
		return a + (b - a) * t;
	}

	/// inverse lerp: how far 'value' is between a and b [0..1]
	[[nodiscard]] inline constexpr float InverseLerp(float a, float b, float value) noexcept
	{
		if (b - a == 0.f) return 0.f;
		return (value - a) / (b - a);
	}

	/// remap value from [inMin..inMax] to [outMin..outMax]
	[[nodiscard]] inline constexpr float Remap(float value, float inMin, float inMax, float outMin, float outMax) noexcept
	{
		return Lerp(outMin, outMax, InverseLerp(inMin, inMax, value));
	}

	/// clamp value between min and max
	template <typename T>
	[[nodiscard]] inline constexpr T Clamp(T val, T min, T max) noexcept
	{
		if (val < min) return min;
		if (val > max) return max;
		return val;
	}

	// ========================================================================
	// angle normalization
	// ========================================================================

	/// normalize a single angle to [-180, 180]
	[[nodiscard]] float NormalizeAngle(float flAngle);

	/// clamp all three angle components to valid ranges
	/// pitch: [-89, 89], yaw: [-180, 180], roll: 0
	[[nodiscard]] QAngle ClampAngles(const QAngle& angAngles);

	// ========================================================================
	// angle <-> vector conversions
	// ========================================================================

	/// convert euler angles (degrees) to forward/right/up direction vectors
	void AngleVectors(const QAngle& angAngles, Vector3* pForward, Vector3* pRight = nullptr, Vector3* pUp = nullptr);

	/// convert forward direction vector to euler angles (degrees)
	[[nodiscard]] QAngle VectorAngles(const Vector3& vecForward);

	/// calculate the angle from source to destination
	[[nodiscard]] QAngle CalcAngle(const Vector3& vecSrc, const Vector3& vecDst);

	// ========================================================================
	// aiming
	// ========================================================================

	/// calculate FOV (angular distance) between two angles
	[[nodiscard]] float GetFOV(const QAngle& angViewAngle, const QAngle& angAimAngle);

	/// smooth angle transition from src toward dst by factor [0..1 = instant, higher = slower]
	[[nodiscard]] QAngle SmoothAngle(const QAngle& angSrc, const QAngle& angDst, float flFactor);

	// ========================================================================
	// projection
	// ========================================================================

	/// project 3D world position to 2D screen coordinates
	/// @param[in] vecWorld 3D world position
	/// @param[out] vecScreen output 2D screen position
	/// @param[in] matView 4x4 view-projection matrix
	/// @param[in] nScreenWidth screen width in pixels
	/// @param[in] nScreenHeight screen height in pixels
	/// @returns: true if the point is in front of the camera
	bool WorldToScreen(const Vector3& vecWorld, Vector2& vecScreen,
		const ViewMatrix_t& matView, int nScreenWidth, int nScreenHeight);
}
