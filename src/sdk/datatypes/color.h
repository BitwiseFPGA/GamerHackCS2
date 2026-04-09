#pragma once
#include <cstdint>
#include <cmath>
#include <algorithm>
#include <array>

// forward declaration for ImGui interop
struct ImVec4;

struct Color
{
	Color() = default;

	/// construct from 8-bit RGBA [0..255]
	constexpr Color(std::uint8_t r, std::uint8_t g, std::uint8_t b, std::uint8_t a = 255) :
		r(r), g(g), b(b), a(a) { }

	/// construct from int RGBA [0..255]
	constexpr Color(int r, int g, int b, int a = 255) :
		r(static_cast<std::uint8_t>(r)), g(static_cast<std::uint8_t>(g)),
		b(static_cast<std::uint8_t>(b)), a(static_cast<std::uint8_t>(a)) { }

	/// construct from float RGBA [0.0..1.0]
	constexpr Color(float r, float g, float b, float a = 1.0f) :
		r(static_cast<std::uint8_t>(r * 255.0f)), g(static_cast<std::uint8_t>(g * 255.0f)),
		b(static_cast<std::uint8_t>(b * 255.0f)), a(static_cast<std::uint8_t>(a * 255.0f)) { }

	/// construct from 8-bit array
	explicit constexpr Color(const std::uint8_t arr[4]) :
		r(arr[0]), g(arr[1]), b(arr[2]), a(arr[3]) { }

	/// construct from packed 32-bit RGBA (0xRRGGBBAA)
	explicit constexpr Color(std::uint32_t packed) :
		r(static_cast<std::uint8_t>((packed >> 24) & 0xFF)),
		g(static_cast<std::uint8_t>((packed >> 16) & 0xFF)),
		b(static_cast<std::uint8_t>((packed >> 8)  & 0xFF)),
		a(static_cast<std::uint8_t>( packed        & 0xFF)) { }

	// array access
	std::uint8_t& operator[](int i) { return reinterpret_cast<std::uint8_t*>(this)[i]; }
	const std::uint8_t& operator[](int i) const { return reinterpret_cast<const std::uint8_t*>(this)[i]; }

	bool operator==(const Color& o) const { return r == o.r && g == o.g && b == o.b && a == o.a; }
	bool operator!=(const Color& o) const { return !(*this == o); }

	/// @returns: copy with modified alpha
	[[nodiscard]] constexpr Color WithAlpha(std::uint8_t newAlpha) const
	{
		return { r, g, b, newAlpha };
	}

	/// @returns: float value [0..1] for component index
	[[nodiscard]] float BaseFloat(int i) const
	{
		const std::uint8_t* data = &r;
		return static_cast<float>(data[i]) / 255.0f;
	}

	/// output RGBA as float[4] in [0..1] range (suitable for ImGui)
	constexpr void ToFloat4(float (&out)[4]) const
	{
		out[0] = static_cast<float>(r) / 255.0f;
		out[1] = static_cast<float>(g) / 255.0f;
		out[2] = static_cast<float>(b) / 255.0f;
		out[3] = static_cast<float>(a) / 255.0f;
	}

	/// @returns: std::array<float,4> in [0..1]
	[[nodiscard]] constexpr std::array<float, 4> ToFloat4() const
	{
		return { static_cast<float>(r) / 255.0f, static_cast<float>(g) / 255.0f,
		         static_cast<float>(b) / 255.0f, static_cast<float>(a) / 255.0f };
	}

	/// construct from float[4] in [0..1]
	[[nodiscard]] static constexpr Color FromFloat4(const float f[4])
	{
		return { f[0], f[1], f[2], f[3] };
	}

	/// output RGB as float[3] in [0..1]
	constexpr void ToFloat3(float (&out)[3]) const
	{
		out[0] = static_cast<float>(r) / 255.0f;
		out[1] = static_cast<float>(g) / 255.0f;
		out[2] = static_cast<float>(b) / 255.0f;
	}

	/// @returns: packed 32-bit value (0xRRGGBBAA)
	[[nodiscard]] constexpr std::uint32_t ToU32() const
	{
		return (static_cast<std::uint32_t>(r) << 24) | (static_cast<std::uint32_t>(g) << 16) |
		       (static_cast<std::uint32_t>(b) << 8)  |  static_cast<std::uint32_t>(a);
	}

	/// @returns: packed ABGR for ImGui IM_COL32 format
	[[nodiscard]] constexpr std::uint32_t ToImU32(float flAlphaMultiplier = 1.0f) const
	{
		const auto ba = static_cast<std::uint8_t>(static_cast<float>(a) * flAlphaMultiplier);
		return (static_cast<std::uint32_t>(ba) << 24) | (static_cast<std::uint32_t>(b) << 16) |
		       (static_cast<std::uint32_t>(g) << 8)   |  static_cast<std::uint32_t>(r);
	}

	/// @returns: ImVec4 for ImGui color pickers etc.
	/// Note: requires ImVec4 to be defined (include imgui.h before calling)
	ImVec4 ToImVec4(float flAlphaMultiplier = 1.0f) const;

