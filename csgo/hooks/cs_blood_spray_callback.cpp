#include "cs_blood_spray_callback.hpp"

#include "../animations/resolver.hpp"

decltype( &hooks::cs_blood_spray_callback ) hooks::old::cs_blood_spray_callback = nullptr;

void __cdecl hooks::cs_blood_spray_callback ( const effect_data_t& effect_data ) {
	//animations::resolver::process_blood ( effect_data );

	old::cs_blood_spray_callback ( effect_data );
}