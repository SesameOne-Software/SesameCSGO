#include "anims.hpp"
#include "resolver.hpp"

#include "../features/ragebot.hpp"

#undef max
#undef min

enum pose_param_t : int {
	strafe_yaw = 0,
	stand,
	lean_yaw,
	speed,
	ladder_yaw,
	ladder_speed,
	jump_fall,
	move_yaw,
	move_blend_crouch,
	move_blend_walk,
	move_blend_run,
	body_yaw,
	body_pitch,
	aim_blend_stand_idle,
	aim_blend_stand_walk,
	aim_blend_stand_run,
	aim_blend_courch_idle,
	aim_blend_crouch_walk,
	death_yaw
};

namespace anim_util {
	float angle_diff ( float dst, float src ) {
		auto delta = fmodf ( dst - src, 360.0f );

		if ( dst > src ) {
			if ( delta >= 180.0f )
				delta -= 360.0f;
		}
		else {
			if ( delta <= -180.0f )
				delta += 360.0f;
		}

		return delta;
	}

	// credits cbrs
	bool build_bones ( player_t* target, matrix3x4_t* mat, int mask, vec3_t rotation, vec3_t origin, float time, std::array<float, 24>& poses ) {
		/* vfunc indices */
		static auto standard_blending_rules_vfunc_idx = pattern::search ( _ ( "client.dll" ), _ ( "FF 90 ? ? ? ? 8B 47 FC" ) ).add ( 2 ).deref ( ).get< uint32_t > ( ) / 4;
		static auto build_transformations_vfunc_idx = pattern::search ( _ ( "client.dll" ), _ ( "FF 90 ? ? ? ? A1 ? ? ? ? B9 ? ? ? ? 8B 40 34 FF D0 85 C0 74 41" ) ).add ( 2 ).deref ( ).get< uint32_t > ( ) / 4;
		static auto update_ik_locks_vfunc_idx = pattern::search ( _ ( "client.dll" ), _ ( "FF 90 ? ? ? ? 8B 8F ? ? ? ? 8D 84 24 ? ? ? ? 50" ) ).add ( 2 ).deref ( ).get< uint32_t > ( ) / 4;
		static auto calculate_ik_locks_vfunc_idx = pattern::search ( _ ( "client.dll" ), _ ( "FF 90 ? ? ? ? 8B 8F ? ? ? ? 8D 84 24 ? ? ? ? 50 FF B7 ? ? ? ? 8D 84 24 ? ? ? ? 50 8D 84 24 ? ? ? ? 50 E8 ? ? ? ? 8B 47 FC 8D 4F FC 8B 80" ) ).add ( 2 ).deref ( ).get< uint32_t > ( ) / 4;

		/* func sigs */
		static auto init_iks = pattern::search ( _ ( "client.dll" ), _ ( "55 8B EC 83 EC 08 8B 45 08 56 57 8B F9 8D" ) ).get< void ( __thiscall* )( void*, void*, vec3_t&, vec3_t&, float, int, int ) > ( );
		static auto update_targets = pattern::search ( _ ( "client.dll" ), _ ( "55 8B EC 83 E4 F0 81 EC 18 01 00 00 33" ) ).get< void ( __thiscall* )( void*, vec3_t*, void*, void*, uint8_t* ) > ( );
		static auto solve_dependencies = pattern::search ( _ ( "client.dll" ), _ ( "55 8B EC 83 E4 F0 81 EC 98 05 00 00 8B 81" ) ).get< void ( __thiscall* )( void*, vec3_t*, void*, void*, uint8_t* ) > ( );

		/* offset sigs */
		static auto iks_off = pattern::search ( _ ( "client.dll" ), _ ( "8D 47 FC 8B 8F" ) ).add ( 5 ).deref ( ).add ( 4 ).get< uint32_t > ( );
		static auto effects_off = pattern::search ( _ ( "client.dll" ), _ ( "75 0D 8B 87" ) ).add ( 4 ).deref ( ).add ( 4 ).get< uint32_t > ( );

		const auto backup_poses = target->poses ( );

		auto cstudio = *reinterpret_cast< void** >( uintptr_t ( target ) + 0x294C );

		if ( !cstudio )
			return false;

		target->poses ( ) = poses;

		//  we need an aligned matrix in the bone accessor, so do this :) bad performance cause memcpy but that's ok
		matrix3x4a_t used [ 128 ];

		//	output shit
		uint8_t bone_computed [ 0x100 ] = { 0 };

		//	needs to be aligned
		matrix3x4a_t base_matrix;

		cs::angle_matrix ( rotation, origin, base_matrix );

		//	store shit
		const auto old_bones = target->bones ( );

		auto iks = *reinterpret_cast< void** >( uintptr_t ( target ) + iks_off );
		*reinterpret_cast< int* >( uintptr_t ( target ) + effects_off ) |= 8;

		//	clear iks & re-create them
		if ( iks ) {
			if ( *( uint32_t* ) ( uintptr_t ( iks ) + 0xFF0 ) > 0 ) {
				int v1 = 0;
				auto v62 = ( uint32_t* ) ( uintptr_t ( iks ) + 0xD0 );

				do {
					*v62 = -9999;
					v62 += 0x55;
					++v1;
				} while ( v1 < *( uint32_t* ) ( uintptr_t ( iks ) + 0xFF0 ) );
			}

			init_iks ( iks, cstudio, rotation, origin, time, cs::i::globals->m_framecount, 0x1000 );
		}

		static vec3_t pos [ 128 ];
		memset ( pos, 0, sizeof ( vec3_t [ 128 ] ) );

		void* q = malloc ( /* sizeof quaternion_t */ 48 * 128 );
		memset ( q, 0, /* sizeof quaternion_t */ 48 * 128 );

		//	set flags and bones
		target->bones ( ) = used;

		//	build some shit
		// IDX = 205 ( 2020.7.10 )
		vfunc< void ( __thiscall* )( player_t*, void*, vec3_t*, void*, float, uint32_t ) > ( target, standard_blending_rules_vfunc_idx ) ( target, cstudio, pos, q, time, mask );

		//	set iks
		if ( iks ) {
			vfunc< void ( __thiscall* )( player_t*, float ) > ( target, update_ik_locks_vfunc_idx ) ( target, time );
			update_targets ( iks, pos, q, target->bones ( ), bone_computed );
			vfunc< void ( __thiscall* )( player_t*, float ) > ( target, calculate_ik_locks_vfunc_idx ) ( target, time );
			solve_dependencies ( iks, pos, q, target->bones ( ), bone_computed );
		}

		//	build the matrix
		// IDX = 189 ( 2020.7.10 )
		vfunc< void ( __thiscall* )( player_t*, void*, vec3_t*, void*, matrix3x4a_t const&, uint32_t, void* ) > ( target, build_transformations_vfunc_idx ) ( target, cstudio, pos, q, base_matrix, mask, bone_computed );

		free ( q );

		//	restore flags and bones
		*reinterpret_cast< int* >( uintptr_t ( target ) + effects_off ) &= ~8;
		target->bones ( ) = old_bones;

		//  and pop out our new matrix
		memcpy ( mat, used, sizeof ( matrix3x4_t ) * 128 );

		target->poses ( ) = backup_poses;

		return true;
	}

