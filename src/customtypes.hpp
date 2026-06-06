#pragma once

#include <cstdint>
#include <cmath>

using u8 = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;
using u64 = uint64_t;

using i8 = int8_t;
using i16 = int16_t;
using i32 = int32_t;
using i64 = int64_t;

using f32 = float;
using f64 = double;

constexpr u64 NS_TO_S = 1000000000;
constexpr f32 NS_TO_S_f32 = 1000000000.0f;

constexpr double PI_F64 = 3.14159265358979323846; 
constexpr float PI_F32 = 3.14159265f; 

struct Vec2 {
    f32 x = 0.0f;
    f32 y = 0.0f;

    constexpr f32 Length() const {
        return hypotf(x, y);
    }

    inline Vec2 Normalize() const {
        f32 len = Length();
        return Vec2{ x / len, y / len };
    }

    constexpr void Lerp(Vec2 b, f32 t) {
        *this = *this + (b - *this) * t;
    }

    // Unary
    constexpr Vec2 operator+() const {
        return *this;
    }

    constexpr Vec2 operator-() const {
        return {-x, -y};
    }

    // Vec2 <op> Vec2
    constexpr Vec2 operator+(const Vec2& rhs) const {
        return {x + rhs.x, y + rhs.y};
    }

    constexpr Vec2 operator-(const Vec2& rhs) const {
        return {x - rhs.x, y - rhs.y};
    }

    constexpr Vec2 operator*(const Vec2& rhs) const {
        return {x * rhs.x, y * rhs.y};
    }

    constexpr Vec2 operator/(const Vec2& rhs) const {
        return {x / rhs.x, y / rhs.y};
    }

    // Vec2 <op> scalar
    constexpr Vec2 operator*(float scalar) const {
        return {x * scalar, y * scalar};
    }

    constexpr Vec2 operator/(float scalar) const {
        return {x / scalar, y / scalar};
    }

    // Compound assignment
    constexpr Vec2& operator+=(const Vec2& rhs) {
        x += rhs.x;
        y += rhs.y;
        return *this;
    }

    constexpr Vec2& operator-=(const Vec2& rhs) {
        x -= rhs.x;
        y -= rhs.y;
        return *this;
    }

    constexpr Vec2& operator*=(const Vec2& rhs) {
        x *= rhs.x;
        y *= rhs.y;
        return *this;
    }

    constexpr Vec2& operator/=(const Vec2& rhs) {
        x /= rhs.x;
        y /= rhs.y;
        return *this;
    }

    constexpr Vec2& operator*=(float scalar) {
        x *= scalar;
        y *= scalar;
        return *this;
    }

    constexpr Vec2& operator/=(float scalar) {
        x /= scalar;
        y /= scalar;
        return *this;
    }

    // Comparison
    constexpr bool operator==(const Vec2& rhs) const {
        return x == rhs.x && y == rhs.y;
    }

    constexpr bool operator!=(const Vec2& rhs) const {
        return !(*this == rhs);
    }
};
