#pragma once
#include "../sdk/sdk.hpp"

namespace hooks {
	void __cdecl cs_blood_spray_callback ( const effect_data_t& effect_data );

	namespace old {
		extern decltype( &hooks::cs_blood_spray_callback ) cs_blood_spray_callback;
	}
}