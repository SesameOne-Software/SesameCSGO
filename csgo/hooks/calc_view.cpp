#include "calc_view.hpp"

decltype( &hooks::calc_view ) hooks::old::calc_view = nullptr;

void __fastcall hooks::calc_view ( REG, vec3_t& eye_pos, vec3_t& eye_angles, float& z_near, float& z_far, float& fov ) {
	static const auto use_new_animstate = pattern::search ( _ ( "client.dll" ), _ ( "88 87 ? ? ? ? 75 14" ) ).add( 2 ).deref( ).get < ptrdiff_t > ( );

	const auto player = reinterpret_cast< player_t* >( ecx );

	if ( !g::local || player != g::local )
		return old::calc_view ( REG_OUT, eye_pos, eye_angles, z_near, z_far, fov );

	const auto backup_use_new_animstate = *reinterpret_cast< bool* >( reinterpret_cast< uintptr_t >( player ) + use_new_animstate );

	*reinterpret_cast< bool* >( reinterpret_cast< uintptr_t >( player ) + use_new_animstate ) = false;

	old::calc_view ( REG_OUT, eye_pos, eye_angles, z_near, z_far, fov );

	*reinterpret_cast< bool* >( reinterpret_cast< uintptr_t >( player ) + use_new_animstate ) = backup_use_new_animstate;
}