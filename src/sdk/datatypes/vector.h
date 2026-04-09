#pragma once
#include <cmath>
#include <limits>
#include <cstdint>

// forward declarations
struct QAngle;
struct Matrix3x4;

struct Vector2D
{
	constexpr Vector2D(float x = 0.0f, float y = 0.0f) :
		x(x), y(y) { }

	constexpr Vector2D(const float* arr) :
		x(arr[0]), y(arr[1]) { }

	// array access
	[[nodiscard]] float& operator[](int i) { return reinterpret_cast<float*>(this)[i]; }
	[[nodiscard]] const float& operator[](int i) const { return reinterpret_cast<const float*>(this)[i]; }

	// relational
	bool operator==(const Vector2D& v) const { return (std::fabsf(x - v.x) < std::numeric_limits<float>::epsilon() && std::fabsf(y - v.y) < std::numeric_limits<float>::epsilon()); }
	bool operator!=(const Vector2D& v) const { return !(*this == v); }

	// arithmetic assignment
	constexpr Vector2D& operator+=(const Vector2D& v) { x += v.x; y += v.y; return *this; }
	constexpr Vector2D& operator-=(const Vector2D& v) { x -= v.x; y -= v.y; return *this; }
	constexpr Vector2D& operator*=(const Vector2D& v) { x *= v.x; y *= v.y; return *this; }
	constexpr Vector2D& operator/=(const Vector2D& v) { x /= v.x; y /= v.y; return *this; }
	constexpr Vector2D& operator+=(float f) { x += f; y += f; return *this; }
	constexpr Vector2D& operator-=(float f) { x -= f; y -= f; return *this; }
	constexpr Vector2D& operator*=(float f) { x *= f; y *= f; return *this; }
	constexpr Vector2D& operator/=(float f) { x /= f; y /= f; return *this; }

	// unary
	constexpr Vector2D operator-() const { return { -x, -y }; }

	// arithmetic
	constexpr Vector2D operator+(const Vector2D& v) const { return { x + v.x, y + v.y }; }
	constexpr Vector2D operator-(const Vector2D& v) const { return { x - v.x, y - v.y }; }
	constexpr Vector2D operator*(const Vector2D& v) const { return { x * v.x, y * v.y }; }
	constexpr Vector2D operator/(const Vector2D& v) const { return { x / v.x, y / v.y }; }
	constexpr Vector2D operator+(float f) const { return { x + f, y + f }; }
	constexpr Vector2D operator-(float f) const { return { x - f, y - f }; }
	constexpr Vector2D operator*(float f) const { return { x * f, y * f }; }
	constexpr Vector2D operator/(float f) const { return { x / f, y / f }; }

	[[nodiscard]] bool IsValid() const { return std::isfinite(x) && std::isfinite(y); }
	[[nodiscard]] bool IsZero() const { return (x == 0.0f && y == 0.0f); }

	[[nodiscard]] float Length() const { return std::sqrtf(LengthSqr()); }
	[[nodiscard]] constexpr float LengthSqr() const { return x * x + y * y; }

	[[nodiscard]] constexpr float Dot(const Vector2D& v) const { return x * v.x + y * v.y; }

	float NormalizeInPlace()
	{
		const float len = Length();
		const float inv = 1.0f / (len + std::numeric_limits<float>::epsilon());
		x *= inv;
		y *= inv;
		return len;
	}

	[[nodiscard]] Vector2D Normalized() const
	{
		Vector2D out = *this;
		out.NormalizeInPlace();
		return out;
	}

	float x, y;
};

struct Vector3
{
	constexpr Vector3(float x = 0.0f, float y = 0.0f, float z = 0.0f) :
		x(x), y(y), z(z) { }

	constexpr Vector3(const float* arr) :
		x(arr[0]), y(arr[1]), z(arr[2]) { }

	constexpr Vector3(const Vector2D& v2) :
		x(v2.x), y(v2.y), z(0.0f) { }