	float local_poses_airtime = 0.0f;
	float local_poses_ground_time = 0.0f;
	float local_walk_to_run_transition = 0.0f;
	int local_walk_to_run_transition_state = 0;
	bool local_running = false;
	
	void update_local_poses ( player_t* ent ) {
		if ( !!( ent->flags ( ) & flags_t::on_ground ) )
			local_poses_ground_time = cs::i::globals->m_curtime;

		local_poses_airtime = cs::i::globals->m_curtime - local_poses_ground_time;

#define CSGO_ANIM_WALK_TO_RUN_TRANSITION_SPEED 2.0f
#define CSGO_ANIM_ONGROUND_FUZZY_APPROACH 8.0f
#define CSGO_ANIM_ONGROUND_FUZZY_APPROACH_CROUCH 16.0f
#define CSGO_ANIM_LADDER_CLIMB_COVERAGE 100.0f
#define CSGO_ANIM_RUN_ANIM_PLAYBACK_MULTIPLIER 0.85f
#define ANIM_TRANSITION_WALK_TO_RUN 0
#define ANIM_TRANSITION_RUN_TO_WALK 1
#define CS_PLAYER_SPEED_RUN 260.0f
#define CS_PLAYER_SPEED_WALK_MODIFIER 0.52f

		const auto speed_2d = ent->vel ( ).length_2d ( );

		if ( local_walk_to_run_transition > 0.0f && local_walk_to_run_transition < 1.0f )
		{
			//currently transitioning between walk and run
			if ( local_walk_to_run_transition_state == ANIM_TRANSITION_WALK_TO_RUN )
			{
				local_walk_to_run_transition += ent->animstate ( )->m_last_clientside_anim_update_time_delta * CSGO_ANIM_WALK_TO_RUN_TRANSITION_SPEED;
			}
			else // m_bWalkToRunTransitionState == ANIM_TRANSITION_RUN_TO_WALK
			{
				local_walk_to_run_transition -= ent->animstate ( )->m_last_clientside_anim_update_time_delta * CSGO_ANIM_WALK_TO_RUN_TRANSITION_SPEED;
			}
			local_walk_to_run_transition = std::clamp ( local_walk_to_run_transition, 0.0f, 1.0f );
		}

		if ( speed_2d > ( CS_PLAYER_SPEED_RUN * CS_PLAYER_SPEED_WALK_MODIFIER ) && local_walk_to_run_transition_state == ANIM_TRANSITION_RUN_TO_WALK )
		{
			//crossed the walk to run threshold
			local_walk_to_run_transition_state = ANIM_TRANSITION_WALK_TO_RUN;
			local_walk_to_run_transition = std::max ( 0.01f, local_walk_to_run_transition );
		}
		else if ( speed_2d < ( CS_PLAYER_SPEED_RUN * CS_PLAYER_SPEED_WALK_MODIFIER ) && local_walk_to_run_transition_state == ANIM_TRANSITION_WALK_TO_RUN )
		{
			//crossed the run to walk threshold
			local_walk_to_run_transition_state = ANIM_TRANSITION_RUN_TO_WALK;
			local_walk_to_run_transition = std::min ( 0.99f, local_walk_to_run_transition );
		}
	}
	
