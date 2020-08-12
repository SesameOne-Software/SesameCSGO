#include "setup_bones.hpp"

#include "../features/ragebot.hpp"
#include "../animations/animations.hpp"

bool hooks::bone_setup::allow = false;

decltype( &hooks::setup_bones ) hooks::old::setup_bones = nullptr;

bool __fastcall hooks::setup_bones ( REG, matrix3x4_t* out, int max_bones, int mask, float curtime ) {
	static auto setup_bones_ret = pattern::search ( _ ( "client.dll" ), _ ( "E8 ? ? ? ? 5E 5D C2 10 00 32 C0" ) ).add ( 5 ).get< int* > ( );

	const auto pl = reinterpret_cast< player_t* > ( uintptr_t ( ecx ) - 4 );
	
	/* remove to force matrix on local player */
	if( pl && pl == g::local )
		return animations::setup_bones ( pl, out, mask, vec3_t ( ), vec3_t ( ), curtime );
	
	if ( pl && pl->is_player ( ) && pl->idx ( ) > 0 && pl->idx ( ) <= 64 && out && !bone_setup::allow ) {
		if ( pl == g::local ) {
			memcpy ( out, &animations::data::fixed_bones [ g::local->idx ( ) ], sizeof ( matrix3x4_t ) * max_bones );
		}
		else {
			if ( features::ragebot::get_misses ( pl->idx ( ) ).bad_resolve % 3 == 0 )
				memcpy ( out, &animations::data::fixed_bones [ pl->idx ( ) ], sizeof ( matrix3x4_t ) * max_bones );
			else if ( features::ragebot::get_misses ( pl->idx ( ) ).bad_resolve % 3 == 1 )
				memcpy ( out, &animations::data::fixed_bones1 [ pl->idx ( ) ], sizeof ( matrix3x4_t ) * max_bones );
			else
				memcpy ( out, &animations::data::fixed_bones2 [ pl->idx ( ) ], sizeof ( matrix3x4_t ) * max_bones );
		}
	
		return true;
	}

	if ( _ReturnAddress ( ) == setup_bones_ret )
		return false;

	return old::setup_bones ( REG_OUT, out, max_bones, mask, curtime );
}