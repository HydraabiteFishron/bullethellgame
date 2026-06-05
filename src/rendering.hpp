#pragma once

#include <cassert>
#include <SDL3/SDL.h>

#include "customtypes.hpp"

constexpr Vec2 CENTER_ORIGIN = { 0.5f, 0.5f };

struct Camera {
    Vec2 pos = {0.0f,0.0f};
    f32 zoom = 1.0f;
};

inline Camera CreateCamera(Vec2 pos = {0.0f,0.0f}, f32 zoom = 1.0f) {
    return { pos, zoom };
}

#pragma region Rendering Math
Vec2 GetTopleft(const Vec2 pos, const Vec2 size, const Vec2 origin = CENTER_ORIGIN);
Vec2 GetRenderCoords(const Camera& cam, const Vec2 world_pos, const Vec2 screen_dimensions);
Vec2 GetRenderSize(const Vec2 size, const f32 zoom);
#pragma endregion // Rendering Math

#pragma region Rendering Helpers
SDL_FRect GetScreenDSTFromWorld(
	Camera cam,
	Vec2 world_pos,
	Vec2 world_size,
	Vec2 wn_size,
	Vec2 origin = CENTER_ORIGIN
);

void RenderCircle(
	SDL_Renderer *rr,
	Vec2 pos,
	f32 radius,
	SDL_FColor col = {1,1,1,1},
	int segments = 24
);
#pragma endregion // Rendering Helpers

// -------------------- // -------------------- // -------------------- //

#pragma region Inlined Definitions
inline Vec2 GetTopleft(const Vec2 pos, const Vec2 size, const Vec2 origin) {
    return pos - size * origin;
}

// for camera with center origin
inline Vec2 GetRenderCoords(const Camera& cam, const Vec2 world_pos, const Vec2 screen_dimensions) {
    assert(cam.zoom > 0.0f && "hey dipshit, camera zoom is set to 0.0f you fucking moron.");
    return (world_pos - cam.pos) * cam.zoom + (screen_dimensions * 0.5f);
}

inline Vec2 GetRenderSize(const Vec2 size, const f32 zoom) {
    return size * zoom;
}

inline SDL_FRect GetScreenDSTFromWorld(
	Camera cam,
	Vec2 world_pos,
	Vec2 world_size,
	Vec2 wn_size,
	Vec2 origin
) {
	Vec2 topleft = world_pos - world_size * origin;

    assert(cam.zoom > 0.0f && "cam zoom is <= 0");
	Vec2 render_coords =
		(topleft - cam.pos) * cam.zoom + (wn_size * 0.5f);

	Vec2 render_size = world_size * cam.zoom;

	return SDL_FRect {
		.x = render_coords.x,
		.y = render_coords.y,
		.w = render_size.x,
		.h = render_size.y
	};
}
#pragma endregion // Inlined Definitions
