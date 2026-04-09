#pragma once
#include <cstdint>
#include <limits>
#include <xmmintrin.h>

#include "vector.h"

// forward declarations
struct QAngle;

#pragma pack(push, 4)

struct Matrix3x4
{
	Matrix3x4() = default;

	constexpr Matrix3x4(
		float m00, float m01, float m02, float m03,
		float m10, float m11, float m12, float m13,
		float m20, float m21, float m22, float m23)
	{
		arrData[0][0] = m00; arrData[0][1] = m01; arrData[0][2] = m02; arrData[0][3] = m03;
		arrData[1][0] = m10; arrData[1][1] = m11; arrData[1][2] = m12; arrData[1][3] = m13;
		arrData[2][0] = m20; arrData[2][1] = m21; arrData[2][2] = m22; arrData[2][3] = m23;
	}

	constexpr Matrix3x4(const Vector3& vecForward, const Vector3& vecLeft, const Vector3& vecUp, const Vector3& vecOrigin)
	{
		SetForward(vecForward);
		SetLeft(vecLeft);
		SetUp(vecUp);
		SetOrigin(vecOrigin);
	}

	[[nodiscard]] float* operator[](int i) { return arrData[i]; }
	[[nodiscard]] const float* operator[](int i) const { return arrData[i]; }

	// column setters (columns = forward/left/up/origin)
	constexpr void SetForward(const Vector3& v) { arrData[0][0] = v.x; arrData[1][0] = v.y; arrData[2][0] = v.z; }
	constexpr void SetLeft(const Vector3& v)    { arrData[0][1] = v.x; arrData[1][1] = v.y; arrData[2][1] = v.z; }
	constexpr void SetUp(const Vector3& v)      { arrData[0][2] = v.x; arrData[1][2] = v.y; arrData[2][2] = v.z; }
	constexpr void SetOrigin(const Vector3& v)  { arrData[0][3] = v.x; arrData[1][3] = v.y; arrData[2][3] = v.z; }

	// column getters
	[[nodiscard]] constexpr Vector3 GetForward() const { return { arrData[0][0], arrData[1][0], arrData[2][0] }; }
	[[nodiscard]] constexpr Vector3 GetLeft() const    { return { arrData[0][1], arrData[1][1], arrData[2][1] }; }
	[[nodiscard]] constexpr Vector3 GetUp() const      { return { arrData[0][2], arrData[1][2], arrData[2][2] }; }
	[[nodiscard]] constexpr Vector3 GetOrigin() const  { return { arrData[0][3], arrData[1][3], arrData[2][3] }; }

	// generic column access
	constexpr void SetColumn(int col, const Vector3& v)
	{
		arrData[0][col] = v.x;
		arrData[1][col] = v.y;
		arrData[2][col] = v.z;
	}

	[[nodiscard]] constexpr Vector3 GetColumn(int col) const
	{
		return { arrData[0][col], arrData[1][col], arrData[2][col] };
	}

	constexpr void Invalidate()
	{
		for (auto& row : arrData)
			for (auto& v : row)
				v = std::numeric_limits<float>::infinity();
	}

