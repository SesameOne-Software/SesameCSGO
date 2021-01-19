#include "update_clientside_animations.hpp"

decltype( &hooks::update_clientside_animations ) hooks::old::update_clientside_animations = nullptr;

/* stop game from updating animations */
void __stdcall hooks::update_clientside_animations ( ) {
	
}