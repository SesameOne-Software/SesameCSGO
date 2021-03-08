#pragma once
#include "../sdk/sdk.hpp"

namespace hooks {
	void __fastcall notify_on_layer_change_cycle( REG , const animlayer_t* layer , const float new_cycle );

	namespace old {
		extern decltype( &hooks::notify_on_layer_change_cycle ) notify_on_layer_change_cycle;
	}
}