	/// concatenate two 3x4 transformations
	[[nodiscard]] constexpr Matrix3x4 ConcatTransforms(const Matrix3x4& o) const
	{
		return {
			arrData[0][0] * o.arrData[0][0] + arrData[0][1] * o.arrData[1][0] + arrData[0][2] * o.arrData[2][0],
			arrData[0][0] * o.arrData[0][1] + arrData[0][1] * o.arrData[1][1] + arrData[0][2] * o.arrData[2][1],
			arrData[0][0] * o.arrData[0][2] + arrData[0][1] * o.arrData[1][2] + arrData[0][2] * o.arrData[2][2],
			arrData[0][0] * o.arrData[0][3] + arrData[0][1] * o.arrData[1][3] + arrData[0][2] * o.arrData[2][3] + arrData[0][3],

			arrData[1][0] * o.arrData[0][0] + arrData[1][1] * o.arrData[1][0] + arrData[1][2] * o.arrData[2][0],
			arrData[1][0] * o.arrData[0][1] + arrData[1][1] * o.arrData[1][1] + arrData[1][2] * o.arrData[2][1],
			arrData[1][0] * o.arrData[0][2] + arrData[1][1] * o.arrData[1][2] + arrData[1][2] * o.arrData[2][2],
			arrData[1][0] * o.arrData[0][3] + arrData[1][1] * o.arrData[1][3] + arrData[1][2] * o.arrData[2][3] + arrData[1][3],

			arrData[2][0] * o.arrData[0][0] + arrData[2][1] * o.arrData[1][0] + arrData[2][2] * o.arrData[2][0],
			arrData[2][0] * o.arrData[0][1] + arrData[2][1] * o.arrData[1][1] + arrData[2][2] * o.arrData[2][1],
			arrData[2][0] * o.arrData[0][2] + arrData[2][1] * o.arrData[1][2] + arrData[2][2] * o.arrData[2][2],
			arrData[2][0] * o.arrData[0][3] + arrData[2][1] * o.arrData[1][3] + arrData[2][2] * o.arrData[2][3] + arrData[2][3]
		};
	}

	/// @returns: euler angles extracted from this transformation matrix (implemented externally)
	[[nodiscard]] QAngle ToAngles() const;

	float arrData[3][4] = {};
};

#pragma pack(pop)

class alignas(16) Matrix3x4a : public Matrix3x4
{
public:
	Matrix3x4a() = default;

	constexpr Matrix3x4a(
		float m00, float m01, float m02, float m03,
		float m10, float m11, float m12, float m13,
		float m20, float m21, float m22, float m23)
	{
		arrData[0][0] = m00; arrData[0][1] = m01; arrData[0][2] = m02; arrData[0][3] = m03;
		arrData[1][0] = m10; arrData[1][1] = m11; arrData[1][2] = m12; arrData[1][3] = m13;
		arrData[2][0] = m20; arrData[2][1] = m21; arrData[2][2] = m22; arrData[2][3] = m23;
	}

	constexpr Matrix3x4a(const Matrix3x4& src)
	{
		for (int r = 0; r < 3; ++r)
			for (int c = 0; c < 4; ++c)
				arrData[r][c] = src.arrData[r][c];
	}

	constexpr Matrix3x4a& operator=(const Matrix3x4& src)
	{
		for (int r = 0; r < 3; ++r)
			for (int c = 0; c < 4; ++c)
				arrData[r][c] = src.arrData[r][c];
		return *this;
	}

	/// SIMD-accelerated concat for aligned matrices
	[[nodiscard]] Matrix3x4a ConcatTransforms(const Matrix3x4a& o) const
	{
		Matrix3x4a out;

		__m128 row0 = _mm_load_ps(arrData[0]);
		__m128 row1 = _mm_load_ps(arrData[1]);
		__m128 row2 = _mm_load_ps(arrData[2]);

		__m128 oRow0 = _mm_load_ps(o.arrData[0]);
		__m128 oRow1 = _mm_load_ps(o.arrData[1]);
		__m128 oRow2 = _mm_load_ps(o.arrData[2]);

		auto mulRow = [&](const __m128& r) -> __m128
		{
			__m128 res = _mm_mul_ps(_mm_shuffle_ps(r, r, _MM_SHUFFLE(0, 0, 0, 0)), oRow0);
			res = _mm_add_ps(res, _mm_mul_ps(_mm_shuffle_ps(r, r, _MM_SHUFFLE(1, 1, 1, 1)), oRow1));
			res = _mm_add_ps(res, _mm_mul_ps(_mm_shuffle_ps(r, r, _MM_SHUFFLE(2, 2, 2, 2)), oRow2));
			// add translation component (w element)
			constexpr std::uint32_t mask[4] = { 0, 0, 0, 0xFFFFFFFF };
			res = _mm_add_ps(res, _mm_and_ps(r, _mm_load_ps(reinterpret_cast<const float*>(mask))));
			return res;
		};

		_mm_store_ps(out.arrData[0], mulRow(row0));
		_mm_store_ps(out.arrData[1], mulRow(row1));
		_mm_store_ps(out.arrData[2], mulRow(row2));
		return out;
	}
};

