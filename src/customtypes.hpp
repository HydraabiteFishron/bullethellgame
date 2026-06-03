#pragma once

#include <cstdint>
#include <cmath>

#include <SDL3/SDL.h>

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

struct UpdateClockNS {
    u64 frame_start;
    u64 frame_end;
    u64 last_frame;

    u64 target_frame_time;
    u64 frame_time;

    u64 last_fps_update;
    u64 frame_counter;

    f32 fps;
    f32 dt;

    void SetTargetFPS(u64 target_fps);
    bool Start();
    void End();
};

// @param target_fps can be set to 0 for unlimited
UpdateClockNS CreateUpdateClockNS(u64 target_fps) {
    UpdateClockNS uc_ns;
    if (target_fps == 0)
        uc_ns.target_frame_time = 0;
    else
        uc_ns.target_frame_time = NS_TO_S / target_fps;
    
    u64 now = SDL_GetTicksNS();
    uc_ns.frame_start = now;
    uc_ns.last_frame = now;
    uc_ns.last_fps_update = now;

    return uc_ns;
}

// @param target_fps can be set to 0 for unlimited
void UpdateClockNS::SetTargetFPS(u64 target_fps) {
    if(target_fps == 0) {
        target_frame_time = 0;
        return;
    }
    target_frame_time = NS_TO_S / target_fps;
}

// returns true if FPS was updated (happens every second)
inline bool UpdateClockNS::Start() {
    last_frame = frame_start;
    frame_start = SDL_GetTicksNS();

    dt = (f32)(frame_start - last_frame) / NS_TO_S_f32;

    frame_counter++;
    u64 fps_update_delta = frame_start - last_fps_update;

    constexpr bool fps_updated = true;
    constexpr bool fps_not_updated = false;

    if (fps_update_delta >= NS_TO_S) {
        fps = frame_counter / ((f32)(fps_update_delta) / NS_TO_S_f32);
        last_fps_update = frame_start;
        frame_counter = 0;

        return fps_updated;
    } else {
        return fps_not_updated;
    }
}

inline void UpdateClockNS::End() {
    frame_end = SDL_GetTicksNS();
    frame_time = frame_end - frame_start;

    if (target_frame_time == 0) { 
        return; }
    
    if (frame_time < target_frame_time) {
        u64 amount_to_sleep = target_frame_time - frame_time;
        SDL_DelayPrecise(amount_to_sleep);
    }
}

struct Vec2 {
    f32 x;
    f32 y;

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