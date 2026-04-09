#pragma once
#include "vector2.h"

struct Rect_t {
    float x, y, w, h;

    constexpr Rect_t() : x(0.f), y(0.f), w(0.f), h(0.f) {}
    constexpr Rect_t(float x, float y, float w, float h) : x(x), y(y), w(w), h(h) {}

    Vector2 Center() const { return { x + w * 0.5f, y + h * 0.5f }; }
    Vector2 GetMin() const { return { x, y }; }
    Vector2 GetMax() const { return { x + w, y + h }; }
    Vector2 GetSize() const { return { w, h }; }

    bool Contains(float px, float py) const { return px >= x && px <= x + w && py >= y && py <= y + h; }
    bool Contains(const Vector2& v) const { return Contains(v.x, v.y); }
};