static_assert(alignof(Matrix3x4a) == 16);

#pragma pack(push, 4)

/// 4x4 matrix used for world-to-screen (view/projection matrix)
struct ViewMatrix
{
	ViewMatrix() = default;

	constexpr ViewMatrix(
		float m00, float m01, float m02, float m03,
		float m10, float m11, float m12, float m13,
		float m20, float m21, float m22, float m23,
		float m30, float m31, float m32, float m33)
	{
		arrData[0][0] = m00; arrData[0][1] = m01; arrData[0][2] = m02; arrData[0][3] = m03;
		arrData[1][0] = m10; arrData[1][1] = m11; arrData[1][2] = m12; arrData[1][3] = m13;
		arrData[2][0] = m20; arrData[2][1] = m21; arrData[2][2] = m22; arrData[2][3] = m23;
		arrData[3][0] = m30; arrData[3][1] = m31; arrData[3][2] = m32; arrData[3][3] = m33;
	}

	constexpr ViewMatrix(const Matrix3x4& m3x4, const Vector4& extraRow = {})
	{
		for (int r = 0; r < 3; ++r)
			for (int c = 0; c < 4; ++c)
				arrData[r][c] = m3x4.arrData[r][c];

		arrData[3][0] = extraRow.x;
		arrData[3][1] = extraRow.y;
		arrData[3][2] = extraRow.z;
		arrData[3][3] = extraRow.w;
	}

	[[nodiscard]] float* operator[](int i) { return arrData[i]; }
	[[nodiscard]] const float* operator[](int i) const { return arrData[i]; }

	bool operator==(const ViewMatrix& o) const
	{
		for (int r = 0; r < 4; ++r)
			for (int c = 0; c < 4; ++c)
				if (arrData[r][c] != o.arrData[r][c])
					return false;
		return true;
	}

	bool operator!=(const ViewMatrix& o) const { return !(*this == o); }

	constexpr ViewMatrix& operator+=(const ViewMatrix& o)
	{
		for (int r = 0; r < 4; ++r)
			for (int c = 0; c < 4; ++c)
				arrData[r][c] += o.arrData[r][c];
		return *this;
	}

	constexpr ViewMatrix& operator-=(const ViewMatrix& o)
	{
		for (int r = 0; r < 4; ++r)
			for (int c = 0; c < 4; ++c)
				arrData[r][c] -= o.arrData[r][c];
		return *this;
	}

	constexpr ViewMatrix& operator*=(float f)
	{
		for (int r = 0; r < 4; ++r)
			for (int c = 0; c < 4; ++c)
				arrData[r][c] *= f;
		return *this;
	}

	constexpr ViewMatrix operator+(const ViewMatrix& o) const { ViewMatrix m = *this; m += o; return m; }
	constexpr ViewMatrix operator-(const ViewMatrix& o) const { ViewMatrix m = *this; m -= o; return m; }
	constexpr ViewMatrix operator*(float f) const { ViewMatrix m = *this; m *= f; return m; }

	[[nodiscard]] constexpr Vector4 GetRow(int i) const
	{
		return { arrData[i][0], arrData[i][1], arrData[i][2], arrData[i][3] };
	}

	[[nodiscard]] constexpr Vector4 GetColumn(int i) const
	{
		return { arrData[0][i], arrData[1][i], arrData[2][i], arrData[3][i] };
	}

	constexpr void SetColumn(int col, const Vector4& v)
	{
		arrData[0][col] = v.x;
		arrData[1][col] = v.y;
		arrData[2][col] = v.z;
		arrData[3][col] = v.w;
	}