	void calc_poses ( player_t* ent, std::array<float, 24>& poses, float feet_yaw ) {
		/* copy existing poses*/
		poses = ent->poses ( );

		/* calculate *some* new ones */
		poses [ pose_param_t::speed ] = ent->vel().length_2d() / ( ( ( ent->weapon ( ) && ent->weapon ( )->data() ) ? std::max ( ent->weapon ( )->data ( )->m_max_speed, 0.001f ) : CS_PLAYER_SPEED_RUN ) * CS_PLAYER_SPEED_WALK_MODIFIER );
		poses [ pose_param_t::stand ] = 1.0f - ent->crouch_amount();

		poses [ pose_param_t::body_yaw] = std::clamp ( cs::normalize ( angle_diff ( cs::normalize ( ent->animstate ( )->m_eye_yaw ), cs::normalize ( feet_yaw ) ) ), -60.0f, 60.0f ) / 120.0f + 0.5f;
		poses [ pose_param_t::move_yaw ] = std::clamp ( cs::normalize ( angle_diff ( cs::normalize( cs::rad2deg ( atan2 ( -ent->vel ( ).y, -ent->vel ( ).x ) ) ), cs::normalize ( feet_yaw ) ) ), -180.0f, 180.0f ) / 360.0f + 0.5f;
		poses [ pose_param_t::lean_yaw ] = 0.0f;

		const float recalc_air_time = ( ent->layers ( ) [ 4 ].m_cycle - 0.72f ) * 1.25f;
		const float clamped = recalc_air_time >= 0.0f ? std::min ( recalc_air_time, 1.0f ) : 0.0f;
		float jump_fall = ( 3.0f - ( clamped + clamped ) ) * ( clamped * clamped );

		if ( jump_fall >= 0.0f )
			jump_fall = std::min( jump_fall, 1.0f );

		poses [ pose_param_t::jump_fall] = jump_fall;
		poses [ pose_param_t::body_pitch ] = std::clamp ( cs::normalize ( ent->animstate ( )->m_pitch ), -90.0f, 90.0f ) / 180.0f + 0.5f;

		poses [ pose_param_t::move_blend_crouch ] = ent->crouch_amount ( );
		poses [ pose_param_t::move_blend_run ] = ( ( ent == g::local ) ? local_walk_to_run_transition : ent->animstate ( )->m_ground_fraction ) * ( 1.0f - ent->crouch_amount ( ) );
		poses [ pose_param_t::move_blend_walk ] = ( 1.0f - ( ( ent == g::local ) ? local_walk_to_run_transition : ent->animstate ( )->m_ground_fraction ) ) * ( 1.0f - ent->crouch_amount ( ) );
	}

