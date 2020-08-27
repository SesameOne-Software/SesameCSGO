#pragma once
#include "../sdk/sdk.hpp"

namespace hooks {
	bool __fastcall in_prediction ( REG );

	namespace old {
		extern decltype( &hooks::in_prediction ) in_prediction;
	}

	namespace prediction {
		extern bool disable_sounds;
	}
}