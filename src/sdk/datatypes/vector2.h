#pragma once
#include <cmath>

// Simplified 2D vector for screen-space / UI coordinates.
// For game-world 2D vectors, prefer Vector2D from vector.h.

class Vector2 {
public:
    float x, y;

    constexpr Vector2() : x(0.f), y(0.f) {}
    constexpr Vector2(float x, float y) : x(x), y(y) {}

    float Dot(const Vector2& v) const { return x * v.x + y * v.y; }
    float Length() const { return std::sqrtf(x * x + y * y); }
    float LengthSqr() const { return x * x + y * y; }
    float Distance(const Vector2& v) const { return (*this - v).Length(); }

    Vector2 Normalized() const {
        float l = Length();
        if (l != 0.f) return { x / l, y / l };
        return {};
    }

    Vector2 operator+(const Vector2& v) const { return { x + v.x, y + v.y }; }
    Vector2 operator-(const Vector2& v) const { return { x - v.x, y - v.y }; }
    Vector2 operator*(float f) const { return { x * f, y * f }; }
    Vector2 operator/(float f) const { return { x / f, y / f }; }

    Vector2& operator+=(const Vector2& v) { x += v.x; y += v.y; return *this; }
    Vector2& operator-=(const Vector2& v) { x -= v.x; y -= v.y; return *this; }
    Vector2& operator*=(float f) { x *= f; y *= f; return *this; }
    Vector2& operator/=(float f) { x /= f; y /= f; return *this; }

    bool operator==(const Vector2& v) const { return x == v.x && y == v.y; }
    bool operator!=(const Vector2& v) const { return !(*this == v); }

    static constexpr Vector2 Zero() { return { 0.f, 0.f }; }
};
