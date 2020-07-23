#include <array>
#include "animations.hpp"
#include "../globals.hpp"
#include "../security/security_handler.hpp"
#include "../features/lagcomp.hpp"
#include "resolver.hpp"
#include "../features/esp.hpp"
#include "../features/lagcomp.hpp"
#include "../menu/menu.hpp"
#include "../features/ragebot.hpp"
#include "../hooks/setup_bones.hpp"

namespace local_data {
	float ground_time = 0.0f;
	bool was_on_ground = true;
	int old_tick = 0;
	float abs = 0.0f;
	float hit_ground_time = 0.0f;
	std::array< float, 24 > poses { };
	std::array< animlayer_t, 15 > overlays { };
	float old_update = 0.0f;

	namespace fake {
		bool should_reset = true;
		float spawn_time = 0.0f;
		std::array< matrix3x4_t, 128 > simulated_mat { };
	}
}

namespace animations {
	namespace data {
		namespace simulated {
			std::array< float, 65 > animtimes { 0.0f };
			std::array< int, 65 > flags { 0 };
			std::array< vec3_t, 65 > origins { vec3_t ( 0.0f, 0.0f, 0.0f ) };
			std::array< vec3_t, 65 > velocities { vec3_t ( 0.0f, 0.0f, 0.0f ) };
			std::array< animstate_t, 65 > animstates { };
			std::array< matrix3x4_t [ 128 ], 65 > bones { };
			std::array< float [ 24 ], 65 > poses { };
			std::array< animlayer_t [ 15 ], 65 > overlays { };
		}

		std::array< animstate_t, 65 > fake_states { };
		std::array< vec3_t, 65 > origin { vec3_t ( FLT_MAX, FLT_MAX, FLT_MAX ) };
		std::array< vec3_t, 65 > old_origin { vec3_t ( FLT_MAX, FLT_MAX, FLT_MAX ) };
		std::array< std::array< matrix3x4_t, 128 >, 65 > bones;
		std::array< std::array< matrix3x4_t, 128 >, 65 > fixed_bones;
		std::array< std::array< matrix3x4_t, 128 >, 65 > fixed_bones1;
		std::array< std::array< matrix3x4_t, 128 >, 65 > fixed_bones2;
		std::array< std::array< float, 24 >, 65 > poses { };
		std::array< int, 65 > last_animation_frame { 0 };
		std::array< int, 65 > old_tick { 0 };
		std::array< int, 65 > choke { 0 };
		std::array< float, 65 > old_simtime { 0.0f };
		std::array< float, 65 > old_eye_yaw { 0.0f };
		std::array< float, 65 > resolved1 { 0.0f };
		std::array< float, 65 > resolved2 { 0.0f };
		std::array< float, 65 > resolved3 { 0.0f };
		std::array< float, 65 > old_abs { 0.0f };
		std::array< float, 65 > body_yaw { 0.0f };
		std::array< std::array< animlayer_t, 15 >, 65 > overlays { };
	}
}

animstate_t animations::fake::fake_state;

float animations::rebuilt::poses::jump_fall( player_t* pl, float air_time ) {
	const float airtime = ( air_time - 0.72f ) * 1.25f;
	const float clamped = airtime >= float( N( 0 ) ) ? std::min< float >( airtime, float( N( 1 ) ) ) : float( N( 0 ) );
	float jump_fall = ( float( N( 3 ) ) - ( clamped + clamped ) ) * ( clamped * clamped );

	if ( jump_fall >= float( N( 0 ) ) )
		jump_fall = std::min< float >( jump_fall, float( N( 1 ) ) );

	return jump_fall;
}

float animations::rebuilt::poses::body_pitch( player_t* pl, float pitch ) {
	auto eye_pitch_normalized = csgo::normalize ( pitch );
	auto new_body_pitch_pose = 0.0f;

	if ( eye_pitch_normalized <= 0.0f )
		new_body_pitch_pose = eye_pitch_normalized / pl->animstate ( )->m_min_pitch;
	else
		new_body_pitch_pose = eye_pitch_normalized / pl->animstate ( )->m_max_pitch;

	return new_body_pitch_pose;
}

float animations::rebuilt::poses::body_yaw( player_t* pl, float yaw ) {
	auto eye_goalfeet_delta = csgo::normalize ( csgo::normalize( yaw ) - csgo::normalize( pl->animstate()->m_abs_yaw ) );
	auto new_body_yaw_pose = 0.0f;

	if ( eye_goalfeet_delta < 0.0f || pl->animstate()->m_max_yaw == 0.0f ) {
		if ( pl->animstate ( )->m_min_yaw != 0.0f )
			new_body_yaw_pose = eye_goalfeet_delta / pl->animstate ( )->m_min_yaw;
	}
	else {
		new_body_yaw_pose = eye_goalfeet_delta / pl->animstate ( )->m_max_yaw;
	}

	return new_body_yaw_pose;
}

float animations::rebuilt::poses::lean_yaw( player_t* pl, float yaw ) {
	return csgo::normalize( pl->animstate( )->m_abs_yaw - yaw );
}

void animations::rebuilt::poses::calculate( player_t* pl ) {
	auto state = pl->animstate( );
	auto layers = pl->layers( );

	if ( !state || !layers )
		return;

	state->m_lean_yaw_pose.set_value ( pl, ( csgo::normalize ( pl->angles ( ).y ) + 180.0f ) / 360.0f );
	state->m_move_blend_walk_pose.set_value ( pl, ( 1.0f - data::overlays [ pl->idx ( ) ][ N ( 6 ) ].m_weight ) * ( 1.0f - pl->crouch_amount ( ) ) );
	state->m_move_blend_run_pose.set_value ( pl, ( 1.0f - pl->crouch_amount ( ) ) * data::overlays [ pl->idx ( ) ][ N ( 6 ) ].m_weight );
	state->m_move_blend_crouch_pose.set_value ( pl, pl->crouch_amount ( ) );
	state->m_jump_fall_pose.set_value ( pl, rebuilt::poses::jump_fall ( pl, data::overlays [ pl->idx ( ) ][ N ( 4 ) ].m_cycle ) );
	state->m_body_pitch_pose.set_value ( pl, ( csgo::normalize ( state->m_pitch ) + 90.0f ) / 180.0f );
	state->m_body_yaw_pose.set_value ( pl, std::clamp ( csgo::normalize ( csgo::normalize ( state->m_eye_yaw ) - csgo::normalize ( state->m_abs_yaw ) ), -60.0f, 60.0f ) / 120.0f + 0.5f );
}

void animations::estimate_vel ( player_t* pl, vec3_t& out ) {
	const auto ground_fraction = data::overlays [ pl->idx ( ) ][ N ( 6 ) ].m_weight;
	const auto max_speed = 260.0f;
	const auto normal_vec = out.normalized ( );

	out = normal_vec * ( ground_fraction * max_speed );
}

std::array< matrix3x4_t, 128 >& animations::fake::matrix( ) {
	return local_data::fake::simulated_mat;
}