	void predict_movement ( player_t* ent, flags_t& old_flags, vec3_t& origin, vec3_t& vel, vec3_t& accel ) {
		/* predict velocity (only on x and y axes) */
		vel.x += accel.x * cs::ticks2time ( 1 );
		vel.y += accel.y * cs::ticks2time ( 1 );

		/* predict gravity's effect on velocity */
		if ( !( ent->flags ( ) & flags_t::on_ground ) )
			vel.z -= g::cvars::sv_gravity->get_float ( ) * cs::ticks2time ( 1 );
		//else if ( !( old_flags & flags_t::on_ground ) )
		//	vel.z = g::cvars::sv_jump_impulse->get_float ( );

		/* ideal position */
		const auto src = origin;
		const auto dst = origin + vel * cs::ticks2time ( 1 );

		trace_t tr {};
		cs::util_tracehull ( origin + vec3_t ( 0.0f, 0.0f, 2.0f ) /* add little offset so we dont fail on uneven ground*/, dst, ent->mins ( ), ent->maxs ( ), mask_playersolid, ent, &tr );

		/* predict position */
		origin = ( tr.m_fraction == 1.0f ) ? dst : tr.m_endpos;

		tr = trace_t {};
		cs::util_tracehull ( origin, origin - vec3_t ( 0.0f, 0.0f, 2.0f ) /* add little offset so we dont fail on uneven ground*/, ent->mins ( ), ent->maxs ( ), mask_playersolid, ent, &tr );

		/* predict flags */
		if ( tr.did_hit ( ) && tr.m_plane.m_normal.z > 0.7f )
			ent->flags ( ) |= flags_t::on_ground;
		else
			ent->flags ( ) &= ~flags_t::on_ground;

		/* calculate new velocity */
		vel = ( origin - src ) / cs::ticks2time ( 1 );
	}
}

void anims::reset_data ( int idx ) {
	if ( !anim_info [ idx ].empty ( ) )
		anim_info [ idx ].clear ( );

	auto ent = cs::i::ent_list->get<player_t*> ( idx );

	if ( ent )
		ent->animate ( ) = true;
}

void anims::manage_fake ( ) {
	if ( !g::local )
		return;

	static animstate_t fake_anim_state { };
	static auto handle = g::local->handle ( );
	static auto spawn_time = g::local->spawn_time ( );

	if ( !g::local->alive ( ) ) {
		spawn_time = 0.0f;
		return;
	}

	bool reset = g::local->spawn_time ( ) != spawn_time || g::local->handle ( ) != handle;

	if ( reset ) {
		g::local->create_animstate ( &fake_anim_state );

		handle = g::local->handle ( );
		spawn_time = g::local->spawn_time ( );
	}

	//if ( g::send_packet ) {
	std::array<animlayer_t, 13> networked_layers = g::local->layers ( );

	auto backup_abs_angles = g::local->abs_angles ( );
	auto backup_poses = g::local->poses ( );
	auto origin = g::local->origin ( );

	fake_anim_state.m_last_clientside_anim_framecount = 0;
	fake_anim_state.m_feet_yaw_rate = 0.0f;
	fake_anim_state.update ( g::angles );

	g::local->set_abs_angles ( vec3_t ( 0.0f, fake_anim_state.m_abs_yaw, 0.0f ) );

	anim_util::build_bones ( g::local, fake_matrix.data ( ), 0x7FF00, vec3_t ( 0.0f, fake_anim_state.m_abs_yaw, 0.0f ), origin, cs::i::globals->m_curtime, g::local->poses ( ) );

	g::local->layers ( ) = networked_layers;
	g::local->poses ( ) = backup_poses;
	g::local->set_abs_angles ( backup_abs_angles );

	for ( auto& iter : fake_matrix )
		iter.set_origin ( iter.origin ( ) - origin );
	//}
}

