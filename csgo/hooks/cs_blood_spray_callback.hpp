#pragma once
#include <sdk.hpp>

namespace hooks {
	void __cdecl cs_blood_spray_callback ( const effect_data_t& effect_data );

	namespace old {
		extern decltype( &cs_blood_spray_callback ) cs_blood_spray_callback;
	}
}