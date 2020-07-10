#pragma once
#include <sdk.hpp>

namespace hooks {
	bool __fastcall setup_bones ( REG, matrix3x4_t* out, int max_bones, int mask, float curtime );

	namespace old {
		extern decltype( &setup_bones ) setup_bones;
	}
}