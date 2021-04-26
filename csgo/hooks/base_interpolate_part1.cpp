#include "base_interpolate_part1.hpp"

decltype( &hooks::base_interpolate_part1 ) hooks::old::base_interpolate_part1 = nullptr;

int __fastcall hooks::base_interpolate_part1 ( REG, float& current_time, vec3_t& old_origin, vec3_t& old_angles, int& no_more_changes ) {
	static auto move_to_last_received_pos = pattern::search ( _ ( "client.dll" ), _ ( "55 8B EC 51 53 56 8B F1 32 DB 8B 06" ) ).get<void ( __thiscall* )( void*, bool )> ( );

	auto pl = ( player_t* ) ecx;

	if ( pl && pl->is_player ( ) && pl != g::local && cs::time2ticks( abs( pl->simtime ( ) - pl->old_simtime ( ) ) ) > 2 ) {
		no_more_changes = 1;
		
		move_to_last_received_pos ( pl, false );

		return 0;
	}

	return old::base_interpolate_part1 ( REG_OUT, current_time, old_origin, old_angles, no_more_changes );
}