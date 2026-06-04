#include <cmath>
#include <cassert>

#include <iostream>
#include <vector>
#include <random>

#define SDL_MAIN_USE_CALLBACKS 1
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include "colors.hpp"
#include "customtypes.hpp"

constexpr Vec2 CENTER_ORIGIN = { 0.5f, 0.5f };
// uses CENTER_ORIGIN by default
Vec2 GetTopleft(const Vec2 pos, const Vec2 size, const Vec2 origin = CENTER_ORIGIN) {
    return pos - size * origin;
}

#pragma region Camera and Camera Math
struct Camera {
    Vec2 pos;
    f32 zoom;
};

Camera CreateCamera(Vec2 pos = {0.0f,0.0f}, f32 zoom = 1.0f) {
    return { pos, zoom };
}

// for camera with center origin
Vec2 GetRenderCoords(const Camera& cam, const Vec2 world_pos, const Vec2 screen_dimensions) {
    assert(cam.zoom > 0.0f && "hey dipshit, camera zoom is set to 0.0f you fucking moron.");
    return (world_pos - cam.pos) * cam.zoom + (screen_dimensions * 0.5f);
}

inline Vec2 GetRenderSize(const Vec2 size, const f32 zoom) {
    return size * zoom;
}
#pragma endregion // Camera and Camera Math

#pragma region Custom Math
constexpr inline f32 DegToRad(f32 deg) {
    return deg * (f32(PI_F32) / 180.0f);
}

template<typename T>
constexpr inline T Lerp(const T& a, const T& b, float t) {
    return a + (b - a) * t;
}
#pragma endregion // Custom Math

#pragma region Game Types
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
    bool fullscreen;
};
#pragma endregion // Game Types

#pragma region Global Vars
std::mt19937 RNG;
Game* game;
#pragma endregion // Global Vars

#pragma region Game Logic Functions
// Rectngle touchy touchy rectangle
bool IsColliding(
    Vec2 p1, Vec2 s1,
    Vec2 p2, Vec2 s2,
    Vec2 o1 = CENTER_ORIGIN, Vec2 o2 = CENTER_ORIGIN
) {
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

        while (now - bp->last_summon_time >= bps) {
            bp->last_summon_time += bps;

            constexpr f32 A = DegToRad(45.0f); // amplitude
            constexpr f32 FREQ = 0.125; // oscillations per second
            constexpr f32 OMEGA = 2.0f * f32(PI_F32) * FREQ; // angular frequency

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
                .time_created = bp->last_summon_time
            };
            bp->bullets.push_back(b);

            randx = (randx + 2) % HIGH;
            b.pos.x = f32(randx * GAP) - f32(width_span) * 0.5f + rxdist(RNG);
            bp->bullets.push_back(b);
        }

        break;
    }
	default:
		break;
    } // switch end

    for (auto& b : bp->bullets) {
        b.pos += (b.vel * dt);
    }
}
#pragma endregion // Game Logic Functions

#pragma region Initialization
SDL_AppResult SDL_AppInit(void** appstate, int argc, char* argv[]) {
    game = (Game *)SDL_calloc(1, sizeof(Game));
    if (!game) {
        std::cout << "failed to allocate memory\n";
        return SDL_APP_FAILURE;
    }
    *appstate = game;

    game->ww = 640;
    game->wh = 360;

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        std::cout << SDL_GetError() << "\n";
        return SDL_APP_FAILURE;
    }

    if (!SDL_CreateWindowAndRenderer(
        "bullethellgame", game->ww, game->wh, SDL_WINDOW_RESIZABLE,
        &game->wn, &game->rr
    )) {
        std::cout << SDL_GetError() << "\n";
        return SDL_APP_FAILURE;
    }

    // rng
    std::random_device rd;
    RNG = std::mt19937(rd());

    // kb
    game->kb = SDL_GetKeyboardState(NULL);

    // fps
    u64 target_fps = 240;
    game->uc_ns = CreateUpdateClockNS(target_fps);

    game->world = CreateDebugWorld();
    return SDL_APP_CONTINUE;
}
#pragma endregion // Initialization

#pragma region Events
SDL_AppResult SDL_AppEvent(void* appstate, SDL_Event* event) {
    Game *state = (Game *)appstate;

    switch (event->type) {
    case SDL_EVENT_QUIT:
        state->quit = true;
        break;

    case SDL_EVENT_WINDOW_RESIZED:
        state->ww = event->window.data1;
        state->wh = event->window.data2;

        // scale camera accordingly
        state->world.camera.zoom = f32(state->wh) / f32(default_height);
        break;

    case SDL_EVENT_KEY_DOWN: {
        const SDL_Keycode key = event->key.key;
        const bool repeat = event->key.repeat;

        if (!repeat) {
            switch (key) {
            // temporary convenient exit
            case SDLK_ESCAPE:
                state->quit = true;
                break;

            case SDLK_F11:
                state->fullscreen = !state->fullscreen;
                SDL_SetWindowFullscreen(state->wn, state->fullscreen);
                break;

            case SDLK_L:
                // simulate a lagspike
                SDL_Delay(1000);
                break;

            default:
                break;
            } // switch (key) end
        } // repeat keys end
    }

    default:
        break;
    } // switch (event->type) end

    return SDL_APP_CONTINUE;
}
#pragma endregion // Events

SDL_AppResult SDL_AppIterate(void* appstate) {
    Game *state = (Game *)appstate;

    if (state->quit)
        return SDL_APP_SUCCESS;

    if (state->uc_ns.Start()) {
        UpdateClockNS& uc = game->uc_ns;

        char buf[256];
        SDL_snprintf(buf, sizeof(buf), "bullethellgame : FPS(%.2f) : DTms(%.2f)", uc.fps, uc.dt * 1000);
        SDL_SetWindowTitle(game->wn, buf);
    }

#pragma region Update
    const float dt = game->uc_ns.dt;

    Player *p = &game->world.player;
    Camera *c = &game->world.camera;
    Vec2 wn_size = { f32(game->ww), f32(game->wh) };

    /* Move Player */ {
        const bool *kb = state->kb;
        Vec2 dir = { 0.0f, 0.0f };

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
    } // Player Movement

    // Update Bullet Patterns and Delete Offscreen
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
    } // Update Bullet Patterns and Delete Offscreen
#pragma endregion // Update

#pragma region Rendering
    SDL_Renderer *rr = game->rr;
	SetDrawColor(rr, Colors::tokyonight_blue);
    SDL_RenderClear(rr);

    /* render world*/ {
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

#pragma endregion // Rendering

    // sleep/frame limiter
    state->uc_ns.End();
    return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void* appstate, SDL_AppResult result) {
    Game *state = (Game *)appstate;

    SDL_DestroyRenderer(game->rr);
    SDL_DestroyWindow(game->wn);
    SDL_free(game);
    SDL_Quit();
}
