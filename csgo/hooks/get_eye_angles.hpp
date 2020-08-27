#pragma once
#include "../sdk/sdk.hpp"

namespace hooks {
	vec3_t& __fastcall get_eye_angles ( REG );

	namespace old {
		extern decltype( &hooks::get_eye_angles ) get_eye_angles;
	}
}