	// array access
	[[nodiscard]] float& operator[](int i) { return reinterpret_cast<float*>(this)[i]; }
	[[nodiscard]] const float& operator[](int i) const { return reinterpret_cast<const float*>(this)[i]; }

	// relational
	[[nodiscard]] bool IsEqual(const Vector3& v, float flEpsilon = std::numeric_limits<float>::epsilon()) const
	{
		return (std::fabsf(x - v.x) < flEpsilon && std::fabsf(y - v.y) < flEpsilon && std::fabsf(z - v.z) < flEpsilon);
	}

	bool operator==(const Vector3& v) const { return IsEqual(v); }
	bool operator!=(const Vector3& v) const { return !IsEqual(v); }

	// assignment
	constexpr Vector3& operator=(const Vector2D& v2) { x = v2.x; y = v2.y; z = 0.0f; return *this; }

	// arithmetic assignment (vector)
	constexpr Vector3& operator+=(const Vector3& v) { x += v.x; y += v.y; z += v.z; return *this; }
	constexpr Vector3& operator-=(const Vector3& v) { x -= v.x; y -= v.y; z -= v.z; return *this; }
	constexpr Vector3& operator*=(const Vector3& v) { x *= v.x; y *= v.y; z *= v.z; return *this; }
	constexpr Vector3& operator/=(const Vector3& v) { x /= v.x; y /= v.y; z /= v.z; return *this; }

	// arithmetic assignment (scalar)
	constexpr Vector3& operator+=(float f) { x += f; y += f; z += f; return *this; }
	constexpr Vector3& operator-=(float f) { x -= f; y -= f; z -= f; return *this; }
	constexpr Vector3& operator*=(float f) { x *= f; y *= f; z *= f; return *this; }
	constexpr Vector3& operator/=(float f) { x /= f; y /= f; z /= f; return *this; }

	// unary
	constexpr Vector3 operator-() const { return { -x, -y, -z }; }

	// arithmetic (vector)
	constexpr Vector3 operator+(const Vector3& v) const { return { x + v.x, y + v.y, z + v.z }; }
	constexpr Vector3 operator-(const Vector3& v) const { return { x - v.x, y - v.y, z - v.z }; }
	constexpr Vector3 operator*(const Vector3& v) const { return { x * v.x, y * v.y, z * v.z }; }
	constexpr Vector3 operator/(const Vector3& v) const { return { x / v.x, y / v.y, z / v.z }; }

	// arithmetic (scalar)
	constexpr Vector3 operator+(float f) const { return { x + f, y + f, z + f }; }
	constexpr Vector3 operator-(float f) const { return { x - f, y - f, z - f }; }
	constexpr Vector3 operator*(float f) const { return { x * f, y * f, z * f }; }
	constexpr Vector3 operator/(float f) const { return { x / f, y / f, z / f }; }

	[[nodiscard]] bool IsValid() const { return std::isfinite(x) && std::isfinite(y) && std::isfinite(z); }
	[[nodiscard]] bool IsZero() const { return (x == 0.0f && y == 0.0f && z == 0.0f); }

	constexpr void Invalidate() { x = y = z = std::numeric_limits<float>::infinity(); }

	[[nodiscard]] float Length() const { return std::sqrtf(LengthSqr()); }
	[[nodiscard]] constexpr float LengthSqr() const { return x * x + y * y + z * z; }
	[[nodiscard]] float Length2D() const { return std::sqrtf(Length2DSqr()); }
	[[nodiscard]] constexpr float Length2DSqr() const { return x * x + y * y; }

	[[nodiscard]] float DistTo(const Vector3& v) const { return (*this - v).Length(); }
	[[nodiscard]] constexpr float DistToSqr(const Vector3& v) const { return (*this - v).LengthSqr(); }

	[[nodiscard]] constexpr float Dot(const Vector3& v) const { return x * v.x + y * v.y + z * v.z; }

