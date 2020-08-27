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
#include "../menu/options.hpp"

#include "../menu/d3d9_render.hpp"

bool setup_bones( player_t* target, matrix3x4_t* mat, int mask, vec3_t rotation, vec3_t origin, float time ) {
	/* vfunc indices */
	static auto standard_blending_rules_vfunc_idx = pattern::search( _( "client.dll" ), _( "FF 90 ? ? ? ? 8B 47 FC" ) ).add( 2 ).deref( ).get< uint32_t >( ) / 4;
	static auto build_transformations_vfunc_idx = pattern::search( _( "client.dll" ), _( "FF 90 ? ? ? ? A1 ? ? ? ? B9 ? ? ? ? 8B 40 34 FF D0 85 C0 74 41" ) ).add( 2 ).deref( ).get< uint32_t >( ) / 4;
	static auto update_ik_locks_vfunc_idx = pattern::search( _( "client.dll" ), _( "FF 90 ? ? ? ? 8B 8F ? ? ? ? 8D 84 24 ? ? ? ? 50" ) ).add( 2 ).deref( ).get< uint32_t >( ) / 4;
	static auto calculate_ik_locks_vfunc_idx = pattern::search( _( "client.dll" ), _( "FF 90 ? ? ? ? 8B 8F ? ? ? ? 8D 84 24 ? ? ? ? 50 FF B7 ? ? ? ? 8D 84 24 ? ? ? ? 50 8D 84 24 ? ? ? ? 50 E8 ? ? ? ? 8B 47 FC 8D 4F FC 8B 80" ) ).add( 2 ).deref( ).get< uint32_t >( ) / 4;

	/* func sigs */
	static auto init_iks = pattern::search( _( "client.dll" ), _( "55 8B EC 83 EC 08 8B 45 08 56 57 8B F9 8D" ) ).get< void( __thiscall* )( void*, studiohdr_t*, vec3_t&, vec3_t&, float, int, int ) >( );
	static auto update_targets = pattern::search( _( "client.dll" ), _( "55 8B EC 83 E4 F0 81 EC 18 01 00 00 33" ) ).get< void( __thiscall* )( void*, vec3_t*, void*, void*, uint8_t* ) >( );
	static auto solve_dependencies = pattern::search( _( "client.dll" ), _( "55 8B EC 83 E4 F0 81 EC 98 05 00 00 8B 81" ) ).get< void( __thiscall* )( void*, vec3_t*, void*, void*, uint8_t* ) >( );

	/* offset sigs */
	static auto iks_off = pattern::search( _( "client.dll" ), _( "8D 47 FC 8B 8F" ) ).add( 5 ).deref( ).add( 4 ).get< uint32_t >( );
	static auto effects_off = pattern::search( _( "client.dll" ), _( "75 0D 8B 87" ) ).add( 4 ).deref( ).add( 4 ).get< uint32_t >( );

	auto cstudio = *reinterpret_cast< studiohdr_t** >( uintptr_t( target ) + 0x294C );

	if ( !cstudio )
		return false;

	//  we need an aligned matrix in the bone accessor, so do this :) bad performance cause memcpy but that's ok
	matrix3x4a_t used [ 128 ];

	//	output shit
	uint8_t bone_computed [ 0x100 ] = { 0 };

	//	needs to be aligned
	matrix3x4a_t base_matrix;

	csgo::angle_matrix( rotation, origin, base_matrix );

	//	store shit
	const auto old_bones = target->bones( );

	auto iks = *reinterpret_cast< void** >( uintptr_t( target ) + iks_off );
	*reinterpret_cast< int* >( uintptr_t( target ) + effects_off ) |= 8;

	//	clear iks & re-create them
	if ( iks ) {
		if ( *( uint32_t* )( uintptr_t( iks ) + 0xFF0 ) > 0 ) {
			int v1 = 0;
			auto v62 = ( uint32_t* )( uintptr_t( iks ) + 0xD0 );

			do {
				*v62 = -9999;
				v62 += 0x55;
				++v1;
			} while ( v1 < *( uint32_t* )( uintptr_t( iks ) + 0xFF0 ) );
		}

		init_iks( iks, cstudio, rotation, origin, time, csgo::i::globals->m_framecount, 0x1000 );
	}

	vec3_t pos [ 128 ];
	memset( pos, 0, sizeof( vec3_t [ 128 ] ) );

	void* q = malloc( /* sizeof quaternion_t */ 48 * 128 );
	memset( q, 0, /* sizeof quaternion_t */ 48 * 128 );

	//	set flags and bones
	target->bones( ) = used;

	//	build some shit
	// IDX = 205 ( 2020.7.10 )
	vfunc< void( __thiscall* )( player_t*, studiohdr_t*, vec3_t*, void*, float, uint32_t ) >( target, standard_blending_rules_vfunc_idx ) ( target, cstudio, pos, q, time, mask );

	//	set iks
	if ( iks ) {
		vfunc< void( __thiscall* )( player_t*, float ) >( target, update_ik_locks_vfunc_idx ) ( target, time );
		update_targets( iks, pos, q, target->bones( ), bone_computed );
		vfunc< void( __thiscall* )( player_t*, float ) >( target, calculate_ik_locks_vfunc_idx ) ( target, time );
		solve_dependencies( iks, pos, q, target->bones( ), bone_computed );
	}

	//	build the matrix
	// IDX = 189 ( 2020.7.10 )
	vfunc< void( __thiscall* )( player_t*, studiohdr_t*, vec3_t*, void*, matrix3x4a_t const&, uint32_t, void* ) >( target, build_transformations_vfunc_idx ) ( target, cstudio, pos, q, base_matrix, mask, bone_computed );

	free( q );

	//	restore flags and bones
	*reinterpret_cast< int* >( uintptr_t( target ) + effects_off ) &= ~8;
	target->bones( ) = old_bones;

	//  and pop out our new matrix
	memcpy( mat, used, sizeof( matrix3x4_t ) * 128 );

	return true;
}

