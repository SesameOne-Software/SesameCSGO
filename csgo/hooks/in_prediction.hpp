#pragma once
#include <sdk.hpp>

namespace hooks {
	bool __fastcall in_prediction ( REG );

	namespace old {
		extern decltype( &in_prediction ) in_prediction;
	}

	namespace prediction {
		extern bool disable_sounds;
	}
}