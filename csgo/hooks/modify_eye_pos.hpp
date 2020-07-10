#pragma once
#include <sdk.hpp>

namespace hooks {
	void __fastcall modify_eye_pos ( REG, vec3_t& pos );

	namespace old {
		extern decltype( &modify_eye_pos ) modify_eye_pos;
	}
}