namespace local_data {
	float ground_time = 0.0f;
	bool was_on_ground = true;
	int old_tick = 0;
	float abs = 0.0f;
	float hit_ground_time = 0.0f;
	std::array< float, 24 > poses { };
	std::array< animlayer_t, 15 > overlays { };
	std::array< animlayer_t, 15 > synced_overlays { };
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
			std::array< vec3_t, 65 > origins { vec3_t( 0.0f, 0.0f, 0.0f ) };
			std::array< vec3_t, 65 > velocities { vec3_t( 0.0f, 0.0f, 0.0f ) };
			std::array< animstate_t, 65 > animstates { };
			std::array< matrix3x4_t [ 128 ], 65 > bones { };
			std::array< float [ 24 ], 65 > poses { };
			std::array< animlayer_t [ 15 ], 65 > overlays { };
		}

		std::array< animstate_t, 65 > fake_states { };
		std::array< vec3_t, 65 > origin { vec3_t( FLT_MAX, FLT_MAX, FLT_MAX ) };
		std::array< vec3_t, 65 > old_origin { vec3_t( FLT_MAX, FLT_MAX, FLT_MAX ) };
		std::array< std::array< matrix3x4_t, 128 >, 65 > bones;
		std::array< matrix3x4_t, 128 > aim_matrix;
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

namespace animations {
	namespace rebuilt {
		namespace poses {
			__forceinline float jump_fall( player_t* pl, float air_time ) {
				const float airtime = ( air_time - 0.72f ) * 1.25f;
				const float clamped = airtime >= float( N( 0 ) ) ? std::min< float >( airtime, float( N( 1 ) ) ) : float( N( 0 ) );
				float jump_fall = ( float( N( 3 ) ) - ( clamped + clamped ) ) * ( clamped * clamped );

				if ( jump_fall >= float( N( 0 ) ) )
					jump_fall = std::min< float >( jump_fall, float( N( 1 ) ) );

				return jump_fall;
			}

			__forceinline float body_pitch( player_t* pl, float pitch ) {
				auto eye_pitch_normalized = csgo::normalize( pitch );
				auto new_body_pitch_pose = 0.0f;

				if ( eye_pitch_normalized <= 0.0f )
					new_body_pitch_pose = eye_pitch_normalized / pl->animstate( )->m_min_pitch;
				else
					new_body_pitch_pose = eye_pitch_normalized / pl->animstate( )->m_max_pitch;

				return new_body_pitch_pose;
			}

			__forceinline float body_yaw( player_t* pl, float yaw ) {
				auto eye_goalfeet_delta = csgo::normalize( csgo::normalize( yaw ) - csgo::normalize( pl->animstate( )->m_abs_yaw ) );
				auto new_body_yaw_pose = 0.0f;

				if ( eye_goalfeet_delta < 0.0f || pl->animstate( )->m_max_yaw == 0.0f ) {
					if ( pl->animstate( )->m_min_yaw != 0.0f )
						new_body_yaw_pose = eye_goalfeet_delta / pl->animstate( )->m_min_yaw;
				}
				else {
					new_body_yaw_pose = eye_goalfeet_delta / pl->animstate( )->m_max_yaw;
				}

				return new_body_yaw_pose;
			}

			__forceinline float lean_yaw( player_t* pl, float yaw ) {
				return csgo::normalize( pl->animstate( )->m_abs_yaw - yaw );
			}

			std::array< float, 65 > ground_fractions { 0.0f };
			std::array< bool, 65 > old_not_walking { false };

			/* local stuff */

			auto last_vel2d = 0.0f;
			auto last_update_time = 0.0f;
			auto updated_tick = 0;
			auto recalc_weight = 0.0f;
			auto recalc_cycle = 0.0f;
			auto last_local_update = 0.0f;

