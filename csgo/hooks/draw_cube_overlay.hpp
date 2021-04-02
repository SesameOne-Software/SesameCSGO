#pragma once
#include "../sdk/sdk.hpp"

namespace hooks {
	void __fastcall draw_cube_overlay ( REG, const vec3_t& origin, const vec3_t& mins, const vec3_t& maxs, vec3_t const& angles, int r, int g, int b, int a, float duration );

	namespace old {
		extern decltype( &hooks::draw_cube_overlay ) draw_cube_overlay;
	}
}
