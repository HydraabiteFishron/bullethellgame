#include <cmath>
#include <cassert>

#include <iostream>
#include <vector>
#include <unordered_map>
#include <random>

#define SDL_MAIN_USE_CALLBACKS 1
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3_image/SDL_image.h>

#include "colors.hpp"
#include "customtypes.hpp"
#include "update_clock.hpp"
#include "rendering.hpp"

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
    Vec2 size = { 8.0f, 8.0f };

	f32 radius = 3.0f;
    f32 speed = 150.0f;
    f32 focused_speed = 75.0f;

    f32 inv_timer = 0.0f;
};

struct Bullet {
    Vec2 vel;
    Vec2 pos;
    Vec2 size;
    
    f32 time_created;
};

enum BPType : u8 {
	BP_INVALID,
    BP_NITORI_RIPOFF,
    BP_AMOUNT
};

struct BulletPattern {
    std::vector<Bullet> bullets;
    // acts as the origin.
    // bullet positions will be relative to this.
    Vec2 pos;

    f32 elapsed = 0.0f;
    f32 last_summon_time = 0.0f;

    BPType type;
};

BulletPattern CreateBulletPattern(BPType type, Vec2 world_pos, size_t initial_vector_capacity = 64) {
    BulletPattern bp;
    bp.bullets.reserve(initial_vector_capacity);
    bp.pos = world_pos;
    bp.type = type;

    return bp;
}

namespace Tiles {
	constexpr int CHUNK_SIZE = 16;
	constexpr int SIZE = 16;
	constexpr f32 SCALE_MOD = 1.69f;
	constexpr f32 SCALED_SIZE = SIZE * SCALE_MOD;

	constexpr char FILE_PATH[] = "assets/tiles_shit.png";
	constexpr char TEXTURE_KEY[] = "tile_atlas";
	constexpr char EMPTY_CHUNK_KEY[] = "empty_chunk";

	// Hardcoded SRC Rects
	constexpr SDL_FRect GRASS_SRC = {
		0, 0,
		Tiles::SIZE, Tiles::SIZE
	};

	enum Type : u8 {
		TILE_GRASS,
		TILE_PATH,
	};
}

enum CameraState : u8 {
	CAMERA_DEFAULT,
	CAMERA_FOLLOW_PLAYER,
};

enum CameraDevHax : u8 {
	CAMDEVHAX_NONE,
	CAMDEVHAX_FREECAM,
	CAMDEVHAX_ZOOM_HAX,
	CAMDEVHAX_AMOUNT
};

struct World {
    std::vector<BulletPattern> active_patterns = {};
    Player player = {};

    Camera camera;
	Camera freecam;
	f32 zoom_override = 1.0f;

	CameraState cam_state = CAMERA_DEFAULT;
	CameraDevHax camdevhax = CAMDEVHAX_NONE;
};

World CreateDebugWorld() {
    World w;
    w.active_patterns = {};
    w.active_patterns.reserve(1);
    w.active_patterns.push_back(CreateBulletPattern(BP_NITORI_RIPOFF, {0.0f, 0.0f}));

	w.cam_state = CAMERA_DEFAULT;
    return w;
}

World CreateDebugWorld2() {
    World w;
    w.active_patterns = {};
	w.cam_state = CAMERA_FOLLOW_PLAYER;

    return w;
}

// constexpr u32 default_width = 640, default_height = 360;
constexpr u32 default_width = 854, default_height = 480;
using TextureMap = std::unordered_map<std::string, SDL_Texture*>;

struct Game {
    World world = {};
    UpdateClockNS uc_ns = {};

	TextureMap tex_map = {};

    u32 ww = 0;
    u32 wh = 0;

    SDL_Event ev = {};
    SDL_Window *wn = NULL;
    SDL_Renderer *rr = NULL;
    const bool *kb = NULL;

    bool quit = false;
    bool fullscreen = false;
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
	//
    game = new Game;
    if (!game) {
        std::cout << "failed to allocate memory\n";
        return SDL_APP_FAILURE;
    }
    *appstate = game;

    game->ww = default_width;
    game->wh = default_height;

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

	// renderer
	SDL_SetDefaultTextureScaleMode(game->rr, SDL_SCALEMODE_PIXELART);

    // rng
    std::random_device rd;
    RNG = std::mt19937(rd());

    // kb
    game->kb = SDL_GetKeyboardState(NULL);

    // fps
    u64 target_fps = 60;
    game->uc_ns = CreateUpdateClockNS(target_fps);

	// world
    game->world = CreateDebugWorld2();

	// textures
	TextureMap *tm = &game->tex_map;