	constexpr void Identity()
	{
		for (int r = 0; r < 4; ++r)
			for (int c = 0; c < 4; ++c)
				arrData[r][c] = (r == c) ? 1.0f : 0.0f;
	}

	[[nodiscard]] constexpr ViewMatrix Transposed() const
	{
		return {
			arrData[0][0], arrData[1][0], arrData[2][0], arrData[3][0],
			arrData[0][1], arrData[1][1], arrData[2][1], arrData[3][1],
			arrData[0][2], arrData[1][2], arrData[2][2], arrData[3][2],
			arrData[0][3], arrData[1][3], arrData[2][3], arrData[3][3]
		};
	}

	[[nodiscard]] const Matrix3x4& As3x4() const { return *reinterpret_cast<const Matrix3x4*>(this); }
	[[nodiscard]] Matrix3x4& As3x4() { return *reinterpret_cast<Matrix3x4*>(this); }

	/// full 4x4 matrix multiply
	[[nodiscard]] constexpr ViewMatrix ConcatTransforms(const ViewMatrix& o) const
	{
		ViewMatrix out;
		for (int r = 0; r < 4; ++r)
			for (int c = 0; c < 4; ++c)
				out.arrData[r][c] = arrData[r][0] * o.arrData[0][c] + arrData[r][1] * o.arrData[1][c] +
				                    arrData[r][2] * o.arrData[2][c] + arrData[r][3] * o.arrData[3][c];
		return out;
	}

	float arrData[4][4] = {};
};

#pragma pack(pop)

/// free-function 4x4 matrix multiply
inline ViewMatrix MatrixMultiply(const ViewMatrix& a, const ViewMatrix& b)
{
	return a.ConcatTransforms(b);
}

/// free-function 3x4 matrix multiply
inline Matrix3x4 MatrixMultiply(const Matrix3x4& a, const Matrix3x4& b)
{
	return a.ConcatTransforms(b);
}

/// bone transform: position (row 0) + quaternion rotation (row 1) packed into 2x4
#pragma warning(push)
#pragma warning(disable : 4201) // nameless struct/union
struct Matrix2x4
{
	[[nodiscard]] Matrix3x4 ToMatrix3x4() const
	{
		Matrix3x4 m;
		Vector4 q = { _21, _22, _23, _24 };
		Vector3 pos = { _11, _12, _13 };

		m[0][0] = 1.0f - 2.0f * q.y * q.y - 2.0f * q.z * q.z;
		m[1][0] = 2.0f * q.x * q.y + 2.0f * q.w * q.z;
		m[2][0] = 2.0f * q.x * q.z - 2.0f * q.w * q.y;

		m[0][1] = 2.0f * q.x * q.y - 2.0f * q.w * q.z;
		m[1][1] = 1.0f - 2.0f * q.x * q.x - 2.0f * q.z * q.z;
		m[2][1] = 2.0f * q.y * q.z + 2.0f * q.w * q.x;

		m[0][2] = 2.0f * q.x * q.z + 2.0f * q.w * q.y;
		m[1][2] = 2.0f * q.y * q.z - 2.0f * q.w * q.x;
		m[2][2] = 1.0f - 2.0f * q.x * q.x - 2.0f * q.y * q.y;

		m[0][3] = pos.x;
		m[1][3] = pos.y;
		m[2][3] = pos.z;

		return m;
	}

	[[nodiscard]] Vector3 GetOrigin(int idx) const
	{
		return { this[idx]._11, this[idx]._12, this[idx]._13 };
	}

	void SetOrigin(int idx, const Vector3& v)
	{
		this[idx]._11 = v.x;
		this[idx]._12 = v.y;
		this[idx]._13 = v.z;
	}

	[[nodiscard]] Vector4 GetRotation(int idx) const
	{
		return { this[idx]._21, this[idx]._22, this[idx]._23, this[idx]._24 };
	}

	union
	{
		struct { float _11, _12, _13, _14, _21, _22, _23, _24; };
	};
};
#pragma warning(pop)