	[[nodiscard]] constexpr Vector3 Cross(const Vector3& v) const
	{
		return { y * v.z - z * v.y, z * v.x - x * v.z, x * v.y - y * v.x };
	}

	float NormalizeInPlace()
	{
		const float len = Length();
		const float inv = 1.0f / (len + std::numeric_limits<float>::epsilon());
		x *= inv;
		y *= inv;
		z *= inv;
		return len;
	}

	[[nodiscard]] Vector3 Normalized() const
	{
		Vector3 out = *this;
		out.NormalizeInPlace();
		return out;
	}

	/// @returns: euler angles from direction vector (implemented externally in math module)
	[[nodiscard]] QAngle ToAngle() const;

	/// @returns: transformed vector by given transformation matrix
	[[nodiscard]] Vector3 Transform(const Matrix3x4& mat) const;

	[[nodiscard]] Vector2D ToVector2D() const { return { x, y }; }

	float x, y, z;
};

// scalar * vector
inline constexpr Vector3 operator*(float f, const Vector3& v) { return v * f; }
inline constexpr Vector2D operator*(float f, const Vector2D& v) { return v * f; }

struct Vector4
{
	constexpr Vector4(float x = 0.0f, float y = 0.0f, float z = 0.0f, float w = 0.0f) :
		x(x), y(y), z(z), w(w) { }

	// array access
	[[nodiscard]] float& operator[](int i) { return reinterpret_cast<float*>(this)[i]; }
	[[nodiscard]] const float& operator[](int i) const { return reinterpret_cast<const float*>(this)[i]; }

	// relational
	bool operator==(const Vector4& v) const { return (x == v.x && y == v.y && z == v.z && w == v.w); }
	bool operator!=(const Vector4& v) const { return !(*this == v); }

	// arithmetic assignment
	constexpr Vector4& operator+=(const Vector4& v) { x += v.x; y += v.y; z += v.z; w += v.w; return *this; }
	constexpr Vector4& operator-=(const Vector4& v) { x -= v.x; y -= v.y; z -= v.z; w -= v.w; return *this; }
	constexpr Vector4& operator*=(float f) { x *= f; y *= f; z *= f; w *= f; return *this; }
	constexpr Vector4& operator/=(float f) { x /= f; y /= f; z /= f; w /= f; return *this; }

	// unary
	constexpr Vector4 operator-() const { return { -x, -y, -z, -w }; }

	// arithmetic
	constexpr Vector4 operator+(const Vector4& v) const { return { x + v.x, y + v.y, z + v.z, w + v.w }; }
	constexpr Vector4 operator-(const Vector4& v) const { return { x - v.x, y - v.y, z - v.z, w - v.w }; }
	constexpr Vector4 operator*(float f) const { return { x * f, y * f, z * f, w * f }; }
	constexpr Vector4 operator/(float f) const { return { x / f, y / f, z / f, w / f }; }

	[[nodiscard]] bool IsValid() const { return std::isfinite(x) && std::isfinite(y) && std::isfinite(z) && std::isfinite(w); }
	[[nodiscard]] bool IsZero() const { return (x == 0.0f && y == 0.0f && z == 0.0f && w == 0.0f); }

	[[nodiscard]] float Length() const { return std::sqrtf(LengthSqr()); }
	[[nodiscard]] constexpr float LengthSqr() const { return x * x + y * y + z * z + w * w; }
	[[nodiscard]] constexpr float Dot(const Vector4& v) const { return x * v.x + y * v.y + z * v.z + w * v.w; }

	float x, y, z, w;
};

struct alignas(16) VectorAligned : Vector3
{
	VectorAligned() = default;

	explicit VectorAligned(const Vector3& v)
	{
		x = v.x;
		y = v.y;
		z = v.z;
		w = 0.0f;
	}

	constexpr VectorAligned& operator=(const Vector3& v)
	{
		x = v.x;
		y = v.y;
		z = v.z;
		w = 0.0f;
		return *this;
	}

	float w = 0.0f;
};

static_assert(alignof(VectorAligned) == 16);