void update_animations ( player_t* pl ) {
	if ( pl->animstate ( )->m_last_clientside_anim_framecount == csgo::i::globals->m_framecount )
		pl->animstate ( )->m_last_clientside_anim_framecount--;

	const auto backup_frametime = csgo::i::globals->m_frametime;
	const auto backup_interp = csgo::i::globals->m_interp;

	csgo::i::globals->m_frametime = csgo::i::globals->m_ipt;
	csgo::i::globals->m_interp = 0.0f;

	pl->animate ( ) = true;
	pl->update ( );
	pl->animate ( ) = false;

	csgo::i::globals->m_frametime = backup_frametime;
	csgo::i::globals->m_interp = backup_interp;
}

int animations::fake::simulate ( ) {
	static int curtick = 0;
	if ( !g::local || !g::local->alive ( ) || !g::local->layers ( ) || !g::local->renderable ( ) ) {
		local_data::fake::should_reset = true;
		curtick = 0;
	return 0;
}

if ( local_data::fake::should_reset || local_data::fake::spawn_time != g::local->spawn_time ( ) ) {
	local_data::fake::spawn_time = g::local->spawn_time ( );
	g::local->create_animstate ( &fake_state );
	local_data::fake::should_reset = false;
	curtick = 0;
}

if ( g::send_packet && csgo::i::globals->m_tickcount > curtick ) {
	curtick = csgo::i::globals->m_tickcount;
	*reinterpret_cast< int* >( uintptr_t ( g::local ) + 0xA68 ) = 0;

	std::array< animlayer_t, 15 > backup_overlays;
	std::memcpy ( &backup_overlays, g::local->layers ( ), N ( sizeof animlayer_t ) * g::local->num_overlays ( ) );

	const float backup_frametime = csgo::i::globals->m_frametime;
	const auto backup_poses = g::local->poses ( );
	auto backup_abs_angles = g::local->abs_angles ( );

	fake_state.m_feet_yaw_rate = 0.0f;
	fake_state.m_unk_feet_speed_ratio = 0.0f;
	fake_state.m_last_clientside_anim_framecount = 0;
	csgo::i::globals->m_frametime = csgo::i::globals->m_ipt;
	fake_state.update ( g::sent_cmd.m_angs );
	csgo::i::globals->m_frametime = backup_frametime;

	KEYBIND ( fd_key, "Sesame->B->Other->Other->Fakeduck Key" );

	if ( !csgo::i::input->m_camera_in_thirdperson ) {
		fake_state.m_duck_amount = fd_key ? 0.0f : g::local->crouch_amount ( );
		fake_state.m_unk_feet_speed_ratio = g::local->crouch_speed ( );
	}

	setup_bones( g::local, reinterpret_cast< matrix3x4_t* > ( &local_data::fake::simulated_mat ), N ( 0x7ff00 ), vec3_t( 0.0f, fake_state.m_abs_yaw, 0.0f ), g::local->origin( ), csgo::i::globals->m_curtime );

	const auto render_origin = g::local->render_origin ( );

	auto i = N ( 0 );

	for ( i = N ( 0 ); i < N ( 128 ); i++ )
		local_data::fake::simulated_mat [ i ].set_origin ( local_data::fake::simulated_mat [ i ].origin ( ) - render_origin );

		g::local->abs_angles ( )= backup_abs_angles;
	g::local->poses ( ) = backup_poses;
	std::memcpy ( g::local->layers ( ), &backup_overlays, N ( sizeof animlayer_t ) * g::local->num_overlays ( ) );

}
}

