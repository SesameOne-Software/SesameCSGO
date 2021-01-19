#pragma once
#include "../sdk/sdk.hpp"

namespace hooks {
	void __stdcall update_clientside_animations ( );

	namespace old {
		extern decltype( &hooks::update_clientside_animations ) update_clientside_animations;
	}
}
