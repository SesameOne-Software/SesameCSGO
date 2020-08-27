#pragma once
#include "../sdk/sdk.hpp"

namespace hooks {
	void __fastcall modify_eye_pos ( REG, vec3_t& pos );

	namespace old {
		extern decltype( &hooks::modify_eye_pos ) modify_eye_pos;
	}
}