			__forceinline void calculate_ground_fraction( player_t* pl ) {
				auto& abs_vel = *reinterpret_cast< vec3_t* >( uintptr_t( pl ) + N( 0x94 ) );

				constexpr auto CS_PLAYER_SPEED_DUCK_MODIFIER = 0.340f;
				constexpr auto CS_PLAYER_SPEED_WALK_MODIFIER = 0.520f;
				constexpr auto CS_PLAYER_SPEED_RUN = 260.0f;

				/* calculate new ground fraction */
				const auto old_ground_fraction = ground_fractions [ pl->idx( ) ];

				if ( old_ground_fraction > 0.0f && old_ground_fraction < 1.0f ) {
					const auto twotickstime = pl->animstate( )->m_last_clientside_anim_update_time_delta * 2.0f;

					if ( old_not_walking [ pl->idx( ) ] )
						ground_fractions [ pl->idx( ) ] = old_ground_fraction - twotickstime;
					else
						ground_fractions [ pl->idx( ) ] = twotickstime + old_ground_fraction;

					ground_fractions [ pl->idx( ) ] = std::clamp< float >( ground_fractions [ pl->idx( ) ], 0.0f, 1.0f );
				}

				if ( abs_vel.length_2d( ) > ( CS_PLAYER_SPEED_RUN * CS_PLAYER_SPEED_WALK_MODIFIER ) && old_not_walking [ pl->idx( ) ] ) {
					old_not_walking [ pl->idx( ) ] = false;
					ground_fractions [ pl->idx( ) ] = std::max< float >( ground_fractions [ pl->idx( ) ], 0.01f );
				}
				else if ( abs_vel.length_2d( ) < ( CS_PLAYER_SPEED_RUN * CS_PLAYER_SPEED_WALK_MODIFIER ) && !old_not_walking [ pl->idx( ) ] ) {
					old_not_walking [ pl->idx( ) ] = true;
					ground_fractions [ pl->idx( ) ] = std::min< float >( ground_fractions [ pl->idx( ) ], 0.99f );
				}
			}

			/* local stuff */

			__forceinline void calculate( player_t* pl ) {
				auto state = pl->animstate( );
				auto layers = pl->layers( );

				if ( !state || !layers )
					return;

				//state->m_lean_yaw_pose.set_value ( pl, ( csgo::normalize ( pl->angles ( ).y ) + 180.0f ) / 360.0f );
				state->m_move_blend_walk_pose.set_value( pl, ( 1.0f - ( pl == g::local ? ground_fractions [ pl->idx( ) ] : data::overlays [ pl->idx( ) ][ N( 6 ) ].m_weight ) ) * ( 1.0f - pl->crouch_amount( ) ) );
				state->m_move_blend_run_pose.set_value( pl, ( 1.0f - pl->crouch_amount( ) ) * ( pl == g::local ? ground_fractions [ pl->idx( ) ] : data::overlays [ pl->idx( ) ][ N( 6 ) ].m_weight ) );
				state->m_move_blend_crouch_pose.set_value( pl, pl->crouch_amount( ) );
				state->m_jump_fall_pose.set_value( pl, rebuilt::poses::jump_fall( pl, pl == g::local ? pl->animstate( )->m_time_in_air : data::overlays [ pl->idx( ) ][ N( 4 ) ].m_cycle ) );
				state->m_body_pitch_pose.set_value( pl, ( csgo::normalize( state->m_pitch ) + 90.0f ) / 180.0f );
				state->m_body_yaw_pose.set_value( pl, std::clamp( csgo::normalize( csgo::normalize( state->m_eye_yaw ) - csgo::normalize( state->m_abs_yaw ) ), -60.0f, 60.0f ) / 120.0f + 0.5f );
			}
		}
	}
}

void animations::estimate_vel( player_t* pl, vec3_t& out ) {
	const auto ground_fraction = data::overlays [ pl->idx( ) ][ N( 6 ) ].m_weight;
	const auto max_speed = 260.0f;
	const auto normal_vec = out.normalized( );

	out = normal_vec * ( ground_fraction * max_speed );
}

std::array< matrix3x4_t, 128 >& animations::fake::matrix( ) {
	return local_data::fake::simulated_mat;
}

namespace lby {
	extern bool in_update;
}

