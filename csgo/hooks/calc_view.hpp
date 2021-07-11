#pragma once
#include "../sdk/sdk.hpp"

namespace hooks {
	void __fastcall calc_view ( REG, vec3_t& eye_pos, vec3_t& eye_angles, float& z_near, float& z_far, float& fov );

	namespace old {
		extern decltype( &hooks::calc_view ) calc_view;
	}
}