#pragma once
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/*
 * EASING FUNCTIONS
 * Standard easing functions for animations
 * All take double [0..1] and return double [0..1]
 */
namespace EASING
{
	// Sine
	inline double InSine(double t) { return 1.0 - std::cos(t * M_PI / 2.0); }
	inline double OutSine(double t) { return std::sin(t * M_PI / 2.0); }
	inline double InOutSine(double t) { return -(std::cos(M_PI * t) - 1.0) / 2.0; }

	// Quad
	inline double InQuad(double t) { return t * t; }
	inline double OutQuad(double t) { return 1.0 - (1.0 - t) * (1.0 - t); }
	inline double InOutQuad(double t) { return t < 0.5 ? 2.0 * t * t : 1.0 - std::pow(-2.0 * t + 2.0, 2.0) / 2.0; }

	// Cubic
	inline double InCubic(double t) { return t * t * t; }
	inline double OutCubic(double t) { return 1.0 - std::pow(1.0 - t, 3.0); }
	inline double InOutCubic(double t) { return t < 0.5 ? 4.0 * t * t * t : 1.0 - std::pow(-2.0 * t + 2.0, 3.0) / 2.0; }

	// Quart
	inline double InQuart(double t) { return t * t * t * t; }
	inline double OutQuart(double t) { return 1.0 - std::pow(1.0 - t, 4.0); }
	inline double InOutQuart(double t) { return t < 0.5 ? 8.0 * t * t * t * t : 1.0 - std::pow(-2.0 * t + 2.0, 4.0) / 2.0; }

	// Quint
	inline double InQuint(double t) { return t * t * t * t * t; }
	inline double OutQuint(double t) { return 1.0 - std::pow(1.0 - t, 5.0); }
	inline double InOutQuint(double t) { return t < 0.5 ? 16.0 * t * t * t * t * t : 1.0 - std::pow(-2.0 * t + 2.0, 5.0) / 2.0; }

	// Expo
	inline double InExpo(double t) { return t == 0.0 ? 0.0 : std::pow(2.0, 10.0 * t - 10.0); }
	inline double OutExpo(double t) { return t == 1.0 ? 1.0 : 1.0 - std::pow(2.0, -10.0 * t); }
	inline double InOutExpo(double t)
	{
		if (t == 0.0) return 0.0;
		if (t == 1.0) return 1.0;
		return t < 0.5 ? std::pow(2.0, 20.0 * t - 10.0) / 2.0 : (2.0 - std::pow(2.0, -20.0 * t + 10.0)) / 2.0;
	}

	// Circ
	inline double InCirc(double t) { return 1.0 - std::sqrt(1.0 - t * t); }
	inline double OutCirc(double t) { return std::sqrt(1.0 - std::pow(t - 1.0, 2.0)); }
	inline double InOutCirc(double t)
	{
		return t < 0.5
			? (1.0 - std::sqrt(1.0 - std::pow(2.0 * t, 2.0))) / 2.0
			: (std::sqrt(1.0 - std::pow(-2.0 * t + 2.0, 2.0)) + 1.0) / 2.0;
	}

	// Back
	inline double InBack(double t)
	{
		constexpr double c1 = 1.70158;
		constexpr double c3 = c1 + 1.0;
		return c3 * t * t * t - c1 * t * t;
	}
	inline double OutBack(double t)
	{
		constexpr double c1 = 1.70158;
		constexpr double c3 = c1 + 1.0;
		return 1.0 + c3 * std::pow(t - 1.0, 3.0) + c1 * std::pow(t - 1.0, 2.0);
	}
	inline double InOutBack(double t)
	{
		constexpr double c1 = 1.70158;
		constexpr double c2 = c1 * 1.525;
		return t < 0.5
			? (std::pow(2.0 * t, 2.0) * ((c2 + 1.0) * 2.0 * t - c2)) / 2.0
			: (std::pow(2.0 * t - 2.0, 2.0) * ((c2 + 1.0) * (t * 2.0 - 2.0) + c2) + 2.0) / 2.0;
	}

	// Elastic
	inline double InElastic(double t)
	{
		if (t == 0.0) return 0.0;
		if (t == 1.0) return 1.0;
		constexpr double c4 = (2.0 * M_PI) / 3.0;
		return -std::pow(2.0, 10.0 * t - 10.0) * std::sin((t * 10.0 - 10.75) * c4);
	}
	inline double OutElastic(double t)
	{
		if (t == 0.0) return 0.0;
		if (t == 1.0) return 1.0;
		constexpr double c4 = (2.0 * M_PI) / 3.0;
		return std::pow(2.0, -10.0 * t) * std::sin((t * 10.0 - 0.75) * c4) + 1.0;
	}
	inline double InOutElastic(double t)
	{
		if (t == 0.0) return 0.0;
		if (t == 1.0) return 1.0;
		constexpr double c5 = (2.0 * M_PI) / 4.5;
		return t < 0.5
			? -(std::pow(2.0, 20.0 * t - 10.0) * std::sin((20.0 * t - 11.125) * c5)) / 2.0
			: (std::pow(2.0, -20.0 * t + 10.0) * std::sin((20.0 * t - 11.125) * c5)) / 2.0 + 1.0;
	}

	// Bounce
	inline double OutBounce(double t)
	{
		constexpr double n1 = 7.5625;
		constexpr double d1 = 2.75;
		if (t < 1.0 / d1) return n1 * t * t;
		if (t < 2.0 / d1) { t -= 1.5 / d1; return n1 * t * t + 0.75; }
		if (t < 2.5 / d1) { t -= 2.25 / d1; return n1 * t * t + 0.9375; }
		t -= 2.625 / d1; return n1 * t * t + 0.984375;
	}
	inline double InBounce(double t) { return 1.0 - OutBounce(1.0 - t); }
	inline double InOutBounce(double t)
	{
		return t < 0.5
			? (1.0 - OutBounce(1.0 - 2.0 * t)) / 2.0
			: (1.0 + OutBounce(2.0 * t - 1.0)) / 2.0;
	}
}
