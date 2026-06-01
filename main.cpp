#include <cmath>
#include <cstdint>
#include <cassert>

#include <iostream>
#include <vector>
#include <random>

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

#pragma region UpdateClock
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

    bool Start();
    void End();
};

UpdateClockNS CreateUpdateClockNS(u64* target_fps) {
    UpdateClockNS uc_ns;
    if (!target_fps)
        uc_ns.target_frame_time = 0;
    else
        uc_ns.target_frame_time = NS_TO_S / *target_fps;
    
    u64 now = SDL_GetTicksNS();
    uc_ns.frame_start = now;
    uc_ns.last_frame = now;
    uc_ns.last_fps_update = now;

    return uc_ns;
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
#pragma endregion

#pragma region CustomTypes
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

constexpr Vec2 CENTER_ORIGIN = { 0.5f, 0.5f };
// uses CENTER_ORIGIN by default
Vec2 GetTopleft(const Vec2 pos, const Vec2 size, const Vec2 origin = CENTER_ORIGIN) {
    return pos - size * origin;
}

struct Camera {
    Vec2 pos;
    f32 zoom;
};

Camera CreateCamera() {
    return { {0.0f, 0.0f}, 1.0f };
}

// for camera with center origin
Vec2 GetRenderCoords(const Camera& cam, const Vec2 world_pos, const Vec2 screen_dimensions) {
    assert(cam.zoom > 0.0f && "hey dipshit, camera zoom is set to 0.0f you fucking moron.");
    return (world_pos - cam.pos) * cam.zoom + (screen_dimensions * 0.5f);
}

inline Vec2 GetRenderSize(const Vec2 size, const f32 zoom) {
    return size * zoom;
}

#pragma endregion

template<typename T>
constexpr inline T Lerp(const T& a, const T& b, float t)
{
    return a + (b - a) * t;
}

struct Player {
    Vec2 vel;
    Vec2 pos;
    Vec2 size;

    f32 speed;
    f32 focused_speed;

    f32 inv_timer;
};

Player CreatePlayer() {
    Player player {
        .pos = { 0.0f, 0.0f },
        .size = { 8.0f, 8.0f },
        .speed = 150.0f,
        .focused_speed = 75.0f
    };

    return player;
}

struct Bullet {
    Vec2 vel;
    Vec2 pos;
    Vec2 size;
    
    f32 time_created;
};

enum BPType : u8 {
    BP_NITORI_RIPOFF,
    BP_AMOUNT
};

struct BulletPattern {
    std::vector<Bullet> bullets;
    // acts as the origin.
    // bullet positions will be relative to this.
    Vec2 pos;

    f32 elapsed;
    f32 last_summon_time;

    BPType type;
};

BulletPattern CreateBulletPattern(BPType type, Vec2 world_origin, size_t initial_vector_capacity = 64) {
    BulletPattern bp = {};
    bp.bullets = {};
    bp.bullets.reserve(initial_vector_capacity);
    bp.pos = world_origin;
    bp.type = type;
    bp.elapsed = 0.0f;
    bp.last_summon_time = 0.0f;

    return bp;
}

struct World {
    Player player;
    Camera camera;
    std::vector<BulletPattern> active_patterns;
};

World CreateDebugWorld() {
    World w;
    w.active_patterns = {};
    w.active_patterns.reserve(1);
    w.active_patterns.push_back(CreateBulletPattern(BP_NITORI_RIPOFF, {0.0f, 0.0f}));

    w.player = CreatePlayer();
    w.camera = CreateCamera();

    return w;
}

constexpr u32 default_width = 640, default_height = 360;

struct Game {
    World world;
    UpdateClockNS uc_ns;

    u32 ww;
    u32 wh;

    SDL_Event ev;
    SDL_Window *wn;
    SDL_Renderer *rr;
    const bool *kb;

    bool quit;
};

std::mt19937 RNG;
Game* game;

void MovePlayer(Player* p, f32 dt) {
    Vec2 dir = { 0.0f, 0.0f };
    // const bool *kb = game->kb;
    const bool *kb = SDL_GetKeyboardState(NULL);

    if (kb[SDL_SCANCODE_UP]) dir.y -= 1.0f;
    if (kb[SDL_SCANCODE_DOWN]) dir.y += 1.0f;
    if (kb[SDL_SCANCODE_LEFT]) dir.x -= 1.0f;
    if (kb[SDL_SCANCODE_RIGHT]) dir.x += 1.0f;

    const bool focused = kb[SDL_SCANCODE_LSHIFT];

    if (dir.Length() > 0.0f) {
        dir = dir.Normalize();
        if (focused) p->vel = dir * p->focused_speed;
        else p->vel = dir * p->speed;
    }

    p->pos += p->vel * dt;
    p->vel = { 0.0f, 0.0f };
}

void MoveFreecam(Vec2* pos, f32 dt) {
    Vec2 dir = { 0.0f, 0.0f };
    const bool *kb = SDL_GetKeyboardState(NULL);

    if (kb[SDL_SCANCODE_UP]) dir.y -= 1.0f;
    if (kb[SDL_SCANCODE_DOWN]) dir.y += 1.0f;
    if (kb[SDL_SCANCODE_LEFT]) dir.x -= 1.0f;
    if (kb[SDL_SCANCODE_RIGHT]) dir.x += 1.0f;

    if (dir.Length() > 0.0f) {
        dir = dir.Normalize();

        constexpr f32 freecam_movespeed = 200.0f;
        *pos += (dir * freecam_movespeed * dt);
    }
}

constexpr inline f32 DegToRad(f32 deg) {
    return deg * (f32(M_PI) / 180.0f);
}

bool IsColliding(Vec2 p1, Vec2 s1, Vec2 p2, Vec2 s2, Vec2 o1 = CENTER_ORIGIN, Vec2 o2 = CENTER_ORIGIN) {
    Vec2 tl1 = p1 - s1 * o1;
    Vec2 tl2 = p2 - s2 * o2;
    return tl1.x + s1.x > tl2.x &&
           tl1.x < tl2.x + s2.x &&
           tl1.y + s1.y > tl2.y &&
           tl1.y < tl2.y + s2.y;
}

void UpdateBulletPattern(BulletPattern *bp, f32 dt) {
    bp->elapsed += dt;
    f32 now = bp->elapsed;

    switch (bp->type) {
    case BP_NITORI_RIPOFF: {
        constexpr f32 bps = 1/16.0f; // summons per second

        f32 bullet_summon_delta = now - bp->last_summon_time;
        while (bullet_summon_delta >= bps) {
            bp->last_summon_time += bullet_summon_delta;
            now = bp->last_summon_time;
            bullet_summon_delta -= bps;

            constexpr f32 A = DegToRad(45.0f); // amplitude
            constexpr f32 FREQ = 0.125; // oscillations per second
            constexpr f32 OMEGA = 2.0f * f32(M_PI) * FREQ; // angular frequency

            f32 angle = DegToRad(270.0f) + A * sinf(OMEGA * now);

            std::uniform_real_distribution<f32> f32_distrib(-10.0f, 10.0f);
            f32 rand_f32 = f32_distrib(RNG);
            angle += DegToRad(rand_f32);

            // f32 bullet_speed = 63.0;
            f32 bullet_speed = 56.0;
            bullet_speed += bullet_speed * (rand_f32 / 100.0f);

            const Vec2 b_vel = Vec2 { cosf(angle), -sinf(angle) } * bullet_speed;

            constexpr u8 HIGH = 7;
            std::uniform_int_distribution<u8> xdist(0, HIGH);
            std::uniform_real_distribution<f32> rxdist(-40.0f, 40.0f);

            constexpr u8 GAP = 69;
            const f32 width_span = f32(HIGH * GAP);
            u8 randx = xdist(RNG);
            f32 x = f32(randx * GAP) - f32(width_span) * 0.5f + rxdist(RNG);

            Bullet b = {
                .vel = b_vel,
                .pos = { x, -f32(default_height) * 0.5f },
                .size = { 12.0, 12.0 },
                .time_created = now
            };
            bp->bullets.push_back(b);

            randx = (randx + 2) % HIGH;
            b.pos.x = f32(randx * GAP) - f32(width_span) * 0.5f + rxdist(RNG);
            bp->bullets.push_back(b);
        }

        break;
    }
    } // switch end

    for (auto& b : bp->bullets) {
        b.pos += (b.vel * dt);
    }
}

int main() {
    std::cout << "bullethell\n";
    
#pragma region Initialization
    game = (Game *)SDL_calloc(1, sizeof(Game));
    if (!game) {
        std::cout << "failed to allocate memory\n";
        return 1;
    }

    game->ww = 640;
    game->wh = 360;

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        std::cout << SDL_GetError() << "\n";
        return 1;
    }

    if (!SDL_CreateWindowAndRenderer(
        "bullethellgame", game->ww, game->wh, SDL_WINDOW_RESIZABLE,
        &game->wn, &game->rr
    )) {
        std::cout << SDL_GetError() << "\n";
        return 1;
    }

    /* rng */ {
        std::random_device rd;
        RNG = std::mt19937(rd());
    }

    // kb
    game->kb = SDL_GetKeyboardState(NULL);

    /* FPS */ {
        u64 target_fps = 60;
        game->uc_ns = CreateUpdateClockNS(&target_fps);
    }

    game->world = CreateDebugWorld();

#pragma endregion // Initialization

    // mainloop
    while (!game->quit) {
        // update clock start
        if (game->uc_ns.Start()) {
            UpdateClockNS& uc = game->uc_ns;

            char buf[256];
            SDL_snprintf(buf, sizeof(buf), "bullethellgame : FPS(%.2f) : DTms(%.2f)", uc.fps, uc.dt * 1000);
            SDL_SetWindowTitle(game->wn, buf);
        }

        // FREECAM BOOL DON'T FORGET THIS PLS
        static Camera freecam = CreateCamera();
        static bool freecam_enabled = false;
        static bool follow_camera_enabled = false;

#pragma region EventLoop
        SDL_Event& ev = game->ev;
        while (SDL_PollEvent(&ev)) {
            if (ev.type == SDL_EVENT_QUIT) {
                game->quit = true;
                break;
            }
            else if (ev.type == SDL_EVENT_WINDOW_RESIZED) {
                game->ww = ev.window.data1;
                game->wh = ev.window.data2;

                // scale camera accordingly
                game->world.camera.zoom = f32(game->wh) / f32(default_height);
            }
            else if (ev.type == SDL_EVENT_KEY_DOWN) {
                const SDL_Keycode key = ev.key.key;
                if (key == SDLK_ESCAPE) { // temporary convenient exit
                    game->quit = true;
                    break;
                } else if (key == SDLK_G && !ev.key.repeat) {
                    freecam_enabled = !freecam_enabled;
                } else if (key == SDLK_F && !ev.key.repeat) {
                    follow_camera_enabled = !follow_camera_enabled;
                }
            }
        }
#pragma endregion // EventLoop

        // everything else
        const float dt = game->uc_ns.dt;

        Player *p = &game->world.player;
        Camera *c = &game->world.camera;
        Vec2 wn_size = { f32(game->ww), f32(game->wh) };

        if (!freecam_enabled) {
            MovePlayer(p, game->uc_ns.dt);
            freecam.pos = c->pos;
            freecam.zoom = c->zoom;
        } else {
            MoveFreecam(&freecam.pos, dt);
        }

        for (auto& bp : game->world.active_patterns) {
            UpdateBulletPattern(&bp, dt);

            // delete offscreen bullets
            for (size_t i = 0; i < bp.bullets.size(); i++) {
                auto& b = bp.bullets[i];
                Vec2 btl = GetTopleft(bp.pos + b.pos, b.size);
                Vec2 brc = GetRenderCoords(*c, btl, wn_size);
                Vec2 brs = b.size * c->zoom;

                constexpr f32 more = 120.0f;
                bool offscreen =
                    brc.x + brs.x < 0 - more ||
                    brc.x > wn_size.x + more ||
                    brc.y + brs.y < 0 - more ||
                    brc.y > wn_size.y + more;

                if (offscreen) {
                    bp.bullets[i] = bp.bullets.back();
                    bp.bullets.pop_back();
                } else {
                    ++i;
                }
            }
        }

        Vec2 arena_pos = { 0, -400 };
        Vec2 arena_size = { 854, 480 };

        bool p_in_arena = IsColliding(p->pos, p->size, arena_pos, arena_size);
        if (p_in_arena) {
            const f32 t = 1.0f - expf(-6.9f * dt);
            const f32 target_zoom = f32(game->wh) / 500.0f;

            c->pos = Lerp(c->pos, arena_pos, t);
            c->zoom = Lerp(c->zoom, target_zoom, t);
        } else {
            const f32 t = 1.0f - expf(-6.9f * dt);
            const f32 target_zoom = f32(game->wh) / f32(default_height);

            if (follow_camera_enabled)
                c->pos = Lerp(c->pos, p->pos, t);
            else
                c->pos = Lerp(c->pos, { 0, 0 }, t);
            c->zoom = Lerp(c->zoom, target_zoom, t);
        }

        SDL_Renderer *rr = game->rr;
        SDL_SetRenderDrawColor(rr, 22, 22, 30, 255);
        SDL_RenderClear(rr);

        if (freecam_enabled)
            c = &freecam;
        else
            c = &game->world.camera;

        /* render world*/ {
            /* render arena */ {
                Vec2 tl = GetTopleft(arena_pos, arena_size);
                Vec2 rc = GetRenderCoords(*c, tl, wn_size);
                Vec2 rs = arena_size * c->zoom;

                SDL_FRect dst = { rc.x, rc.y, rs.x, rs.y };
                SDL_SetRenderDrawColor(rr, 0, 0, 0, 255);
                SDL_RenderFillRect(rr, &dst);
            }

            /* render player */ {
                Vec2 ptl = GetTopleft(p->pos, p->size);
                Vec2 prc = GetRenderCoords(*c, ptl, wn_size);
                Vec2 prs = p->size * c->zoom;

                SDL_FRect pdst = { prc.x, prc.y, prs.x, prs.y };
                SDL_SetRenderDrawColor(rr, 255, 255, 255, 255);
                SDL_RenderFillRect(rr, &pdst);
            }

            /* render bullets */ {
                for (const auto& bp : game->world.active_patterns) {
                    for (const auto& b : bp.bullets) {
                        Vec2 btl = GetTopleft(bp.pos + b.pos, b.size);
                        Vec2 brc = GetRenderCoords(*c, btl, wn_size);
                        Vec2 brs = b.size * c->zoom;

                        SDL_FRect bdst = { brc.x, brc.y, brs.x, brs.y };
                        SDL_SetRenderDrawColor(rr, 255, 255, 255, 255);
                        SDL_RenderFillRect(rr, &bdst);
                    }
                }
            }
        }

        SDL_RenderPresent(rr);

        // update clock end
        game->uc_ns.End();
    }

    SDL_DestroyRenderer(game->rr);
    SDL_DestroyWindow(game->wn);
    SDL_free(game);
    SDL_Quit();
}