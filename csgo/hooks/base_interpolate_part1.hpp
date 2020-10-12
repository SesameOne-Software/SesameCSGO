#pragma once
#include "../sdk/sdk.hpp"

namespace hooks {
	int __fastcall base_interpolate_part1 ( REG, float& current_time, vec3_t& old_origin, vec3_t& old_angles, int& no_more_changes );

	namespace old {
		extern decltype( &hooks::base_interpolate_part1 ) base_interpolate_part1;
	}
}
