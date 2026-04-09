#include "math.h"
#include <cmath>

// ============================================================================
// angle normalization
// ============================================================================

float MATH::NormalizeAngle(float flAngle)
{
	// fast path for already-normal angles
	if (flAngle >= -180.f && flAngle <= 180.f)
		return flAngle;

	// use remainder for large angles
	flAngle = std::fmod(flAngle + 180.f, 360.f);
	if (flAngle < 0.f)
		flAngle += 360.f;
	return flAngle - 180.f;
}

QAngle MATH::ClampAngles(const QAngle& angAngles)
{
	QAngle out;
	out.x = Clamp(NormalizeAngle(angAngles.x), -89.f, 89.f);
	out.y = NormalizeAngle(angAngles.y);
	out.z = 0.f;
	return out;
}

// ============================================================================
// angle <-> vector conversions
// ============================================================================

void MATH::AngleVectors(const QAngle& angAngles, Vector3* pForward, Vector3* pRight, Vector3* pUp)
{
	const float flPitchRad = angAngles.x * DEG2RAD;
	const float flYawRad   = angAngles.y * DEG2RAD;
	const float flRollRad  = angAngles.z * DEG2RAD;

	const float sp = std::sin(flPitchRad);
	const float cp = std::cos(flPitchRad);
	const float sy = std::sin(flYawRad);
	const float cy = std::cos(flYawRad);
	const float sr = std::sin(flRollRad);
	const float cr = std::cos(flRollRad);

	if (pForward != nullptr)
	{
		pForward->x = cp * cy;
		pForward->y = cp * sy;
		pForward->z = -sp;
	}

	if (pRight != nullptr)
	{
		pRight->x = -sr * sp * cy + cr * sy;
		pRight->y = -sr * sp * sy - cr * cy;
		pRight->z = -sr * cp;
	}

	if (pUp != nullptr)
	{
		pUp->x = cr * sp * cy + sr * sy;
		pUp->y = cr * sp * sy - sr * cy;
		pUp->z = cr * cp;
	}
}

QAngle MATH::VectorAngles(const Vector3& vecForward)
{
	QAngle angResult;

	if (vecForward.x == 0.f && vecForward.y == 0.f)
	{
		// looking straight up or down
		angResult.x = (vecForward.z > 0.f) ? -90.f : 90.f;
		angResult.y = 0.f;
	}
	else
	{
		angResult.x = std::atan2(-vecForward.z, vecForward.Length2D()) * RAD2DEG;
		angResult.y = std::atan2(vecForward.y, vecForward.x) * RAD2DEG;
	}

	angResult.z = 0.f;
	return angResult;
}

QAngle MATH::CalcAngle(const Vector3& vecSrc, const Vector3& vecDst)
{
	const Vector3 vecDelta = vecDst - vecSrc;
	return VectorAngles(vecDelta);
}

// ============================================================================
// aiming
// ============================================================================

float MATH::GetFOV(const QAngle& angViewAngle, const QAngle& angAimAngle)
{
	// compute forward vectors for both angles
	Vector3 vecViewFwd, vecAimFwd;
	AngleVectors(angViewAngle, &vecViewFwd);
	AngleVectors(angAimAngle, &vecAimFwd);

	// dot product gives cosine of angle between them
	float flDot = vecViewFwd.Dot(vecAimFwd);
	flDot = Clamp(flDot, -1.f, 1.f);

	return std::acos(flDot) * RAD2DEG;
}

QAngle MATH::SmoothAngle(const QAngle& angSrc, const QAngle& angDst, float flFactor)
{
	if (flFactor <= 0.f)
		return angSrc;

	QAngle angDelta;
	angDelta.x = NormalizeAngle(angDst.x - angSrc.x);
	angDelta.y = NormalizeAngle(angDst.y - angSrc.y);
	angDelta.z = 0.f;

	QAngle angResult;
	angResult.x = angSrc.x + angDelta.x / flFactor;
	angResult.y = angSrc.y + angDelta.y / flFactor;
	angResult.z = 0.f;

	return ClampAngles(angResult);
}

// ============================================================================
// projection
// ============================================================================

bool MATH::WorldToScreen(const Vector3& vecWorld, Vector2& vecScreen,
	const ViewMatrix_t& matView, int nScreenWidth, int nScreenHeight)
{
	// multiply world position by view-projection matrix
	const float w = matView[3][0] * vecWorld.x +
	                matView[3][1] * vecWorld.y +
	                matView[3][2] * vecWorld.z +
	                matView[3][3];

	// behind camera
	if (w < 0.001f)
		return false;

	const float flInvW = 1.0f / w;

	const float x = matView[0][0] * vecWorld.x +
	                matView[0][1] * vecWorld.y +
	                matView[0][2] * vecWorld.z +
	                matView[0][3];

	const float y = matView[1][0] * vecWorld.x +
	                matView[1][1] * vecWorld.y +
	                matView[1][2] * vecWorld.z +
	                matView[1][3];

	// convert to screen coordinates
	vecScreen.x = (static_cast<float>(nScreenWidth) * 0.5f) + (x * flInvW) * (static_cast<float>(nScreenWidth) * 0.5f);
	vecScreen.y = (static_cast<float>(nScreenHeight) * 0.5f) - (y * flInvW) * (static_cast<float>(nScreenHeight) * 0.5f);

	return true;
}
