#pragma once

#include "../../sdk/datatypes/qangle.h"
#include "../../sdk/datatypes/vector.h"

namespace F::LEGITBOT::MATH
{
	QAngle CalcAngle(const Vector3& vecSrc, const Vector3& vecDst);
	float GetFOVDistance(const QAngle& angView, const QAngle& angAim);
	QAngle SmoothAngle(const QAngle& angSrc, const QAngle& angDst, float flFactor);
}