std::array< animlayer_t, 13> last_anim_layers_queued {};
std::array< animlayer_t, 13> last_anim_layers {};
float last_local_feet_yaw;
std::array<float, 24> last_local_poses;

void anims::update_anims ( player_t* ent, vec3_t& angles ) {
	static auto& invalidate_bone_cache = **reinterpret_cast< bool** >( pattern::search ( _ ( "client.dll" ), _ ( "C6 05 ? ? ? ? ? 89 47 70" ) ).add ( 2 ).get<uint32_t> ( ) );
	static auto invalidate_physics_recursive = pattern::search ( _ ( "client.dll" ), _ ( "55 8B EC 83 E4 F8 83 EC 0C 53 8B 5D 08 8B C3 56" ) ).get<void ( __thiscall* )( player_t*, int )> ( );

	const float backup_frametime = cs::i::globals->m_frametime;
	const float backup_curtime = cs::i::globals->m_curtime;

	const auto state = ent->animstate ( );

	if ( !state )
		return;

	static bool init_last_data = false;

	if ( !init_last_data ) {
		last_local_feet_yaw = state->m_abs_yaw;
		last_local_poses = ent->poses ( );
		last_anim_layers = ent->layers ( );
		init_last_data = true;
	}
	
	if ( ent != g::local ) {
		cs::i::globals->m_frametime = cs::ticks2time ( 1 );
		cs::i::globals->m_curtime = ent->simtime ( );

		*reinterpret_cast< uint32_t* >( reinterpret_cast< uintptr_t > ( ent ) + 0xE8 ) &= ~0x1000;
		*reinterpret_cast< vec3_t* >( reinterpret_cast< uintptr_t > ( ent ) + 0x94 ) = ent->vel();
	}
	else {
		//state->m_last_clientside_anim_update_time_delta = std::max ( 0.0f, cs::i::globals->m_curtime - state->m_last_clientside_anim_update );
	}

	state->m_last_clientside_anim_framecount = 0;
	state->m_feet_yaw_rate = 0.0f;

	const bool backup_invalidate_bone_cache = invalidate_bone_cache;
	
	std::array<animlayer_t, 13> backup_animlayers = ent->layers ( );
	ent->layers ( ) [ 12 ].m_weight = 0.0f;

	ent->animate ( ) = true;
	ent->update ( );
	//state->update ( angles );
	ent->animate ( ) = false;

	ent->layers ( ) = backup_animlayers;
	ent->layers ( ) [ 12 ].m_weight = 0.0f;

	invalidate_physics_recursive ( ent, 2 /* ANGLES_CHANGED */ );
	invalidate_physics_recursive ( ent, 8 /* ANIMATION_CHANGED */ );
	invalidate_physics_recursive ( ent, 32 /* SEQUENCE_CHANGED */ );
	
	invalidate_bone_cache = backup_invalidate_bone_cache;	
	
	if ( ent != g::local ) {
		cs::i::globals->m_frametime = backup_frametime;
		cs::i::globals->m_curtime = backup_curtime;

		auto yaw_offset = resolver::resolve_yaw ( ent );

		if ( yaw_offset == std::numeric_limits<float>::max ( ) ) {
			std::array<float, 24> calc_poses {};

			anim_util::calc_poses ( ent, calc_poses, state->m_abs_yaw );
			
			const auto idx = ent->idx ( );

			anim_util::build_bones ( ent, aim_matricies [ idx ][0].data ( ), 256, vec3_t ( 0.0f, state->m_abs_yaw, 0.0f ), ent->origin ( ), ent->simtime ( ), calc_poses );
			
			for ( auto& aim_mat : aim_matricies [ idx ] )
				aim_mat = aim_matricies [ idx ][ 0 ];

			ent->poses ( ) = calc_poses;
			ent->set_abs_angles ( vec3_t ( 0.0f, state->m_abs_yaw, 0.0f ) );
		}
		else {
			auto resolved_angle1 = cs::normalize ( state->m_eye_yaw + yaw_offset );
			auto resolved_angle2 = cs::normalize ( state->m_eye_yaw - yaw_offset );
			auto resolved_angle3 = cs::normalize ( state->m_eye_yaw + ( yaw_offset < 0.0f ? -yaw_offset * 0.5f : 0.0f ) );

			std::array<float, 24> resolved_poses1 {};
			std::array<float, 24> resolved_poses2 {};
			std::array<float, 24> resolved_poses3 {};

			anim_util::calc_poses ( ent, resolved_poses1, resolved_angle1 );
			anim_util::calc_poses ( ent, resolved_poses2, resolved_angle2 );
			anim_util::calc_poses ( ent, resolved_poses3, resolved_angle3 );

			const auto idx = ent->idx ( );

			anim_util::build_bones ( ent, aim_matricies [ idx ][ 0 ].data ( ), 256, vec3_t ( 0.0f, resolved_angle1, 0.0f ), ent->origin ( ), ent->simtime ( ), resolved_poses1 );
			anim_util::build_bones ( ent, aim_matricies [ idx ][ 1 ].data ( ), 256, vec3_t ( 0.0f, resolved_angle2, 0.0f ), ent->origin ( ), ent->simtime ( ), resolved_poses2 );
			anim_util::build_bones ( ent, aim_matricies [ idx ][ 2 ].data ( ), 256, vec3_t ( 0.0f, resolved_angle3, 0.0f ), ent->origin ( ), ent->simtime ( ), resolved_poses3 );

			const auto misses = features::ragebot::get_misses ( ent->idx ( ) ).bad_resolve;

			if ( misses % 3 == 0 ) {
				ent->poses ( ) = resolved_poses1;
				ent->set_abs_angles ( vec3_t ( 0.0f, resolved_angle1, 0.0f ) );
			}
			else if ( misses % 3 == 1 ) {
				ent->poses ( ) = resolved_poses2;
				ent->set_abs_angles ( vec3_t ( 0.0f, resolved_angle2, 0.0f ) );
			}
			else {
				ent->poses ( ) = resolved_poses3;
				ent->set_abs_angles ( vec3_t ( 0.0f, resolved_angle3, 0.0f ) );
			}
		}
	}
	else {
		/* calc animlayers */
		anim_util::update_local_poses ( ent );

		/* dont update on local until we send packet */
		if ( !cs::i::client_state->choked ( ) ) {
			memcpy ( last_anim_layers.data ( ), last_anim_layers_queued.data ( ), sizeof ( last_anim_layers ) );
			last_local_feet_yaw = state->m_abs_yaw;
			anim_util::calc_poses ( ent, last_local_poses, last_local_feet_yaw );
		}
		
		ent->poses ( ) = last_local_poses;
		ent->layers ( ) = last_anim_layers;

		ent->set_abs_angles ( vec3_t ( 0.0f, last_local_feet_yaw, 0.0f ) );

		if ( !cs::i::client_state->choked ( ) ) {
			manage_fake ( );
		}
	}
	
	/* store animation info */
	anim_info [ ent->idx() ].push_front ( anim_info_t ( ent ) );

	while ( anim_info [ ent->idx ( ) ].size ( ) > 16 )
		anim_info [ ent->idx ( ) ].pop_back ( );
}