__forceinline void update_animations( player_t* pl ) {
	pl->animstate( )->m_last_clientside_anim_framecount = 0;
	pl->animstate( )->m_feet_yaw_rate = 0.0f;

	const auto backup_frametime = csgo::i::globals->m_frametime;
	const auto backup_interp = csgo::i::globals->m_interp;
	const auto backup_curtime = csgo::i::globals->m_curtime;

	static bool last_on_ground = false;
	static float last_ground_time = 0.0f;
	static float last_moving_time = 0.0f;
	static bool last_moving = false;
	static float last_stop_moving_time = 0.0f;

	if ( pl == g::local ) {
		auto& abs_vel = *reinterpret_cast< vec3_t* >( uintptr_t( pl ) + N( 0x94 ) );

		if ( abs_vel.length_2d( ) > 0.1f && !last_moving )
			last_stop_moving_time = csgo::i::globals->m_curtime;

		if ( last_moving = abs_vel.length_2d( ) > 0.1f )
			last_moving_time = csgo::i::globals->m_curtime;

		pl->animstate( )->m_speed2d = abs_vel.length_2d( );
		pl->animstate( )->m_up_vel = abs_vel.z;
		pl->animstate( )->m_vel2d = { abs_vel.x, abs_vel.y };
		pl->animstate( )->m_on_ground = pl->flags( ) & 1;
		pl->animstate( )->m_hit_ground = pl->flags( ) & 1 && !last_on_ground;
		pl->animstate( )->m_time_in_air = fabsf( csgo::i::globals->m_curtime - last_ground_time );
		pl->animstate( )->m_moving = abs_vel.length_2d( ) > 0.1f;
		pl->animstate( )->m_time_since_stop = last_moving_time;
		pl->animstate( )->m_time_since_move = last_stop_moving_time;

		if ( !csgo::i::input->m_camera_in_thirdperson )
			pl->animstate( )->m_hit_ground = false;
	}

	pl->animstate( )->m_last_clientside_anim_update_time_delta = std::max< float >( 0.0f, pl == g::local ? csgo::ticks2time( 1 ) : ( pl->simtime( ) - pl->old_simtime( ) ) );

	if ( pl != g::local )
		csgo::i::globals->m_curtime = pl->old_simtime( ) + csgo::ticks2time( 1 );

	csgo::i::globals->m_frametime = csgo::ticks2time( 1 );
	csgo::i::globals->m_interp = 0.0f;

	pl->animate( ) = true;
	pl->update( );
	pl->animate( ) = false;

	if ( pl == g::local ) {
		auto& abs_vel = *reinterpret_cast< vec3_t* >( uintptr_t( pl ) + N( 0x94 ) );

		pl->animstate( )->m_speed2d = abs_vel.length_2d( );
		pl->animstate( )->m_up_vel = abs_vel.z;
		pl->animstate( )->m_vel2d = { abs_vel.x, abs_vel.y };
		pl->animstate( )->m_on_ground = pl->flags( ) & 1;
		pl->animstate( )->m_hit_ground = pl->flags( ) & 1 && !last_on_ground;
		pl->animstate( )->m_time_in_air = fabsf( csgo::i::globals->m_curtime - last_ground_time );
		pl->animstate( )->m_moving = abs_vel.length_2d( ) > 0.1f;
		pl->animstate( )->m_time_since_stop = last_moving_time;
		pl->animstate( )->m_time_since_move = last_stop_moving_time;

		if ( last_on_ground = pl->flags( ) & 1 )
			last_ground_time = csgo::i::globals->m_curtime;

		if ( !csgo::i::input->m_camera_in_thirdperson )
			pl->animstate( )->m_hit_ground = false;
	}

	//if ( pl != g::local ) {
	//	pl->animstate ( )->m_pitch = pl->angles ( ).x;
	//	pl->animstate ( )->m_eye_yaw = pl->angles ( ).y;
	//}

	pl->animstate( )->m_last_clientside_anim_framecount = 0;
	pl->animstate( )->m_feet_yaw_rate = 0.0f;

	csgo::i::globals->m_frametime = backup_frametime;
	csgo::i::globals->m_interp = backup_interp;
	csgo::i::globals->m_curtime = backup_curtime;
}

int animations::fake::simulate( ) {
	static auto& fd_enabled = options::vars [ _( "antiaim.fakeduck" ) ].val.b;
	static auto& fd_mode = options::vars [ _( "antiaim.fakeduck_mode" ) ].val.i;
	static auto& fd_key = options::vars [ _( "antiaim.fakeduck_key" ) ].val.i;
	static auto& fd_key_mode = options::vars [ _( "antiaim.fd_key_mode" ) ].val.i;

	static int curtick = 0;

	if ( !g::local || !g::local->alive( ) || !g::local->layers( ) || !g::local->renderable( ) ) {
		local_data::fake::should_reset = true;
		curtick = 0;
		return 0;
	}

	if ( local_data::fake::should_reset || local_data::fake::spawn_time != g::local->spawn_time( ) ) {
		local_data::fake::spawn_time = g::local->spawn_time( );
		g::local->create_animstate( &fake_state );
		local_data::fake::should_reset = false;
		curtick = 0;
	}

	if ( g::send_packet && csgo::i::globals->m_tickcount > curtick ) {
		curtick = csgo::i::globals->m_tickcount;
		*reinterpret_cast< int* >( uintptr_t( g::local ) + 0xA68 ) = 0;

		std::array< animlayer_t, 15 > backup_overlays;
		std::memcpy( &backup_overlays, g::local->layers( ), N( sizeof animlayer_t ) * g::local->num_overlays( ) );
		std::memcpy( g::local->layers( ), &local_data::overlays, N( sizeof animlayer_t ) * g::local->num_overlays( ) );

		*reinterpret_cast< uint32_t* >( uintptr_t( g::local ) + N( 0xe8 ) ) &= ~0x1000; /* EFL_DIRTY_ABSVELOCITY */
		auto& abs_vel = *reinterpret_cast< vec3_t* >( uintptr_t( g::local ) + N( 0x94 ) );

		if ( fabsf( abs_vel.x ) < 0.001f )
			abs_vel.x = 0.0f;

		if ( fabsf( abs_vel.y ) < 0.001f )
			abs_vel.y = 0.0f;

		if ( fabsf( abs_vel.z ) < 0.001f )
			abs_vel.z = 0.0f;

		g::local->layers( ) [ 12 ].m_weight = 0.0f;
		const float backup_frametime = csgo::i::globals->m_frametime;
		const auto backup_poses = g::local->poses( );
		auto backup_abs_angles = g::local->abs_angles( );

		fake_state.m_last_clientside_anim_update_time_delta = std::max< float >( 0.0f, csgo::i::globals->m_curtime - g::local->simtime( ) );

		fake_state.m_feet_yaw_rate = 0.0f;
		fake_state.m_unk_feet_speed_ratio = 0.0f;
		fake_state.m_last_clientside_anim_framecount = 0;
		csgo::i::globals->m_frametime = csgo::i::globals->m_ipt;
		fake_state.update( g::sent_cmd.m_angs );
		csgo::i::globals->m_frametime = backup_frametime;

		g::local->layers( ) [ 12 ].m_weight = 0.0f;

		g::local->setup_bones( reinterpret_cast< matrix3x4_t* > ( &local_data::fake::simulated_mat ), 128, 256, csgo::i::globals->m_curtime );

		const auto render_origin = g::local->render_origin( );

		auto i = N( 0 );

		for ( i = N( 0 ); i < N( 128 ); i++ )
			local_data::fake::simulated_mat [ i ].set_origin( local_data::fake::simulated_mat [ i ].origin( ) - render_origin );

		g::local->abs_angles( ) = backup_abs_angles;
		g::local->poses( ) = backup_poses;
		std::memcpy( g::local->layers( ), &backup_overlays, N( sizeof animlayer_t ) * g::local->num_overlays( ) );
	}
}

