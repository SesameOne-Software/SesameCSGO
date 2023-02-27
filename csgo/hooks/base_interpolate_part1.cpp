#include "base_interpolate_part1.hpp"

#include "../menu/options.hpp"

decltype( &hooks::base_interpolate_part1 ) hooks::old::base_interpolate_part1 = nullptr;

int __fastcall hooks::base_interpolate_part1 ( REG, float& current_time, vec3_t& old_origin, vec3_t& old_angles, int& no_more_changes ) {
	static auto& main_switch = options::vars [ _ ( "global.assistance_type" ) ].val.i;

	static auto move_to_last_received_pos = pattern::search ( _ ( "client.dll" ), _ ( "E8 ? ? ? ? 83 7E 5C 00" ) ).resolve_rip().get<void ( __thiscall* )( void*, bool )> ( );

	auto pl = reinterpret_cast< player_t* >( ecx );

	if ( pl && pl->is_player ( ) && pl != g::local && main_switch == 2 ) {
		no_more_changes = 1;
		move_to_last_received_pos ( pl, false );
		return 0;
	}

	return old::base_interpolate_part1 ( REG_OUT, current_time, old_origin, old_angles, no_more_changes );
}