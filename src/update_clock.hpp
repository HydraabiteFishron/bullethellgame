#pragma once

#include <SDl3/SDL.h>
#include "customtypes.hpp"

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
static inline UpdateClockNS CreateUpdateClockNS(u64 target_fps) {
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
inline void UpdateClockNS::SetTargetFPS(u64 target_fps) {
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