int animations::store_local( ) {
	if ( !g::local->valid( ) || !g::local->animstate( ) || !g::local->layers( ) )
		return -1;

	const auto animstate = g::local->animstate( );

	//if ( g::local->simtime ( ) != last_local_update ) {
	//	g::local->layers ( ) [ 12 ].m_weight = 0.0f;
	//
	//	std::memcpy ( &local_data::overlays, g::local->layers ( ), N ( sizeof animlayer_t ) * g::local->num_overlays ( ) );
	//	std::memcpy ( &data::overlays [ g::local->idx ( ) ], g::local->layers ( ), N ( sizeof animlayer_t ) * g::local->num_overlays ( ) );
	//}
	//
	//last_local_update = g::local->simtime ( );

	g::local->layers( ) [ 12 ].m_weight = 0.0f;

	std::memcpy( &local_data::overlays, g::local->layers( ), N( sizeof animlayer_t ) * g::local->num_overlays( ) );
	std::memcpy( &data::overlays [ g::local->idx( ) ], g::local->layers( ), N( sizeof animlayer_t ) * g::local->num_overlays( ) );

	return 0;
}

int animations::restore_local( bool render ) {
	static auto& fd_enabled = options::vars [ _( "antiaim.fakeduck" ) ].val.b;
	static auto& fd_mode = options::vars [ _( "antiaim.fakeduck_mode" ) ].val.i;
	static auto& fd_key = options::vars [ _( "antiaim.fakeduck_key" ) ].val.i;
	static auto& fd_key_mode = options::vars [ _( "antiaim.fd_key_mode" ) ].val.i;

	if ( render ) {
		/* update viewmodel manually */ {
			using update_all_viewmodel_addons_t = int( __fastcall* )( void* );
			static auto update_all_viewmodel_addons = pattern::search( _( "client.dll" ), _( "55 8B EC 83 E4 ? 83 EC ? 53 8B D9 56 57 8B 03 FF 90 ? ? ? ? 8B F8 89 7C 24 ? 85 FF 0F 84 ? ? ? ? 8B 17 8B CF" ) ).get< update_all_viewmodel_addons_t >( );

			if ( g::local && g::local->viewmodel_handle( ) != -1 && csgo::i::ent_list->get_by_handle< void* >( g::local->viewmodel_handle( ) ) )
				update_all_viewmodel_addons( csgo::i::ent_list->get_by_handle< void* >( g::local->viewmodel_handle( ) ) );
		}
	}

	if ( !g::local->valid( ) || !g::local->animstate( ) || !g::local->layers( ) || !g::ucmd ) {
		local_data::old_tick = 0;

		if ( g::local ) {
			data::origin [ g::local->idx( ) ] = vec3_t( FLT_MAX, FLT_MAX, FLT_MAX );
			data::old_origin [ g::local->idx( ) ] = vec3_t( FLT_MAX, FLT_MAX, FLT_MAX );
		}

		return 0;
	}

	*reinterpret_cast< int* >( uintptr_t( g::local ) + 0xA68 ) = 0;

	const auto state = g::local->animstate( );

	*reinterpret_cast< uint32_t* >( uintptr_t( g::local ) + N( 0xe8 ) ) &= ~0x1000; /* EFL_DIRTY_ABSVELOCITY */
	auto& abs_vel = *reinterpret_cast< vec3_t* >( uintptr_t( g::local ) + N( 0x94 ) );

	if ( fabsf( abs_vel.x ) < 0.001f )
		abs_vel.x = 0.0f;

	if ( fabsf( abs_vel.y ) < 0.001f )
		abs_vel.y = 0.0f;

	if ( fabsf( abs_vel.z ) < 0.001f )
		abs_vel.z = 0.0f;

	if ( !render ) {
		if ( g::ucmd->m_buttons & 1 ) {
			g::angles = g::ucmd->m_angs;
			g::hold_aim = true;
		}

		if ( !g::hold_aim ) {
			g::angles = g::ucmd->m_angs;
		}

		const auto backup_flags = g::local->flags( );

		if ( g::local->flags( ) & 1 )
			local_data::was_on_ground = true;

		// if ( csgo::i::globals->m_tickcount > local_data::old_tick ) {
		if ( csgo::i::globals->m_tickcount > local_data::old_tick ) {
			local_data::old_tick = csgo::i::globals->m_tickcount;

			std::memcpy( g::local->layers( ), &local_data::overlays, N( sizeof animlayer_t ) * g::local->num_overlays( ) );

			rebuilt::poses::calculate_ground_fraction( g::local );

			g::local->layers( ) [ 12 ].m_weight = 0.0f;

			//if ( !( g::local->flags ( ) & 1 ) )
			//	g::local->layers ( ) [ 4 ].m_cycle = recalc_cycle;
			//else
			//if ( g::local->flags ( ) & 1 )
			//	g::local->layers ( ) [ 6 ].m_weight = ground_fractions [ g::local->idx ( ) ];

			update_animations( g::local );

			rebuilt::poses::calculate( g::local );

			setup_bones( g::local, reinterpret_cast< matrix3x4_t* > ( &data::aim_matrix ), 256, vec3_t( 0.0f, local_data::abs, 0.0f ), g::local->origin( ), csgo::i::globals->m_curtime );

			if ( g::send_packet /*&& updated_tick != csgo::i::globals->m_tickcount*/ ) {
				//recalc_weight = ( g::local->flags ( ) & 1 ) ? run_fraction : 0.0f;
				rebuilt::poses::recalc_cycle = std::fabsf( rebuilt::poses::last_update_time - local_data::hit_ground_time );
				rebuilt::poses::updated_tick = csgo::i::globals->m_tickcount;

				if ( local_data::was_on_ground ) {
					local_data::was_on_ground = false;
					local_data::hit_ground_time = csgo::i::globals->m_curtime;
				}

				rebuilt::poses::last_vel2d = abs_vel.length_2d( );
				rebuilt::poses::last_update_time = csgo::i::globals->m_curtime;

				local_data::abs = state->m_abs_yaw;
				local_data::poses = g::local->poses( );
				local_data::old_update = csgo::i::globals->m_curtime;

				/* store old origin for lag-comp break check */
				data::old_origin [ g::local->idx( ) ] = data::origin [ g::local->idx( ) ];
				data::origin [ g::local->idx( ) ] = g::local->origin( );

				//std::memcpy ( &local_data::synced_overlays, g::local->layers ( ), N ( sizeof animlayer_t ) * g::local->num_overlays ( ) );

				g::hold_aim = false;
			}
		}

		g::local->set_abs_angles( vec3_t( 0.0f, local_data::abs, 0.0f ) );

		std::memcpy( g::local->layers( ), &local_data::overlays, N( sizeof animlayer_t ) * g::local->num_overlays( ) );
		g::local->poses( ) = local_data::poses;
	}
	else {
		g::local->set_abs_angles( vec3_t( 0.0f, local_data::abs, 0.0f ) );

		std::memcpy( g::local->layers( ), &local_data::overlays, N( sizeof animlayer_t ) * g::local->num_overlays( ) );
		g::local->poses( ) = local_data::poses;

		//setup_bones( g::local, nullptr, N( 256 ), vec3_t( 0.0f, local_data::abs, 0.0f ), g::local->abs_origin( ), csgo::i::globals->m_curtime );
//
		//if ( g::local->bone_cache( ) )
		//	std::memcpy( &data::fixed_bones [ g::local->idx( ) ], g::local->bone_cache( ), sizeof( matrix3x4_t ) * g::local->bone_count( ) );
		//features::lagcomp::cache( g::local );
	}
}