	/// construct from ImVec4
	static Color FromImVec4(const ImVec4& v);

	/// convert RGB to HSB/HSV, output in [0..1]
	void ToHSB(float (&hsb)[3]) const
	{
		float base[3];
		ToFloat3(base);

		float& rr = base[0];
		float& gg = base[1];
		float& bb = base[2];

		float kernel = 0.0f;
		if (gg < bb) { std::swap(gg, bb); kernel = -1.0f; }
		if (rr < gg) { std::swap(rr, gg); kernel = -2.0f / 6.0f - kernel; }

		const float chroma = rr - (std::min)(gg, bb);
		hsb[0] = std::fabsf(kernel + (gg - bb) / (6.0f * chroma + 1e-20f));
		hsb[1] = chroma / (rr + 1e-20f);
		hsb[2] = rr;
	}

	/// construct from HSB/HSV [0..1]
	[[nodiscard]] static Color FromHSB(float h, float s, float v, float alpha = 1.0f)
	{
		constexpr float hueRange = 60.0f / 360.0f;
		const float hPrime = std::fmodf(h, 1.0f) / hueRange;
		const int sector = static_cast<int>(hPrime);
		const float frac = hPrime - static_cast<float>(sector);

		const float p = v * (1.0f - s);
		const float q = v * (1.0f - s * frac);
		const float t = v * (1.0f - s * (1.0f - frac));

		float rr, gg, bb;
		switch (sector)
		{
		case 0: rr = v; gg = t; bb = p; break;
		case 1: rr = q; gg = v; bb = p; break;
		case 2: rr = p; gg = v; bb = t; break;
		case 3: rr = p; gg = q; bb = v; break;
		case 4: rr = t; gg = p; bb = v; break;
		default: rr = v; gg = p; bb = q; break;
		}

		return { rr, gg, bb, alpha };
	}

	// predefined colors
	static constexpr Color White()   { return { 255, 255, 255, 255 }; }
	static constexpr Color Black()   { return {   0,   0,   0, 255 }; }
	static constexpr Color Red()     { return { 255,   0,   0, 255 }; }
	static constexpr Color Green()   { return {   0, 255,   0, 255 }; }
	static constexpr Color Blue()    { return {   0,   0, 255, 255 }; }
	static constexpr Color Yellow()  { return { 255, 255,   0, 255 }; }
	static constexpr Color Cyan()    { return {   0, 255, 255, 255 }; }
	static constexpr Color Magenta() { return { 255,   0, 255, 255 }; }
	static constexpr Color Orange()  { return { 255, 165,   0, 255 }; }

	/// @returns: packed ABGR (alpha in high byte)
	[[nodiscard]] constexpr std::uint32_t GetABGR() const
	{
		return (static_cast<std::uint32_t>(a) << 24) | (static_cast<std::uint32_t>(b) << 16) |
		       (static_cast<std::uint32_t>(g) << 8)  |  static_cast<std::uint32_t>(r);
	}

	/// @returns: RGB as Vector3-like {r/255, g/255, b/255}
	struct Vec3Result { float x, y, z; };
	[[nodiscard]] constexpr Vec3Result Vec3() const
	{
		return { static_cast<float>(r) / 255.0f, static_cast<float>(g) / 255.0f, static_cast<float>(b) / 255.0f };
	}

	/// linear interpolation between two colors
	[[nodiscard]] static constexpr Color Lerp(const Color& a, const Color& b, float t)
	{
		return {
			static_cast<std::uint8_t>(static_cast<float>(a.r) + (static_cast<float>(b.r) - static_cast<float>(a.r)) * t),
			static_cast<std::uint8_t>(static_cast<float>(a.g) + (static_cast<float>(b.g) - static_cast<float>(a.g)) * t),
			static_cast<std::uint8_t>(static_cast<float>(a.b) + (static_cast<float>(b.b) - static_cast<float>(a.b)) * t),
			static_cast<std::uint8_t>(static_cast<float>(a.a) + (static_cast<float>(b.a) - static_cast<float>(a.a)) * t)
		};
	}

	// predefined colors (continued)
	static constexpr Color None()    { return {   0,   0,   0,   0 }; }

	std::uint8_t r = 0, g = 0, b = 0, a = 0;
};

/// HSV representation with float components [0..1]
struct hsv_t
{
	float h, s, v;

	constexpr hsv_t() : h(0.f), s(0.f), v(0.f) {}
	constexpr hsv_t(float h, float s, float v) : h(h), s(s), v(v) {}

	/// convert HSV to Color
	[[nodiscard]] Color ToColor(std::uint8_t alpha = 255) const
	{
		return Color::FromHSB(h, s, v, static_cast<float>(alpha) / 255.0f);
	}
};

/// convert Color to Valve-style HSV struct
inline hsv_t ToValveHSV(const Color& c)
{
	float hsb[3];
	c.ToHSB(hsb);
	return { hsb[0], hsb[1], hsb[2] };
}
