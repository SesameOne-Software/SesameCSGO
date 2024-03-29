#pragma once
#include "../sdk/sdk.hpp"

namespace hooks {
	bool __fastcall setup_bones ( REG, matrix3x4_t* out, int max_bones, int mask, float curtime );

	namespace bone_setup {
		extern bool allow;
	}

	namespace old {
		extern decltype( &hooks::setup_bones ) setup_bones;
	}
}