int animations::fix_pl( player_t* pl ) {
	if ( !pl->valid( ) || !pl->animstate( ) || !pl->layers( ) ) {
		if ( pl ) {
			data::origin [ pl->idx( ) ] = vec3_t( FLT_MAX, FLT_MAX, FLT_MAX );
			data::old_origin [ pl->idx( ) ] = vec3_t( FLT_MAX, FLT_MAX, FLT_MAX );
		}

		return 0;
	}

	const auto state = pl->animstate( );

	auto backup_vel = pl->vel( );
	auto backup_abs_origin = pl->abs_origin( );
	auto backup_origin = pl->origin( );
	auto backup_abs_vel = *reinterpret_cast< vec3_t* >( uintptr_t( pl ) + N( 0x94 ) );
	auto backup_eflags = *reinterpret_cast< uint32_t* >( uintptr_t( pl ) + N( 0xe8 ) );

	/* fix velocity */
	*reinterpret_cast< uint32_t* >( uintptr_t( pl ) + N( 0xe8 ) ) &= ~0x1000; /* EFL_DIRTY_ABSVELOCITY */
	*reinterpret_cast< vec3_t* >( uintptr_t( pl ) + N( 0x94 ) ) = pl->vel( );

	if ( pl->simtime( ) != pl->old_simtime( ) ) {
		data::choke [ pl->idx( ) ] = csgo::time2ticks( pl->simtime( ) - data::old_simtime [ pl->idx( ) ] ) - 1;
		data::old_simtime [ pl->idx( ) ] = pl->simtime( );

		if ( pl->layers( ) )
			pl->layers( ) [ 12 ].m_weight = 0.0f;

		std::memcpy( &data::overlays [ pl->idx( ) ], pl->layers( ), N( sizeof animlayer_t ) * pl->num_overlays( ) );

		data::old_tick [ pl->idx( ) ] = csgo::i::globals->m_tickcount;

		state->m_pitch = pl->angles( ).x;
		state->m_eye_yaw = pl->angles( ).y;

		update_animations( pl );

		state->m_pitch = pl->angles( ).x;
		state->m_eye_yaw = pl->angles( ).y;

		data::old_abs [ pl->idx( ) ] = state->m_abs_yaw;
		data::old_eye_yaw [ pl->idx( ) ] = state->m_eye_yaw;

		data::resolved1 [ pl->idx( ) ] = data::resolved2 [ pl->idx( ) ] = data::resolved3 [ pl->idx( ) ] = state->m_abs_yaw;
		//state->m_abs_yaw = data::old_abs [ pl->idx ( ) ];
		resolver::resolve( pl, data::resolved1 [ pl->idx( ) ], data::resolved2 [ pl->idx( ) ], data::resolved3 [ pl->idx( ) ] );

		/* build and store safe point matrix */
		//if ( safe_point && safe_point_key ) {
		//	const auto opposite_rotation = state->m_abs_yaw;
		//
		//	setup_bones ( pl, ( matrix3x4_t* ) &data::bones [ pl->idx ( ) ], N ( 256 ), vec3_t( 0.0f, state->m_abs_yaw, 0.0f ), pl->origin(), csgo::i::globals->m_curtime );
		//
		//	for ( auto& bone : data::bones [ pl->idx ( ) ] )
		//		bone.set_origin ( bone.origin ( ) - pl->origin( ) );
		//}

		pl->set_abs_origin( pl->origin( ) );

		state->m_abs_yaw = data::resolved1 [ pl->idx( ) ];
		state->m_feet_yaw = state->m_abs_yaw;
		rebuilt::poses::calculate( pl );
		//pl->setup_bones( reinterpret_cast< matrix3x4_t* >( &data::fixed_bones [ pl->idx( ) ] ), 128, 256, csgo::i::globals->m_curtime );
		setup_bones( pl, reinterpret_cast< matrix3x4_t* >( &data::fixed_bones [ pl->idx( ) ] ), 256, pl->abs_angles( ), pl->origin( ), pl->simtime( ) );

		state->m_abs_yaw = data::resolved2 [ pl->idx( ) ];
		state->m_feet_yaw = state->m_abs_yaw;
		rebuilt::poses::calculate( pl );
		//pl->setup_bones( reinterpret_cast< matrix3x4_t* >( &data::fixed_bones1 [ pl->idx( ) ] ), 128, 256, csgo::i::globals->m_curtime );
		setup_bones( pl, reinterpret_cast< matrix3x4_t* >( &data::fixed_bones1 [ pl->idx( ) ] ), 256, pl->abs_angles( ), pl->origin( ), pl->simtime( ) );

		state->m_abs_yaw = data::resolved3 [ pl->idx( ) ];
		state->m_feet_yaw = state->m_abs_yaw;
		rebuilt::poses::calculate( pl );
		//pl->setup_bones( reinterpret_cast< matrix3x4_t* >( &data::fixed_bones2 [ pl->idx( ) ] ), 128, 256, csgo::i::globals->m_curtime );
		setup_bones( pl, reinterpret_cast< matrix3x4_t* >( &data::fixed_bones2 [ pl->idx( ) ] ), 256, pl->abs_angles( ), pl->origin( ), pl->simtime( ) );

		pl->set_abs_origin( backup_abs_origin );
	}

	if ( pl->bone_cache( ) ) {
		if ( features::ragebot::get_misses( pl->idx( ) ).bad_resolve % 3 == 0 ) {
			state->m_abs_yaw = data::resolved1 [ pl->idx( ) ];
			state->m_feet_yaw = state->m_abs_yaw;
			pl->set_abs_angles( vec3_t( 0.0f, state->m_abs_yaw, 0.0f ) );
			//memcpy( pl->bone_cache( ), &data::fixed_bones [ pl->idx( ) ], sizeof( matrix3x4_t ) * pl->bone_count( ) );
		}
		else if ( features::ragebot::get_misses( pl->idx( ) ).bad_resolve % 3 == 1 ) {
			state->m_abs_yaw = data::resolved2 [ pl->idx( ) ];
			state->m_feet_yaw = state->m_abs_yaw;
			pl->set_abs_angles( vec3_t( 0.0f, state->m_abs_yaw, 0.0f ) );
			//memcpy( pl->bone_cache( ), &data::fixed_bones1 [ pl->idx( ) ], sizeof( matrix3x4_t ) * pl->bone_count( ) );
		}
		else {
			state->m_abs_yaw = data::resolved3 [ pl->idx( ) ];
			state->m_feet_yaw = state->m_abs_yaw;
			pl->set_abs_angles( vec3_t( 0.0f, state->m_abs_yaw, 0.0f ) );
			//memcpy( pl->bone_cache( ), &data::fixed_bones2 [ pl->idx( ) ], sizeof( matrix3x4_t ) * pl->bone_count( ) );
		}

		rebuilt::poses::calculate( pl );
		//pl->setup_bones( reinterpret_cast< matrix3x4_t* >( &data::bones [ pl->idx( ) ] ), 128, 256, csgo::i::globals->m_curtime );
	}

	pl->origin( ) = backup_origin;
	pl->abs_origin( ) = backup_abs_origin;
	pl->vel( ) = backup_vel;

	features::lagcomp::cache( pl );

	*reinterpret_cast< uint32_t* >( uintptr_t( pl ) + N( 0xe8 ) ) = backup_eflags;
}