void anims::apply_anims ( player_t* ent ) {
	if ( ent == g::local && g::local ) {
		ent->poses ( ) = last_local_poses;
		ent->set_abs_angles ( vec3_t( 0.0f, last_local_feet_yaw, 0.0f ) );
		ent->layers ( ) = last_anim_layers;
	}
	else {
		auto& info = anim_info [ ent->idx ( ) ];

		if ( info.empty ( ) )
			return;

		auto& first = info.front ( );

		ent->set_abs_angles ( first.m_abs_angles );
		ent->poses ( ) = first.m_poses;
	}
}

void anims::on_net_update_end ( int idx ) {
	auto ent = cs::i::ent_list->get<player_t*> ( idx );
	
	if ( !ent || !ent->is_player ( ) || !ent->alive ( ) || ent->dormant ( ) || !ent->animstate ( ) )
		return reset_data ( idx );

	ent->animate ( ) = false;

	if ( ent != g::local && ent->simtime ( ) > ent->old_simtime ( ) ) {
		update_anims ( ent, ent->angles ( ) );

		if ( g::local && ent->team ( ) != g::local->team ( ) )
			features::lagcomp::cache ( ent );
	}

	if ( ent == g::local /*&& ent->simtime ( ) > ent->old_simtime ( )*/ ) {
		ent->layers ( ) [ 4 ].m_cycle = anim_util::local_poses_airtime;
		ent->layers ( ) [ 4 ].m_playback_rate = ( ( anim_util::local_poses_airtime > 0.0f ) ? cs::ticks2time(1) : 0.0f );
		ent->layers ( ) [ 4 ].m_weight = (( anim_util::local_poses_airtime > 0.0f) ? 1.0f : 0.0f );

		last_anim_layers_queued = ent->layers ( );
	}
}

