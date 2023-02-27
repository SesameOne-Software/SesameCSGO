#include "calc_view.hpp"

decltype( &hooks::calc_view ) hooks::old::calc_view = nullptr;

void __fastcall hooks::calc_view ( REG, vec3_t& eye_pos, vec3_t& eye_angles, float& z_near, float& z_far, float& fov ) {
	const auto player = reinterpret_cast< player_t* >( ecx );

	if ( !g::local || player != g::local )
		return old::calc_view ( REG_OUT, eye_pos, eye_angles, z_near, z_far, fov );

	const auto backup_use_new_animstate = *reinterpret_cast< bool* >( reinterpret_cast< uintptr_t >( player ) + 0x39E1 );

	*reinterpret_cast< bool* >( reinterpret_cast< uintptr_t >( player ) + 0x39E1 ) = false;

	old::calc_view ( REG_OUT, eye_pos, eye_angles, z_near, z_far, fov );

	*reinterpret_cast< bool* >( reinterpret_cast< uintptr_t >( player ) + 0x39E1 ) = backup_use_new_animstate;
}