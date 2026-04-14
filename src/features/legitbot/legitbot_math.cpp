#include "legitbot_math.h"

#include <cmath>
#include <numbers>

QAngle F::LEGITBOT::MATH::CalcAngle(const Vector3& vecSrc, const Vector3& vecDst)
{
	Vector3 delta = vecDst - vecSrc;
	float hyp = std::sqrtf(delta.x * delta.x + delta.y * delta.y);

	QAngle angles;
	angles.x = -std::atan2f(delta.z, hyp) * (180.0f / std::numbers::pi_v<float>);
	angles.y = std::atan2f(delta.y, delta.x) * (180.0f / std::numbers::pi_v<float>);
	angles.z = 0.0f;
	return angles;
}

float F::LEGITBOT::MATH::GetFOVDistance(const QAngle& angView, const QAngle& angAim)
{
	QAngle delta;
	delta.x = std::remainderf(angAim.x - angView.x, 360.0f);
	delta.y = std::remainderf(angAim.y - angView.y, 360.0f);
	delta.z = 0.0f;
	return std::sqrtf(delta.x * delta.x + delta.y * delta.y);
}

QAngle F::LEGITBOT::MATH::SmoothAngle(const QAngle& angSrc, const QAngle& angDst, float flFactor)
{
	if (flFactor <= 0.0f)
		return angDst;

	QAngle delta;
	delta.x = std::remainderf(angDst.x - angSrc.x, 360.0f);
	delta.y = std::remainderf(angDst.y - angSrc.y, 360.0f);
	delta.z = 0.0f;

	QAngle result;
	result.x = angSrc.x + delta.x / (flFactor + 1.0f);
	result.y = angSrc.y + delta.y / (flFactor + 1.0f);
	result.z = 0.0f;
	return result.Clamp();
}
