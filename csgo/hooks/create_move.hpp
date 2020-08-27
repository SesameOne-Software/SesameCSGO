#pragma once
#include "../sdk/sdk.hpp"

namespace hooks {
	bool __fastcall create_move( REG, float sampletime, ucmd_t* ucmd );

	namespace old {
		extern decltype( &hooks::create_move ) create_move;
	}

	namespace vars {
		extern bool in_refresh;
	}
}