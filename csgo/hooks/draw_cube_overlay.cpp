#include "draw_cube_overlay.hpp"

decltype( &hooks::draw_cube_overlay ) hooks::old::draw_cube_overlay = nullptr;

void __fastcall hooks::draw_cube_overlay ( REG, const vec3_t& origin, const vec3_t& mins, const vec3_t& maxs, vec3_t const& angles, int r, int g, int b, int a, float duration ) {
	if ( r == 255 )
		dbg_print ( _ ( "clientsided_shot: 0x%X\n" ), _ReturnAddress ( ) );
	else
		dbg_print ( _ ( "serversided_shot: 0x%X\n" ), _ReturnAddress ( ) );

	r = 255;
	g = 100;
	b = 255;
	a = 100;

	old::draw_cube_overlay ( REG_OUT, origin, mins, maxs, angles, r, g, b, a, duration );
}