struct varmapentry_t {
	uint16_t m_type;
	uint16_t m_needstointerpolate;
	void* data;
	void* watcher;
};

struct varmapping_t {
	varmapentry_t* m_entries;
	int m_interpolatedentries;
	float m_lastinterptime;
};

int animations::run( int stage ) {
	if ( stage == 5 )
		sesui::binds::frame_time = csgo::i::globals->m_frametime;

	if ( !csgo::i::engine->is_connected( ) || !csgo::i::engine->is_in_game( ) )
		return 0;

	switch ( stage ) {
		case 5: { /* fix local anims */
			RUN_SAFE(
				"animations::fake::simulate",
				animations::fake::simulate( );
			);

			RUN_SAFE(
				"animations::restore_local",
				animations::restore_local( true );
			);

			/*auto util_playerbyindex = [ ] ( int idx ) {
				using player_by_index_fn = player_t * ( __fastcall* )( int );
				static auto fn = pattern::search( _( "server.dll" ), _( "85 C9 7E 2A A1" ) ).get< player_by_index_fn >( );
				return fn( idx );
			};

			static auto draw_hitboxes = pattern::search( _( "server.dll" ), _( "55 8B EC 81 EC ? ? ? ? 53 56 8B 35 ? ? ? ? 8B D9 57 8B CE" ) ).get< std::uintptr_t >( );

			const auto dur = -1.f;

			for ( auto i = 1; i <= csgo::i::globals->m_max_clients; i++ ) {
				auto e_local = csgo::i::ent_list->get< player_t* >( i );

				if ( !e_local )
					continue;

				auto e = util_playerbyindex( e_local->idx( ) );

				if ( !e )
					continue;

				__asm {
					pushad
					movss xmm1, dur
					push 0
					mov ecx, e
					call draw_hitboxes
					popad
				}
			}*/
		} break;
		case 4: {
			security_handler::update( );

			RUN_SAFE(
				"animations::store_local",
				store_local( );
			);

			for ( auto i = 1; i <= csgo::i::globals->m_max_clients; i++ ) {
				const auto pl = csgo::i::ent_list->get< player_t* >( i );

				resolver::process_event_buffer( i );

				if ( !pl || !pl->is_player( ) || pl == g::local /*|| pl->team ( ) == g::local->team( )*/ ) {
					if ( pl && pl != g::local )
						pl->animate( ) = true;

					continue;
				}

				RUN_SAFE(
					"animations::fix_pl",
					fix_pl( pl );
				);
			}
		} break;
		case 2: /* store incoming data */ {
			//for ( auto i = 1; i <= csgo::i::globals->m_max_clients; i++ ) {
			//	const auto pl = csgo::i::ent_list->get< player_t* >( i );
//
			//	resolver::process_event_buffer( i );
//
			//	if ( !pl || !pl->is_player( ) || pl == g::local /*|| pl->team ( ) == g::local->team( )*/ ) {
			//		if ( pl && pl != g::local ) {
			//			auto map = reinterpret_cast< varmapping_t* >( uintptr_t( pl ) + 0x24 );
//
			//			if ( map )
			//				for ( auto i = 0; i < map->m_interpolatedentries; i++ )
			//					map->m_entries [ i ].m_needstointerpolate = true;
			//		}
//
			//		continue;
			//	}
//
			//	auto map = reinterpret_cast< varmapping_t* >( uintptr_t( pl ) + 0x24 );
//
			//	if ( map )
			//		for ( auto i = 0; i < map->m_interpolatedentries; i++ )
			//			map->m_entries [ i ].m_needstointerpolate = false;
			//}
		} break;
	}

	return 0;
}