	(*tm)[Tiles::TEXTURE_KEY] = IMG_LoadTexture(game->rr, Tiles::FILE_PATH);
	assert((*tm)[Tiles::TEXTURE_KEY] != nullptr
		&& "tile_shit.png could not be loaded");

	/* very stupid shenanigans */ {
		constexpr int chunk_tex_size = Tiles::CHUNK_SIZE * Tiles::SCALED_SIZE;
		SDL_Texture *empty_grass_chunk_target = SDL_CreateTexture(
			game->rr,
			SDL_PIXELFORMAT_RGBA8888,
			SDL_TEXTUREACCESS_TARGET,
			chunk_tex_size,
			chunk_tex_size
		);
		
		assert(empty_grass_chunk_target != nullptr
			&& "could not create empty_grass_chunk"
			"texture target with SDL_CreateTexture()");

		// create the empty default chunk
		SDL_SetRenderTarget(game->rr, empty_grass_chunk_target);
		for (size_t y = 0; y < Tiles::CHUNK_SIZE; y++) {
		for (size_t x = 0; x < Tiles::CHUNK_SIZE; x++) {
			SDL_FRect dst = {
				x * Tiles::SCALED_SIZE,
				y * Tiles::SCALED_SIZE,
				Tiles::SCALED_SIZE,
				Tiles::SCALED_SIZE
			};
			SDL_FRect src = Tiles::GRASS_SRC;
			SDL_RenderTexture(game->rr, (*tm)[Tiles::TEXTURE_KEY], &src, &dst);
		}}
		SDL_Surface *wa = SDL_RenderReadPixels(game->rr, NULL);
		SDL_SetRenderTarget(game->rr, nullptr);

		SDL_Texture *empty_grass_chunk_static =
			SDL_CreateTextureFromSurface(game->rr, wa);

		SDL_DestroyTexture(empty_grass_chunk_target);
		SDL_DestroySurface(wa);
		
		(*tm)[Tiles::EMPTY_CHUNK_KEY] = empty_grass_chunk_static;
	}

	SDL_RaiseWindow(game->wn);
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

			case SDLK_G: {
				CameraDevHax *hax = &state->world.camdevhax;

				// cycle through camdevhax
				u8 new_hax = (*hax + 1) % CAMDEVHAX_AMOUNT;
				*hax = CameraDevHax(new_hax);
			} break;

            default:
                break;
            } // switch (key) end
        } // repeat keys end
    } break;

	case SDL_EVENT_MOUSE_WHEEL: {
		f32 *zoom = NULL;

		CameraDevHax hax = state->world.camdevhax;
		if (hax == CAMDEVHAX_FREECAM)
			zoom = &state->world.freecam.zoom;
		else if (hax == CAMDEVHAX_ZOOM_HAX)
			zoom = &state->world.zoom_override;
		else
			break;

		*zoom *= std::pow(1.1f, event->wheel.y);
		if (*zoom < 0.1f)
			*zoom = 0.1f;
		else if (*zoom > 5.0f)
			*zoom = 5.0f;
	} break;

    default:
        break;
    } // switch (event->type) end

    return SDL_APP_CONTINUE;
}
#pragma endregion // Events