void anims::on_render_start ( int idx ) {
	auto ent = cs::i::ent_list->get<player_t*> ( idx );

	if ( !ent || !ent->is_player ( ) || !ent->alive ( ) || ent->dormant ( ) || !ent->animstate ( ) )
		return reset_data ( idx );

	ent->animate ( ) = false;

	apply_anims ( ent );

	//static int tick_count = cs::i::globals->m_tickcount;
	//if ( tick_count != cs::i::globals->m_tickcount && g::local && g::local->alive() && g::local->animstate ( ))
	//	anim_util::build_bones ( g::local, real_matrix.data ( ), 256 /* BONE_USED_BY_HITBOX */, vec3_t ( 0.0f, g::local->animstate ( )->m_abs_yaw, 0.0f ), g::local->origin ( ), cs::i::globals->m_curtime, g::local->poses() );
}

void anims::pre_fsn ( int stage ) {
	switch ( stage ) {
	case 5: {
		for ( auto i = 1; i <= cs::i::globals->m_max_clients; i++ ) {
			resolver::process_event_buffer ( i );

			on_render_start ( i );
		}
	} break;
	}
}

void anims::fsn ( int stage ) {
	switch ( stage ) {
	case 5: {
		for ( auto i = 1; i <= cs::i::globals->m_max_clients; i++ ) {
			auto ent = cs::i::ent_list->get<player_t*> ( i );

			if ( !ent || !ent->is_player() || !ent->alive ( ) )
				continue;

			/* update viewmodel manually tox fix it dissappearing*/
			using update_all_viewmodel_addons_t = int ( __fastcall* )( void* );
			static auto update_all_viewmodel_addons = pattern::search ( _ ( "client.dll" ), _ ( "55 8B EC 83 E4 ? 83 EC ? 53 8B D9 56 57 8B 03 FF 90 ? ? ? ? 8B F8 89 7C 24 ? 85 FF 0F 84 ? ? ? ? 8B 17 8B CF" ) ).get< update_all_viewmodel_addons_t > ( );

			if ( ent->viewmodel_handle ( ) != 0xFFFFFFFF && cs::i::ent_list->get_by_handle< void* > ( ent->viewmodel_handle ( ) ) )
				update_all_viewmodel_addons ( cs::i::ent_list->get_by_handle< void* > ( ent->viewmodel_handle ( ) ) );
		}
	} break;
	case 4: {
		for ( auto i = 1; i <= cs::i::globals->m_max_clients; i++ )
			on_net_update_end ( i );
	} break;
	}
}