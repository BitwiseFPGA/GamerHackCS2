#pragma once
#include <cmath>
#include <limits>
#include <algorithm>

#include "vector.h"

// forward declarations
struct Matrix3x4;

#pragma warning(push)
#pragma warning(disable : 4201) // nameless struct/union

struct QAngle
{
	constexpr QAngle(float pitch = 0.0f, float yaw = 0.0f, float roll = 0.0f) :
		x(pitch), y(yaw), z(roll) { }

	constexpr QAngle(const float* arr) :
		x(arr[0]), y(arr[1]), z(arr[2]) { }

	// array access
	[[nodiscard]] float& operator[](int i) { return reinterpret_cast<float*>(this)[i]; }
	[[nodiscard]] const float& operator[](int i) const { return reinterpret_cast<const float*>(this)[i]; }

	// relational
	[[nodiscard]] bool IsEqual(const QAngle& a, float flEpsilon = std::numeric_limits<float>::epsilon()) const
	{
		return (std::fabsf(x - a.x) < flEpsilon && std::fabsf(y - a.y) < flEpsilon && std::fabsf(z - a.z) < flEpsilon);
	}

	bool operator==(const QAngle& a) const { return IsEqual(a); }
	bool operator!=(const QAngle& a) const { return !IsEqual(a); }

	// arithmetic assignment (angle)
	constexpr QAngle& operator+=(const QAngle& a) { x += a.x; y += a.y; z += a.z; return *this; }
	constexpr QAngle& operator-=(const QAngle& a) { x -= a.x; y -= a.y; z -= a.z; return *this; }
	constexpr QAngle& operator*=(const QAngle& a) { x *= a.x; y *= a.y; z *= a.z; return *this; }
	constexpr QAngle& operator/=(const QAngle& a) { x /= a.x; y /= a.y; z /= a.z; return *this; }

	// arithmetic assignment (scalar)
	constexpr QAngle& operator+=(float f) { x += f; y += f; z += f; return *this; }
	constexpr QAngle& operator-=(float f) { x -= f; y -= f; z -= f; return *this; }
	constexpr QAngle& operator*=(float f) { x *= f; y *= f; z *= f; return *this; }
	constexpr QAngle& operator/=(float f) { x /= f; y /= f; z /= f; return *this; }

	// unary
	constexpr QAngle operator-() const { return { -x, -y, -z }; }

	// arithmetic (angle)
	constexpr QAngle operator+(const QAngle& a) const { return { x + a.x, y + a.y, z + a.z }; }
	constexpr QAngle operator-(const QAngle& a) const { return { x - a.x, y - a.y, z - a.z }; }
	constexpr QAngle operator*(const QAngle& a) const { return { x * a.x, y * a.y, z * a.z }; }
	constexpr QAngle operator/(const QAngle& a) const { return { x / a.x, y / a.y, z / a.z }; }

	// arithmetic (scalar)
	constexpr QAngle operator+(float f) const { return { x + f, y + f, z + f }; }
	constexpr QAngle operator-(float f) const { return { x - f, y - f, z - f }; }
	constexpr QAngle operator*(float f) const { return { x * f, y * f, z * f }; }
	constexpr QAngle operator/(float f) const { return { x / f, y / f, z / f }; }

	[[nodiscard]] bool IsValid() const
	{
		return std::isfinite(x) && std::isfinite(y) && std::isfinite(z);
	}

	[[nodiscard]] bool IsZero() const
	{
		return (x == 0.0f && y == 0.0f && z == 0.0f);
	}

	[[nodiscard]] float Length() const
	{
		return std::sqrtf(x * x + y * y + z * z);
	}

	[[nodiscard]] float Length2D() const
	{
		return std::sqrtf(x * x + y * y);
	}

	/// clamp components to valid Source 2 ranges
	constexpr QAngle& Clamp()
	{
		x = std::clamp(x, -89.0f, 89.0f);
		y = std::clamp(y, -180.0f, 180.0f);
		z = std::clamp(z, -50.0f, 50.0f);
		return *this;
	}

	/// map angles to [-180, 180] range
	QAngle& Normalize()
	{
		x = std::remainderf(x, 360.0f);
		y = std::remainderf(y, 360.0f);
		z = std::remainderf(z, 360.0f);
		return *this;
	}

	/// @returns: normalized copy
	[[nodiscard]] QAngle Normalized() const
	{
		QAngle out = *this;
		out.Normalize();
		return out;
	}

	/// convert angle to direction vectors (implemented in math module)
	/// @param[out] pForward optional forward direction vector
	/// @param[out] pRight optional right direction vector
	/// @param[out] pUp optional up direction vector
	void Forward(Vector3* pForward, Vector3* pRight = nullptr, Vector3* pUp = nullptr) const;

	/// @returns: matrix converted from this angle
	[[nodiscard]] Matrix3x4 ToMatrix(const Vector3& vecOrigin = {}) const;

	union
	{
		struct { float x, y, z; };
		struct { float pitch, yaw, roll; };
	};
};

#pragma warning(pop)