#pragma region Iteration
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

	World *wld = &game->world;
    Player *p = &wld->player;
    Camera *c = &wld->camera;
    Vec2 wn_size = { f32(game->ww), f32(game->wh) };

	// Movement
	if (wld->camdevhax == CAMDEVHAX_FREECAM) {
        const bool *kb = state->kb;
        Vec2 dir = { 0.0f, 0.0f };

        if (kb[SDL_SCANCODE_UP]) dir.y -= 1.0f;
        if (kb[SDL_SCANCODE_DOWN]) dir.y += 1.0f;
        if (kb[SDL_SCANCODE_LEFT]) dir.x -= 1.0f;
        if (kb[SDL_SCANCODE_RIGHT]) dir.x += 1.0f;

        const bool focused = kb[SDL_SCANCODE_LSHIFT];
		constexpr f32 fc_mvspd = 400.0f;
		constexpr f32 foc_fc_mvspd = 200.0f;

        if (dir.Length() > 0.0f) {
            dir = dir.Normalize();
			const f32 mvspd = (focused) ?
				foc_fc_mvspd / wld->freecam.zoom :
				fc_mvspd / wld->freecam.zoom;

			wld->freecam.pos += dir * mvspd * dt;
        }
	} // freecam movement
	else { // player movement
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

	} // player movement
	
	if (wld->cam_state == CAMERA_FOLLOW_PLAYER) {
		c->pos = p->pos;

		// make sure freecam is synced when disabled
		if (wld->camdevhax != CAMDEVHAX_FREECAM)
			wld->freecam = *c;
	}

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
		TextureMap *tex_map = &state->tex_map;
		// cam to use for rendering
		Camera rr_cam = *c;

		if (wld->camdevhax == CAMDEVHAX_FREECAM)
			rr_cam = wld->freecam;
		else if (wld->camdevhax == CAMDEVHAX_ZOOM_HAX)
			rr_cam.zoom = wld->zoom_override;

		/* render tiles */ {
			auto it = tex_map->find(Tiles::TEXTURE_KEY);


			if (it != tex_map->end()) {
				SDL_Texture *tile_atlas = it->second;
				Vec2 tex_size = {};

				SDL_GetTextureSize(tile_atlas, &tex_size.x, &tex_size.y);
				tex_size *= Tiles::SCALE_MOD;

				constexpr int CHUNK_SIZE = Tiles::CHUNK_SIZE;
				int ply_chunk_x = int(p->pos.x) / (CHUNK_SIZE * Tiles::SCALED_SIZE);
				int ply_chunk_y = int(p->pos.y) / (CHUNK_SIZE * Tiles::SCALED_SIZE);

				// simulates a rounding down because
				// integer division rounds towards 0
				if (p->pos.x < 0.0f) ply_chunk_x -= 1;
				if (p->pos.y < 0.0f) ply_chunk_y -= 1;

				constexpr size_t MAX_CTR = 15;
				Vec2 chunks_to_render[MAX_CTR] = {};
				size_t ctr_count = 0;

				for (size_t y = 0; y < 3; y++) {
				for (size_t x = 0; x < 5; x++) {
					assert(ctr_count <= MAX_CTR &&
						"seems your math was wrong dumbass");

					chunks_to_render[ctr_count] = Vec2{
						f32(ply_chunk_x) + f32(x) - 2.0f,
						f32(ply_chunk_y) + f32(y) - 1.0f
					}; ctr_count++;
				}}

				for (size_t c = 0; c < MAX_CTR; c++) {
					Vec2 chunk_pos = chunks_to_render[c] * CHUNK_SIZE;
					chunk_pos *= Tiles::SCALED_SIZE;

					constexpr Vec2 size_of_one_tile = {
						f32(Tiles::SCALED_SIZE),
						f32(Tiles::SCALED_SIZE)
					};
					constexpr Vec2 size_of_one_chunk = {
						Tiles::SCALED_SIZE * CHUNK_SIZE,
						Tiles::SCALED_SIZE * CHUNK_SIZE
					};

					Vec2 tl = GetTopleft(chunk_pos, size_of_one_tile);
					Vec2 rc = GetRenderCoords(rr_cam, tl, wn_size);
					Vec2 rs = size_of_one_chunk * rr_cam.zoom;

					SDL_FRect dst = {rc.x,rc.y,rs.x,rs.y};
					SDL_Texture *empty_chunk_tex = (*tex_map)[Tiles::EMPTY_CHUNK_KEY];
					SDL_RenderTexture(rr, empty_chunk_tex, NULL, &dst);
				}
			} else {
				std::cout << "tex_map not found\n";
			}
		}

		/* what the heck is a SDL_RenderGeometry()??? */ {
			Vec2 pos = GetRenderCoords(rr_cam, p->pos, wn_size);
			SDL_FColor col1 = {
				Colors::red.r / 255.0,
				Colors::red.g / 255.0,
				Colors::red.b / 255.0,
				Colors::red.a / 255.0
			};
			SDL_FColor col2 = {
				Colors::white.r / 255.0,
				Colors::white.g / 255.0,
				Colors::white.b / 255.0,
				Colors::white.a / 255.0
			};

			f32 zoom = rr_cam.zoom;
			RenderCircle(rr, pos, 4.5f * zoom, col1);
			RenderCircle(rr, pos, 3.0f * zoom, col2);
		} // SDL_RenderGeometry() test

        /* render bullets */ {
            for (const auto& bp : game->world.active_patterns) {
                for (const auto& b : bp.bullets) {

					SDL_FRect dst = GetScreenDSTFromWorld(
						rr_cam,
						b.pos,
						b.size,
						wn_size
					);

					SetDrawColor(rr, Colors::white);
                    SDL_RenderFillRect(rr, &dst);
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
#pragma endregion // Iteration

void SDL_AppQuit(void* appstate, SDL_AppResult result) {
    Game *state = (Game *)appstate;

    SDL_DestroyRenderer(game->rr);
    SDL_DestroyWindow(game->wn);
    delete game;
    SDL_Quit();
}