<<<<<<< HEAD
bool animations::setup_bones ( player_t* target, matrix3x4_t* mat, int mask, vec3_t rotation, vec3_t origin, float time ) {
	const auto backup_mask_1 = *reinterpret_cast< int* >( uintptr_t ( target ) + N ( 0x269C ) );
	const auto backup_mask_2 = *reinterpret_cast< int* >( uintptr_t ( target ) + N ( 0x26B0 ) );
	const auto backup_flags = *reinterpret_cast< int* >( uintptr_t ( target ) + N ( 0xe8 ) );
	const auto backup_effects = *reinterpret_cast< int* >( uintptr_t ( target ) + N ( 0xf0 ) );
	const auto backup_use_pred_time = *reinterpret_cast< int* >( uintptr_t ( target ) + N ( 0x2ee ) );
	auto backup_abs_origin = target->abs_origin ( );
	auto backup_abs_angle = target->abs_angles ( );
=======
/*
@CBRS

bool setup_bones( player_t *target, mat3x4_t *mat, int mask, vec_t rotation, vec_t origin, float time ) {
		auto cstudio = ( c_studio_hdr * ) target->cstudio( );
		if ( !cstudio )
			return false;

		//  we need an aligned matrix in the bone accessor, so do this :) bad performance cause memcpy but that's ok
		mat3x4a_t used[128];

		//	output shit
		byte bone_computed[0x100] = { 0 };

		//	needs to be aligned
		alignas( 16 ) mat3x4_t base_matrix;
		math::angle_matrix( rotation, origin, base_matrix );

		//	store shit
		auto old_bones = target->bones( );
		auto iks = target->iks( );
		target->effects( ) |= 8;

		//	clear iks & re-create them
		if ( iks ) {
			if ( *( DWORD * ) ( uintptr_t( iks ) + 0xFF0 ) > 0 ) {
				int v1 = 0;
				auto v62 = ( DWORD * ) ( uintptr_t( iks ) + 0xD0 );
				do {
					*v62 = -9999;
					v62 += 0x55;
					++v1;
				} while ( v1 < *( DWORD * ) ( uintptr_t( iks ) + 0xFF0 ));
			}

			typedef void ( __thiscall *init_iks_fn )( void *, c_studio_hdr *, vec_t &, vec_t &, float, int, int );
			static auto init_iks = backend::memory::pattern( backend::memory::files.client, "55 8B EC 83 EC 08 8B 45 08 56 57 8B F9 8D" ).cast< init_iks_fn >( );
			init_iks( iks, cstudio, rotation, origin, time, cs::globals->frame_count, 0x1000 );
		}

		vec_t pos[128];
		ZeroMemory ( pos, sizeof( vec_t[128] ));
		quaternion_t q[128];
		ZeroMemory ( q, sizeof( quaternion_t[128] ));

		//	set flags and bones
		target->bones( ) = used;

		//	build some shit
		target->standard_blending_rules( cstudio, pos, q, time, mask );

		//	set iks
		if ( iks ) {
			typedef void ( __thiscall *fn )( void *, vec_t *, quaternion_t *, void *, byte * );
			static auto update_targets = backend::memory::pattern( backend::memory::files.client, "55 8B EC 83 E4 F0 81 EC 18 01 00 00 33" ).cast< fn >( );
			static auto solve_dependencies = backend::memory::pattern( backend::memory::files.client, "55 8B EC 83 E4 F0 81 EC 98 05 00 00 8B 81" ).cast< fn >( );

			target->update_ik_locks( time );
			update_targets( iks, pos, q, target->bones( ), bone_computed );
			target->calculate_ik_locks( time );
			solve_dependencies( iks, pos, q, target->bones( ), bone_computed );
		}

		//	build the matrix
		target->build_transformations( cstudio, pos, q, base_matrix, mask, bone_computed );

		//	restore flags and bones
		target->effects( ) &= ~8;
		target->bones( ) = old_bones;

		//  and pop out our new matrix
		memcpy( mat, used, sizeof( mat3x4_t ) * 128 );

		return true;
	}

*/

bool animations::build_matrix( player_t* pl, matrix3x4_t* out, int max_bones, int mask, float seed ) {
	const auto backup_mask_1 = *reinterpret_cast< int* >( uintptr_t( pl ) + N( 0x269C ) );
	const auto backup_mask_2 = *reinterpret_cast< int* >( uintptr_t( pl ) + N( 0x26B0 ) );
	const auto backup_flags = *reinterpret_cast< int* >( uintptr_t( pl ) + N( 0xe8 ) );
	const auto backup_effects = *reinterpret_cast< int* >( uintptr_t( pl ) + N( 0xf0 ) );
	const auto backup_use_pred_time = *reinterpret_cast< int* >( uintptr_t( pl ) + N( 0x2ee ) );
	auto backup_abs_origin = pl->abs_origin( );
>>>>>>> b3ebc1ec849f3f15140c954ebc5447282cf2bf4a

	*reinterpret_cast< int* >( uintptr_t ( target ) + 0xA68 ) = 0;

	*reinterpret_cast< int* >( uintptr_t ( target ) + N ( 0x26AC ) ) = 0;
	*reinterpret_cast< int* >( uintptr_t ( target ) + N ( 0xe8 ) ) |= N ( 8 );

	/* disable matrix interpolation */
	*reinterpret_cast< int* >( uintptr_t ( target ) + N ( 0xf0 ) ) |= N ( 8 );

	/* use our setup time */
	//*reinterpret_cast< bool* >( uintptr_t ( target ) + N ( 0x2ee ) ) = true;

	/* use uninterpolated origin */
	if ( target != g::local ) {
		target->set_abs_angles ( rotation );
		target->set_abs_origin ( origin );
	}

	//if ( !out )
	target->inval_bone_cache ( );

	const auto backup_curtime = csgo::i::globals->m_curtime;
	const auto backup_frametime = csgo::i::globals->m_frametime;
	const auto backup_framecount = csgo::i::globals->m_framecount;

	csgo::i::globals->m_framecount = INT_MAX;
	csgo::i::globals->m_curtime = csgo::i::globals->m_curtime;
	csgo::i::globals->m_frametime = csgo::i::globals->m_ipt;

	hooks::bone_setup::allow = true;
	const auto ret = hooks::old::setup_bones ( target->renderable(), nullptr, mat, 128, mask, time );
	hooks::bone_setup::allow = false;

	csgo::i::globals->m_framecount = backup_framecount;
	csgo::i::globals->m_curtime = backup_curtime;
	csgo::i::globals->m_frametime = backup_frametime;

	*reinterpret_cast< int* >( uintptr_t ( target ) + N ( 0xe8 ) ) = backup_flags;
	*reinterpret_cast< int* >( uintptr_t ( target ) + N ( 0xf0 ) ) = backup_effects;

	if ( target != g::local ) {
		target->set_abs_angles ( backup_abs_angle );
		target->set_abs_origin ( backup_abs_origin );
	}

	return ret;
}

//bool animations::setup_bones ( player_t* target, matrix3x4_t* mat, int mask, vec3_t rotation, vec3_t origin, float time ) {
//	/* vfunc indices */
//	static auto standard_blending_rules_vfunc_idx = pattern::search ( _ ( "client.dll" ), _ ( "FF 90 ? ? ? ? 8B 47 FC" ) ).add ( 2 ).deref ( ).get< uint32_t > ( ) / 4;
//	static auto build_transformations_vfunc_idx = pattern::search ( _ ( "client.dll" ), _ ( "FF 90 ? ? ? ? 8B 8F ? ? ? ? 8D 84 24 ? ? ? ? 50" ) ).add( 2 ).deref( ).get< uint32_t > ( ) / 4;
//	static auto update_ik_locks_vfunc_idx = pattern::search ( _ ( "client.dll" ), _ ( "FF 90 ? ? ? ? 8B 8F ? ? ? ? 8D 84 24 ? ? ? ? 50" ) ).add ( 2 ).deref ( ).get< uint32_t > ( ) / 4;
//	static auto calculate_ik_locks_vfunc_idx = pattern::search ( _ ( "client.dll" ), _ ( "FF 90 ? ? ? ? 8B 8F ? ? ? ? 8D 84 24 ? ? ? ? 50 FF B7 ? ? ? ? 8D 84 24 ? ? ? ? 50 8D 84 24 ? ? ? ? 50 E8 ? ? ? ? 8B 47 FC 8D 4F FC 8B 80" ) ).add ( 2 ).deref ( ).get< uint32_t > ( ) / 4;
//
//	/* func sigs */
//	static auto init_iks = pattern::search ( _ ( "client.dll" ), _ ( "55 8B EC 83 EC 08 8B 45 08 56 57 8B F9 8D" ) ).get< void ( __thiscall* )( void*, studiohdr_t*, vec3_t&, vec3_t&, float, int, int ) > ( );
//	static auto update_targets = pattern::search ( _ ( "client.dll" ), _ ( "55 8B EC 83 E4 F0 81 EC 18 01 00 00 33" ) ).get< void ( __thiscall* )( void*, vec3_t*, void*, void*, uint8_t* ) > ( );
//	static auto solve_dependencies = pattern::search ( _ ( "client.dll" ), _ ( "55 8B EC 83 E4 F0 81 EC 98 05 00 00 8B 81" ) ).get< void ( __thiscall* )( void*, vec3_t*, void*, void*, uint8_t* ) > ( );
//
//	/* offset sigs */
//	static auto iks_off = pattern::search ( _ ( "client.dll" ), _ ( "8D 47 FC 8B 8F" ) ).add ( 5 ).deref ( ).add ( 4 ).get< uint32_t > ( );
//	static auto effects_off = pattern::search ( _ ( "client.dll" ), _ ( "75 0D 8B 87" ) ).add ( 4 ).deref ( ).add ( 4 ).get< uint32_t > ( );
//
//	auto cstudio = *reinterpret_cast< studiohdr_t** >( uintptr_t ( target ) + 0x294C );
//	
//	if ( !cstudio )
//		return false;
//
//	//  we need an aligned matrix in the bone accessor, so do this :) bad performance cause memcpy but that's ok
//	matrix3x4_t used [ 128 ];
//
//	//	output shit
//	uint8_t bone_computed [ 0x100 ] = { 0 };
//
//	//	needs to be aligned
//	alignas( 16 ) matrix3x4_t base_matrix;
//
//	csgo::angle_matrix ( rotation, origin, base_matrix );
//
//	//	store shit
//	matrix3x4_t old_bones [ 128 ];
//	memcpy ( old_bones, target->bone_accessor ( ).get_bone_arr_for_write ( ), sizeof(matrix3x4_t) * target->bone_count());
//
//	auto iks = *reinterpret_cast< void** >( uintptr_t ( target ) + iks_off );
//	*reinterpret_cast< int* >( uintptr_t ( target ) + effects_off ) |= 8;
//
//	//	clear iks & re-create them
//	if ( iks ) {
//		if ( *( uint32_t* ) ( uintptr_t ( iks ) + 0xFF0 ) > 0 ) {
//			int v1 = 0;
//			auto v62 = ( uint32_t* ) ( uintptr_t ( iks ) + 0xD0 );
//
//			do {
//				*v62 = -9999;
//				v62 += 0x55;
//				++v1;
//			} while ( v1 < *( uint32_t* ) ( uintptr_t ( iks ) + 0xFF0 ) );
//		}
//
//		init_iks ( iks, cstudio, rotation, origin, time, csgo::i::globals->m_framecount, 0x1000 );
//	}
//
//	vec3_t pos [ 128 ];
//	memset ( pos, 0, sizeof ( vec3_t [ 128 ] ) );
//
//	void* q = malloc ( /* sizeof quaternion_t */ 48 * 128 );
//	memset ( q, 0, /* sizeof quaternion_t */ 48 * 128 );
//
//	//	set flags and bones
//	memcpy ( target->bone_accessor ( ).get_bone_arr_for_write ( ), used, sizeof ( matrix3x4_t ) * target->bone_count ( ) );
//
//	//	build some shit
//	// IDX = 205 ( 2020.7.10 )
//	vfunc< void ( __thiscall* )( player_t*, studiohdr_t*, vec3_t*, void*, float, uint32_t ) > ( target, standard_blending_rules_vfunc_idx ) ( target, cstudio, pos, q, time, mask );
//
//	//	set iks
//	if ( iks ) {
//		vfunc< void ( __thiscall* )( player_t*, float ) > ( target, update_ik_locks_vfunc_idx ) ( target, time );
//		update_targets ( iks, pos, q, target->bone_accessor ( ).get_bone_arr_for_write ( ), bone_computed );
//		vfunc< void ( __thiscall* )( player_t*, float ) > ( target, calculate_ik_locks_vfunc_idx ) ( target, time );
//		solve_dependencies ( iks, pos, q, target->bone_accessor ( ).get_bone_arr_for_write ( ), bone_computed );
//	}
//
//	//	build the matrix
//	// IDX = 189 ( 2020.7.10 )
//	vfunc< void ( __thiscall* )( player_t*, studiohdr_t*, vec3_t*, void*, matrix3x4_t const&, uint32_t, void* ) > ( target, build_transformations_vfunc_idx ) ( target, cstudio, pos, q, base_matrix, mask, bone_computed );
//
//	free ( q );
//
//	//	restore flags and bones
//	*reinterpret_cast< int* >( uintptr_t ( target ) + effects_off ) &= ~8;
//	memcpy ( target->bone_accessor ( ).get_bone_arr_for_write ( ), old_bones, sizeof ( matrix3x4_t ) * target->bone_count ( ) );
//
//	//  and pop out our new matrix
//	memcpy ( mat ? mat : target->bone_accessor ( ).get_bone_arr_for_write ( ), used, sizeof ( matrix3x4_t ) * target->bone_count ( ) );
//
//	return true;
//}

auto last_vel2d = 0.0f;
auto last_update_time = 0.0f;
auto updated_tick = 0;
auto recalc_weight = 0.0f;
auto recalc_cycle = 0.0f;
auto last_local_update = 0.0f;

<<<<<<< HEAD
/*

*/

int animations::store_local ( ) {
	if ( !g::local || !g::local->animstate ( ) )
		return -1;

	const auto animstate = g::local->animstate ( );

	if ( g::local->simtime ( ) != last_local_update ) {
		g::local->layers ( ) [ 12 ].m_weight = 0.0f;
		//g::local->layers ( ) [ 4 ].m_cycle = recalc_cycle;

		std::memcpy ( &local_data::overlays, g::local->layers ( ), N ( sizeof animlayer_t ) * g::local->num_overlays ( ) );
		std::memcpy ( &data::overlays [ g::local->idx ( ) ], g::local->layers ( ), N ( sizeof animlayer_t ) * g::local->num_overlays ( ) );
	}

	last_local_update = g::local->simtime ( );

	return 0;
}

int animations::restore_local ( ) {
=======
int animations::fix_local ( ) {
	/*
	@CBRS
		here's a tidbit. guess where animations get ran? post frame.
		what's that mean?
		after run command is called
		so just update animations for each usrcmd parsed in run command

	*/

>>>>>>> b3ebc1ec849f3f15140c954ebc5447282cf2bf4a
	/* update viewmodel manually */ {
		using update_all_viewmodel_addons_t = int ( __fastcall* )( void* );
		static auto update_all_viewmodel_addons = pattern::search ( _ ( "client.dll" ), _ ( "55 8B EC 83 E4 ? 83 EC ? 53 8B D9 56 57 8B 03 FF 90 ? ? ? ? 8B F8 89 7C 24 ? 85 FF 0F 84 ? ? ? ? 8B 17 8B CF" ) ).get< update_all_viewmodel_addons_t > ( );

		if ( g::local && g::local->viewmodel_handle ( ) != -1 && csgo::i::ent_list->get_by_handle< void* > ( g::local->viewmodel_handle ( ) ) )
			update_all_viewmodel_addons ( csgo::i::ent_list->get_by_handle< void* > ( g::local->viewmodel_handle ( ) ) );
	}

	if ( !g::local->valid ( ) || !g::local->animstate ( ) || !g::local->layers ( ) || !g::ucmd ) {
		local_data::old_tick = 0;

		if ( g::local ) {
			data::origin [ g::local->idx ( ) ] = vec3_t ( FLT_MAX, FLT_MAX, FLT_MAX );
			data::old_origin [ g::local->idx ( ) ] = vec3_t ( FLT_MAX, FLT_MAX, FLT_MAX );
		}

		return 0;
	}

	if ( g::ucmd->m_buttons & 1 ) {
		g::angles = g::ucmd->m_angs;
		g::hold_aim = true;
	}

	if ( !g::hold_aim ) {
		g::angles = g::ucmd->m_angs;
	}

	const auto state = g::local->animstate ( );

	*reinterpret_cast< int* >( uintptr_t ( g::local ) + 0xA68 ) = 0;
	const auto backup_flags = g::local->flags ( );

	if ( g::local->flags ( ) & 1 )
		local_data::was_on_ground = true;

	// if ( csgo::i::globals->m_tickcount > local_data::old_tick ) {
	if ( csgo::i::globals->m_tickcount > local_data::old_tick ) {
		local_data::old_tick = csgo::i::globals->m_tickcount;

		const auto run_fraction = g::local->vel ( ).length_2d ( ) / ( ( g::local->weapon ( ) && g::local->weapon ( )->data ( ) ) ? g::local->weapon ( )->data ( )->m_max_speed : 260.0f );

		std::memcpy ( g::local->layers ( ), &local_data::overlays, N ( sizeof animlayer_t ) * g::local->num_overlays ( ) );

		update_animations ( g::local );

		rebuilt::poses::calculate ( g::local );

		if ( g::send_packet && updated_tick != csgo::i::globals->m_tickcount ) {
			recalc_weight = ( g::local->flags ( ) & 1 ) ? run_fraction : 0.0f;
			recalc_cycle = std::fabsf ( last_update_time - local_data::hit_ground_time );

			updated_tick = csgo::i::globals->m_tickcount;

			if ( local_data::was_on_ground ) {
				local_data::was_on_ground = false;
				local_data::hit_ground_time = csgo::i::globals->m_curtime;
			}

			last_vel2d = g::local->vel ( ).length_2d ( );
			last_update_time = csgo::i::globals->m_curtime;

			local_data::abs = state->m_abs_yaw;
			local_data::poses = g::local->poses ( );
			local_data::old_update = features::prediction::predicted_curtime;

			/* store old origin for lag-comp break check */
			data::old_origin [ g::local->idx ( ) ] = data::origin [ g::local->idx ( ) ];
			data::origin [ g::local->idx ( ) ] = g::local->origin ( );

			g::hold_aim = false;
		}
	}

	g::local->set_abs_angles ( vec3_t ( 0.0f, local_data::abs, 0.0f ) );

	state->m_feet_yaw_rate = 0.0f;
	state->m_unk_feet_speed_ratio = 0.0f;

	std::memcpy ( g::local->layers ( ), &local_data::overlays, N ( sizeof animlayer_t ) * g::local->num_overlays ( ) );
	g::local->poses ( ) = local_data::poses;
	//g::local->flags ( ) = backup_flags;

	//KEYBIND ( fd_key, "Sesame->B->Other->Other->Fakeduck Key" );
	//
	//if ( !csgo::i::input->m_camera_in_thirdperson ) {
	//	state->m_duck_amount = fd_key ? 0.0f : g::local->crouch_amount ( );
	//	state->m_unk_feet_speed_ratio = g::local->crouch_speed ( );
	//}

	setup_bones ( g::local, nullptr, N ( 0x7ff00 ), vec3_t ( 0.0f, local_data::abs, 0.0f ), g::local->abs_origin ( ), csgo::i::globals->m_curtime );
	
	if ( g::local->bone_accessor ( ).get_bone_arr_for_write ( ) )
		std::memcpy ( &data::fixed_bones [ g::local->idx ( ) ], g::local->bone_accessor ( ).get_bone_arr_for_write ( ), sizeof ( matrix3x4_t ) * g::local->bone_count ( ) );
	//features::lagcomp::cache( g::local );
}

//int animations::fix_local ( ) {
//	/* update viewmodel manually */ {
//		using update_all_viewmodel_addons_t = int ( __fastcall* )( void* );
//		static auto update_all_viewmodel_addons = pattern::search ( _ ( "client.dll" ), _ ( "55 8B EC 83 E4 ? 83 EC ? 53 8B D9 56 57 8B 03 FF 90 ? ? ? ? 8B F8 89 7C 24 ? 85 FF 0F 84 ? ? ? ? 8B 17 8B CF" ) ).get< update_all_viewmodel_addons_t > ( );
//
//		if ( g::local && g::local->viewmodel_handle ( ) != -1 && csgo::i::ent_list->get_by_handle< void* > ( g::local->viewmodel_handle ( ) ) )
//			update_all_viewmodel_addons ( csgo::i::ent_list->get_by_handle< void* > ( g::local->viewmodel_handle ( ) ) );
//	}
//
//	if ( !g::local->valid ( ) || !g::local->animstate ( ) || !g::local->layers ( ) || !g::ucmd ) {
//		local_data::old_tick = 0;
//
//		if ( g::local ) {
//			data::origin [ g::local->idx ( ) ] = vec3_t ( FLT_MAX, FLT_MAX, FLT_MAX );
//			data::old_origin [ g::local->idx ( ) ] = vec3_t ( FLT_MAX, FLT_MAX, FLT_MAX );
//		}
//
//		return 0;
//	}
//
//	if ( g::ucmd->m_buttons & 1 ) {
//		g::angles = g::ucmd->m_angs;
//		g::hold_aim = true;
//	}
//
//	if ( !g::hold_aim ) {
//		g::angles = g::ucmd->m_angs;
//	}
//
//	const auto state = g::local->animstate ( );
//
//	*reinterpret_cast< int* >( uintptr_t ( g::local ) + 0xA68 ) = 0;
//	const auto backup_flags = g::local->flags ( );
//
//	if ( g::send_packet ) {
//		//if ( g::local->layers ( ) )
//		//	g::local->layers ( ) [ 12 ].m_weight = 0.0f;
//
//		//if ( g::local->vel ( ).length_2d ( ) < 5.0f && g::local->flags ( ) & 1 )
//		//	g::local->layers ( ) [ 6 ].m_weight = 0.0f;
//
//		//if ( !( g::local->flags ( ) & 1 ) )
//		//	g::local->layers ( ) [ 4 ].m_cycle = std::fabsf ( csgo::i::globals->m_curtime - local_data::hit_ground_time );
//		//
//		//std::memcpy ( &local_data::overlays, g::local->layers ( ), N ( sizeof animlayer_t ) * g::local->num_overlays ( ) );
//		//std::memcpy ( &data::overlays [ g::local->idx ( ) ], g::local->layers ( ), N ( sizeof animlayer_t ) * g::local->num_overlays ( ) );
//	}
//
//	if ( g::local->flags ( ) & 1 )
//		local_data::was_on_ground = true;
//
//	// if ( csgo::i::globals->m_tickcount > local_data::old_tick ) {
//	if ( csgo::i::globals->m_tickcount > local_data::old_tick ) {
//		local_data::old_tick = csgo::i::globals->m_tickcount;
//
//		const auto run_fraction = g::local->vel ( ).length_2d ( ) / ( ( g::local->weapon ( ) && g::local->weapon ( )->data ( ) ) ? g::local->weapon ( )->data ( )->m_max_speed : 260.0f );
//
//		g::local->layers ( ) [ 12 ].m_weight = 0.0f;
//		//g::local->layers ( ) [ 6 ].m_weight = recalc_weight;
//		g::local->layers ( ) [ 4 ].m_cycle = recalc_cycle;
//
//		std::memcpy ( &local_data::overlays, g::local->layers ( ), N ( sizeof animlayer_t ) * g::local->num_overlays ( ) );
//		std::memcpy ( &data::overlays [ g::local->idx ( ) ], g::local->layers ( ), N ( sizeof animlayer_t ) * g::local->num_overlays ( ) );
//
//		update_animations( g::local );
//
//		rebuilt::poses::calculate ( g::local );
//
//		if ( g::send_packet && updated_tick != csgo::i::globals->m_tickcount ) {
//			recalc_weight = ( g::local->flags ( ) & 1 ) ? run_fraction : 0.0f;
//			recalc_cycle = std::fabsf ( last_update_time - local_data::hit_ground_time );
//
//			updated_tick = csgo::i::globals->m_tickcount;
//
//			if ( local_data::was_on_ground ) {
//				local_data::was_on_ground = false;
//				local_data::hit_ground_time = csgo::i::globals->m_curtime;
//			}
//
//			last_vel2d = g::local->vel ( ).length_2d ( );
//			last_update_time = csgo::i::globals->m_curtime;
//
//			local_data::abs = state->m_abs_yaw;
//			local_data::poses = g::local->poses ( );
//			local_data::old_update = features::prediction::predicted_curtime;
//
//			/* store old origin for lag-comp break check */
//			data::old_origin [ g::local->idx ( ) ] = data::origin [ g::local->idx ( ) ];
//			data::origin [ g::local->idx ( ) ] = g::local->origin ( );
//
//			g::hold_aim = false;
//		}
//	}
//
//	g::local->set_abs_angles( vec3_t( 0.0f, local_data::abs, 0.0f ) );
//
//	state->m_feet_yaw_rate = 0.0f;
//	state->m_unk_feet_speed_ratio = 0.0f;
//
//	std::memcpy( g::local->layers( ), &local_data::overlays, N( sizeof animlayer_t ) * g::local->num_overlays( ) );
//	g::local->poses( ) = local_data::poses;
//	//g::local->flags ( ) = backup_flags;
//
//	KEYBIND ( fd_key, "Sesame->B->Other->Other->Fakeduck Key" );
//
//	if ( !csgo::i::input->m_camera_in_thirdperson ) {
//		state->m_duck_amount = fd_key ? 0.0f : g::local->crouch_amount ( );
//		state->m_unk_feet_speed_ratio = g::local->crouch_speed ( );
//	}
//
//	setup_bones ( g::local, nullptr, N( 0x7ff00 ), vec3_t ( 0.0f, local_data::abs, 0.0f ), g::local->abs_origin( ), csgo::i::globals->m_curtime );
//
//	if ( g::local->bone_accessor ( ).get_bone_arr_for_write ( ) )
//		std::memcpy ( &data::fixed_bones [ g::local->idx ( ) ], g::local->bone_accessor ( ).get_bone_arr_for_write ( ), sizeof ( matrix3x4_t ) * g::local->bone_count ( ) );
//	//features::lagcomp::cache( g::local );
//}

<<<<<<< HEAD
=======
void animations::simulate ( player_t* pl, bool updated ) {
	/*
	@CBRS
		this is fairly spaghetti so i won't look through the entire thing, but here's a animation rundown
		make animation states update for every position and angle a player is at on the server since you last saw them. make sure the client-only shit is not ran, and ur done
		yay
		you can do dumb airtime fixes (i.e. this layer4.weight < last layer4.weight = they hit ground some time), or you can simulate each command the player would have made, 
		which will in turn correctly set airtime when ~raytracing~ occurs
		easy enough, yeah?
	*/

	if ( updated && pl->animstate ( ) && pl->layers ( ) && pl->bone_accessor ( ).get_bone_arr_for_write ( ) ) {
		data::simulated::animtimes [ pl->idx ( ) ] = pl->simtime ( );
		data::simulated::flags [ pl->idx ( ) ] = pl->flags ( );
		data::simulated::velocities [ pl->idx ( ) ] = pl->vel ( );
		data::simulated::origins [ pl->idx ( ) ] = pl->origin ( );

		//pl->create_animstate ( &data::simulated::animstates [ pl->idx ( ) ] );

		std::memcpy ( &data::simulated::animstates [ pl->idx ( ) ], pl->animstate ( ), sizeof ( animstate_t ) );
		std::memcpy ( data::simulated::poses [ pl->idx ( ) ], &pl->poses ( ), sizeof ( float ) * 24 );
		std::memcpy ( data::simulated::overlays [ pl->idx ( ) ], pl->layers ( ), sizeof ( animlayer_t ) * 15 );
		std::memcpy ( data::simulated::bones [ pl->idx ( ) ], pl->bone_accessor ( ).get_bone_arr_for_write ( ), sizeof ( matrix3x4_t ) * pl->bone_count( ) );

		data::simulated::animtimes [ pl->idx ( ) ] = pl->simtime ( );

		return;
	}

	const auto recs = features::lagcomp::get_all ( pl );
	const auto delta_ticks = csgo::i::globals->m_tickcount - data::old_tick [ pl->idx ( ) ];

	animstate_t backup_animstate;
	auto backup_flags = pl->flags ( );
	auto backup_abs_vel = *reinterpret_cast< vec3_t* >( uintptr_t ( pl ) + N ( 0x94 ) );
	auto backup_vel = pl->vel ( );
	auto backup_abs_origin = pl->abs_origin ( );
	auto backup_origin = pl->origin ( );
	float backup_poses [ 24 ];
	animlayer_t backup_overlays [ 15 ];
	auto backup_eflags = *reinterpret_cast< uint32_t* >( uintptr_t ( pl ) + N ( 0xe8 ) );

	if ( pl->layers ( ) )
		std::memcpy ( backup_overlays, pl->layers ( ), sizeof ( animlayer_t ) * 15 );

	std::memcpy ( backup_poses, &pl->poses ( ), sizeof ( float ) * 24 );
	
	if ( pl->animstate ( ) )
		std::memcpy ( &backup_animstate, pl->animstate ( ), sizeof ( animstate_t ) );

	if ( recs.first.size ( ) >= 2 ) {
		auto first_rec = recs.first [ 0 ];
		auto next_rec = recs.first [ 1 ];
		auto accel = ( first_rec.m_vel - next_rec.m_vel ) / ( first_rec.m_simtime - next_rec.m_simtime );

		data::simulated::velocities[ pl->idx ( ) ].x += accel.x * csgo::ticks2time ( 1 );
		data::simulated::velocities[ pl->idx ( ) ].y += accel.y * csgo::ticks2time ( 1 );
	}

	data::simulated::velocities [ pl->idx ( ) ].z += -800.0f * csgo::ticks2time ( 1 );

	auto from = data::simulated::origins [ pl->idx ( ) ];
	auto to = data::simulated::origins [ pl->idx ( ) ] + data::simulated::velocities [ pl->idx ( ) ] * csgo::ticks2time ( 1 );

	trace_t tr;
	csgo::util_tracehull ( from + vec3_t ( 0.0f, 0.0f, 2.0f ), to, pl->mins( ), pl->maxs ( ), 0x201400B, pl, &tr );

	data::simulated::flags [ pl->idx ( ) ] &= ~1;

	if ( !tr.is_visible( ) ) {
		to = tr.m_endpos;
		data::simulated::flags [ pl->idx ( ) ] |= 1;
	}

	data::simulated::origins [ pl->idx ( ) ] = to;

	pl->flags ( ) = data::simulated::flags [ pl->idx ( ) ];
	pl->abs_origin ( ) = data::simulated::origins [ pl->idx ( ) ];
	pl->origin ( ) = data::simulated::origins [ pl->idx ( ) ];
	pl->vel ( ) = data::simulated::velocities [ pl->idx ( ) ];
	*reinterpret_cast< uint32_t* >( uintptr_t ( pl ) + N ( 0xe8 ) ) &= ~0x1000; /* EFL_DIRTY_ABSVELOCITY */
	*reinterpret_cast< vec3_t* >( uintptr_t ( pl ) + N ( 0x94 ) ) = data::simulated::velocities [ pl->idx ( ) ];

	if ( pl->layers ( ) )
		std::memcpy ( pl->layers ( ), data::simulated::overlays [ pl->idx ( ) ], sizeof ( animlayer_t ) * 15 );

	std::memcpy ( &pl->poses ( ), data::simulated::poses [ pl->idx ( ) ], sizeof ( float ) * 24 );

	if ( pl->animstate ( ) )
		std::memcpy ( pl->animstate ( ), &data::simulated::animstates [ pl->idx ( ) ], sizeof ( animstate_t ) );

	auto update_angle = pl->angles ( );

	if ( pl->weapon ( ) && csgo::time2ticks ( pl->weapon ( )->last_shot_time ( ) ) == csgo::time2ticks ( pl->simtime ( ) ) + delta_ticks )
		update_angle = csgo::calc_angle ( pl->eyes ( ), g::local->eyes ( ) );

	csgo::clamp ( update_angle );

	data::simulated::animstates [ pl->idx ( ) ].m_force_update = true;
	data::simulated::animstates [ pl->idx ( ) ].m_last_clientside_anim_framecount = 0;

	const float backup_curtime = csgo::i::globals->m_curtime;
	const float backup_frametime = csgo::i::globals->m_frametime;

	csgo::i::globals->m_curtime = pl->simtime ( ) + csgo::ticks2time ( delta_ticks );
	csgo::i::globals->m_frametime = csgo::i::globals->m_ipt;

	data::simulated::animstates [ pl->idx ( ) ].update ( update_angle );

	csgo::i::globals->m_curtime = backup_curtime;
	csgo::i::globals->m_frametime = backup_frametime;

	resolver::resolve ( pl, data::simulated::animstates [ pl->idx ( ) ].m_abs_yaw );

	if ( pl->animstate ( ) )
		std::memcpy ( pl->animstate ( ), &data::simulated::animstates [ pl->idx ( ) ], sizeof ( animstate_t ) );

	build_matrix ( pl, data::simulated::bones [ pl->idx ( ) ], N ( 128 ), N ( 0x7ff00 ), csgo::i::globals->m_curtime );

	if ( pl->layers ( ) )
		std::memcpy ( data::simulated::overlays [ pl->idx ( ) ], pl->layers ( ), sizeof ( animlayer_t ) * 15 );

	std::memcpy ( data::simulated::poses [ pl->idx ( ) ], &pl->poses ( ), sizeof ( float ) * 24 );

	if ( pl->animstate ( ) )
		std::memcpy ( &data::simulated::animstates [ pl->idx ( ) ], pl->animstate ( ), sizeof ( animstate_t ) );

	*reinterpret_cast< uint32_t* >( uintptr_t ( pl ) + N ( 0xe8 ) ) = backup_eflags;
	pl->flags ( ) = backup_flags;
	pl->vel ( ) = backup_vel;
	pl->origin ( ) = backup_origin;
	pl->abs_origin ( ) = backup_abs_origin;
	
	if ( pl->layers ( ) )
		std::memcpy ( pl->layers ( ), backup_overlays, sizeof ( animlayer_t ) * 15 );

	std::memcpy ( &pl->poses ( ), backup_poses, sizeof ( float ) * 24 );

	if ( pl->animstate ( ) )
		std::memcpy ( pl->animstate ( ), &backup_animstate, sizeof ( animstate_t ) );

	data::simulated::animtimes [ pl->idx ( ) ] = pl->simtime ( ) + csgo::ticks2time( delta_ticks );
}

>>>>>>> b3ebc1ec849f3f15140c954ebc5447282cf2bf4a
int animations::fix_pl ( player_t* pl ) {
	//FIND ( int, safe_point_mode, "rage", "aimbot", "safe point", oxui::object_dropdown );
	KEYBIND ( safe_point_key, "Sesame->A->Default->Accuracy->Safe Point Key" );
	OPTION ( bool, safe_point, "Sesame->A->Default->Accuracy->Safe Point", oxui::object_checkbox );

	if ( !pl->valid ( ) || !pl->animstate ( ) || !pl->layers ( ) ) {
		if ( pl ) {
			data::origin [ pl->idx ( ) ] = vec3_t ( FLT_MAX, FLT_MAX, FLT_MAX );
		data::old_origin [ pl->idx ( ) ] = vec3_t ( FLT_MAX, FLT_MAX, FLT_MAX );
	}

		return 0;
	}

		const auto state = pl->animstate ( );

	auto backup_vel = pl->vel ( );
	auto backup_abs_origin = pl->abs_origin ( );
	auto backup_origin = pl->origin ( );
	auto backup_abs_vel = *reinterpret_cast< vec3_t* >( uintptr_t ( pl ) + N ( 0x94 ) );
	auto backup_eflags = *reinterpret_cast< uint32_t* >( uintptr_t ( pl ) + N ( 0xe8 ) );

	/* fix velocity */
	*reinterpret_cast< uint32_t* >( uintptr_t ( pl ) + N ( 0xe8 ) ) &= ~0x1000; /* EFL_DIRTY_ABSVELOCITY */


	pl->set_abs_origin ( pl->origin ( ) );
	* reinterpret_cast< vec3_t* >( uintptr_t ( pl ) + N ( 0x94 ) ) = pl->vel ( );

	if ( pl->simtime ( ) != pl->old_simtime ( ) ) {
		data::choke [ pl->idx ( ) ] = csgo::time2ticks ( pl->simtime ( ) - data::old_simtime [ pl->idx ( ) ] ) - 1;
		data::old_simtime [ pl->idx ( ) ] = pl->simtime ( );

		if ( pl->layers ( ) )
			pl->layers ( ) [ 12 ].m_weight = 0.0f;

		std::memcpy ( &data::overlays [ pl->idx ( ) ], pl->layers ( ), N ( sizeof animlayer_t ) * pl->num_overlays ( ) );

		data::old_tick [ pl->idx ( ) ] = csgo::i::globals->m_tickcount;

		state->m_feet_yaw_rate = 0.0f;
		state->m_feet_yaw = state->m_abs_yaw;

		update_animations ( pl );

		data::old_abs [ pl->idx ( ) ] = state->m_abs_yaw;
	data::old_eye_yaw [ pl->idx ( ) ] = state->m_eye_yaw;

	data::resolved1 [ pl->idx ( ) ] = data::resolved2 [ pl->idx ( ) ] = data::resolved3 [ pl->idx ( ) ] = state->m_abs_yaw;
	//state->m_abs_yaw = data::old_abs [ pl->idx ( ) ];
	resolver::resolve ( pl, data::resolved1 [ pl->idx ( ) ], data::resolved2 [ pl->idx ( ) ], data::resolved3 [ pl->idx ( ) ] );

	state->m_eye_yaw = pl->angles ( ).y;

	/* build and store safe point matrix */
	//if ( safe_point && safe_point_key ) {
	//	const auto opposite_rotation = state->m_abs_yaw;
	//
	//	setup_bones ( pl, ( matrix3x4_t* ) &data::bones [ pl->idx ( ) ], N ( 0x7ff00 ), vec3_t( 0.0f, state->m_abs_yaw, 0.0f ), pl->origin(), csgo::i::globals->m_curtime );
	//
	//	for ( auto& bone : data::bones [ pl->idx ( ) ] )
	//		bone.set_origin ( bone.origin ( ) - pl->origin( ) );
	//}

	state->m_abs_yaw = data::resolved1 [ pl->idx ( ) ];
	rebuilt::poses::calculate ( pl );
	setup_bones ( pl, reinterpret_cast< matrix3x4_t* >( &data::fixed_bones [ pl->idx ( ) ] ), N ( 0x7ff00 ), vec3_t ( 0.0f, data::resolved1 [ pl->idx ( ) ], 0.0f ), pl->origin ( ), csgo::i::globals->m_curtime );
	
	state->m_abs_yaw = data::resolved2 [ pl->idx ( ) ];
	rebuilt::poses::calculate ( pl );
	setup_bones ( pl, reinterpret_cast< matrix3x4_t* >( &data::fixed_bones1 [ pl->idx ( ) ] ), N ( 0x7ff00 ), vec3_t ( 0.0f, data::resolved2 [ pl->idx ( ) ], 0.0f ), pl->origin ( ), csgo::i::globals->m_curtime );
	
	state->m_abs_yaw = data::resolved3 [ pl->idx ( ) ];
	rebuilt::poses::calculate ( pl );
	setup_bones ( pl, reinterpret_cast< matrix3x4_t* >( &data::fixed_bones2 [ pl->idx ( ) ] ), N ( 0x7ff00 ), vec3_t ( 0.0f, data::resolved3 [ pl->idx ( ) ], 0.0f ), pl->origin ( ), csgo::i::globals->m_curtime );
	}

	if ( pl->bone_accessor ( ).get_bone_arr_for_write ( ) ) {
		if ( features::ragebot::get_misses ( pl->idx ( ) ).bad_resolve % 3 == 0 ) {
			state->m_abs_yaw = data::resolved1 [ pl->idx ( ) ];
			pl->set_abs_angles (vec3_t( 0.0f, state->m_abs_yaw, 0.0f) );
			memcpy ( pl->bone_accessor ( ).get_bone_arr_for_write ( ), &data::fixed_bones [ pl->idx ( ) ], sizeof ( matrix3x4_t ) * pl->bone_count ( ) );
		}
		else if ( features::ragebot::get_misses ( pl->idx ( ) ).bad_resolve % 3 == 1 ) {
			state->m_abs_yaw = data::resolved2[ pl->idx ( ) ];
			pl->set_abs_angles ( vec3_t ( 0.0f, state->m_abs_yaw, 0.0f ) );
			memcpy ( pl->bone_accessor ( ).get_bone_arr_for_write ( ), &data::fixed_bones1 [ pl->idx ( ) ], sizeof ( matrix3x4_t ) * pl->bone_count ( ) );
		}
		else {
			state->m_abs_yaw = data::resolved3 [ pl->idx ( ) ];
			pl->set_abs_angles ( vec3_t ( 0.0f, state->m_abs_yaw, 0.0f ) );
			memcpy ( pl->bone_accessor ( ).get_bone_arr_for_write ( ), &data::fixed_bones2 [ pl->idx ( ) ], sizeof ( matrix3x4_t ) * pl->bone_count ( ) );
		}
			
	}

				pl->origin( ) = backup_origin;
				pl->abs_origin( ) = backup_abs_origin;
				pl->vel ( ) = backup_vel;
				
				features::lagcomp::cache( pl );

				*reinterpret_cast< uint32_t* >( uintptr_t ( pl ) + N ( 0xe8 ) ) = backup_eflags;

}

int animations::run( int stage ) {
	if ( !csgo::i::engine->is_connected ( ) || !csgo::i::engine->is_in_game ( ) )
		return 0;

		switch ( stage ) {
		case 5: { /* fix local anims */
			RUN_SAFE (	
				"animations::fake::simulate",
				animations::fake::simulate ( );
			);

			RUN_SAFE (
				"animations::restore_local",
				restore_local ( );
			);

			//auto util_playerbyindex = [ ] ( int idx ) {
			//	using player_by_index_fn = player_t * ( __fastcall* )( int );
			//	static auto fn = pattern::search( _( "server.dll" ), _( "85 C9 7E 2A A1" ) ).get< player_by_index_fn >( );
			//	return fn( idx );
			//};
			//
			//static auto draw_hitboxes = pattern::search( _( "server.dll" ), _( "55 8B EC 81 EC ? ? ? ? 53 56 8B 35 ? ? ? ? 8B D9 57 8B CE" ) ).get< std::uintptr_t >( );
			//
			//const auto dur = -1.f;
			//
			//for ( auto i = 1; i <= csgo::i::globals->m_max_clients; i++ ) {
			//	auto e_local = csgo::i::ent_list->get< player_t* >( i );
			//
			//	if ( !e_local )
			//		continue;
			//
			//	auto e = util_playerbyindex( e_local->idx( ) );
			//
			//	if ( !e )
			//		continue;
			//
			//	__asm {
			//		pushad
			//		movss xmm1, dur
			//		push 0
			//		mov ecx, e
			//		call draw_hitboxes
			//		popad
			//	}
			//}
			} break;
		case 4: {
			security_handler::update ( );

			RUN_SAFE (
				"animations::store_local",
				store_local ( );
			);

			/* reset anti-bruteforce when new round starts */
			if ( g::round == round_t::starting ) {
				for ( auto i = 0; i <= 64; i++ ) {
					resolver::rdata::player_hurt [ i ] = false;
					resolver::rdata::queued_hit [ i ] = false;
					resolver::rdata::tried_side [ i ] = 0;
					resolver::rdata::forced_side [ i ] = 0;
					features::ragebot::get_misses ( i ).bad_resolve = 0;
				}
			}

			for ( auto i = 1; i <= csgo::i::globals->m_max_clients; i++ ) {
				const auto pl = csgo::i::ent_list->get< player_t* > ( i );

				resolver::process_event_buffer ( i );

<<<<<<< HEAD
				if ( !pl || !pl->is_player ( ) || pl == g::local /*|| pl->team ( ) == g::local->team( )*/ ) {				
=======
				/*
				@CBRS
					ew
					reevaluateanimlod is fixeable with a hook... and you already have the hook
					hint: is_hltv, 84 C0 0F 85 ? ? ? ? A1 ? ? ? ? 8B B7
				*/

				if ( !pl || !pl->is_player ( ) || pl == g::local || pl->team ( ) == g::local->team( ) ) {
					if ( pl ) {
						auto var_map = reinterpret_cast< uintptr_t >( pl ) + 36;
						auto var_map_list_count = *reinterpret_cast< int* >( var_map + 20 );

						for ( auto index = 0; index < var_map_list_count; index++ )
							*reinterpret_cast< uint16_t* >( *reinterpret_cast< uintptr_t* >( var_map ) + index * 12 ) = 1;
					}

>>>>>>> b3ebc1ec849f3f15140c954ebc5447282cf2bf4a
					if ( pl && pl != g::local )
						pl->animate ( ) = true;

					continue;
				}
<<<<<<< HEAD
=======
				auto var_map = reinterpret_cast< uintptr_t >( pl ) + 36;
				auto var_map_list_count = *reinterpret_cast< int* >( var_map + 20 );

				for ( auto index = 0; index < var_map_list_count; index++ )
					*reinterpret_cast< uint16_t* >( *reinterpret_cast< uintptr_t* >( var_map ) + index * 12 ) = 0;
>>>>>>> b3ebc1ec849f3f15140c954ebc5447282cf2bf4a

				RUN_SAFE (
					"animations::fix_pl",
					fix_pl ( pl );
				);
			}
		} break;
		case 2: /* store incoming data */ {

		} break;
		}

		return 0;
}