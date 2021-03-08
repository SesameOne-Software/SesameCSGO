#pragma once
#include "../sdk/sdk.hpp"

namespace hooks {
	void __fastcall accumulate_layers( REG , void* setup , vec3_t& pos , void* q , float time );

	namespace old {
		extern decltype( &hooks::accumulate_layers ) accumulate_layers;
	}
}
