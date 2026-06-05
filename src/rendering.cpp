#include "rendering.hpp"

void RenderCircle(
	SDL_Renderer *rr,
	Vec2 pos,
	f32 radius,
	SDL_FColor col,
	int segments
) {
	// I just found out because I'm currently getting a warning
	// but bro... C++ is scared of variable length arrays?????
	// are we serious here my man????? stack overflow ma balls bro.
	if (segments >= 32)
		segments = 32;
	SDL_Vertex vertices[segments + 2];
	int indices[segments * 3];

	size_t vc = 0;
	size_t ic = 0;

	vertices[vc] = {
		{ pos.x, pos.y },
		col,
		{ 0.0f, 0.0f }
	};
	vc++;

	for (int i = 0; i <= segments; ++i) {
		float angle = (float)i / f32(segments) * SDL_PI_F * 2.0f;

		constexpr SDL_FPoint zero_point = { 0.0f, 0.0f };
		SDL_FPoint _pos = {
			pos.x + std::cos(angle) * radius,
			pos.y + std::sin(angle) * radius
		};

		vertices[vc] =
			{ _pos, col, zero_point }; vc++;
	}

	for (int i = 1; i <= segments; ++i) {
		indices[ic] = 0;   ic++;
		indices[ic] = i;   ic++;
		indices[ic] = i+1; ic++;
	}

	SDL_RenderGeometry(
		rr,
		nullptr,
		vertices,
		segments + 2,
		indices,
		segments * 3
	);
}
