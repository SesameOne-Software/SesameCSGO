#pragma once
#include "../sdk/sdk.hpp"

namespace hooks {
	void __fastcall notify_on_layer_change_weight( REG, const animlayer_t* layer , const float new_weight );

	namespace old {
		extern decltype( &hooks::notify_on_layer_change_weight ) notify_on_layer_change_weight;
	}
}
