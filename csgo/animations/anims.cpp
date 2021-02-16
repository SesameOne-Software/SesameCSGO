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
	__forceinline float angle_diff ( float dst, float src ) {
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
	__forceinline bool build_bones ( player_t* target, matrix3x4_t* mat, int mask, vec3_t rotation, vec3_t origin, float time, std::array<float, 24>& poses ) {
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
	
	__forceinline void update_local_poses ( player_t* ent ) {
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
	
	__forceinline void calc_poses ( player_t* ent, std::array<float, 24>& poses, float feet_yaw ) {
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
		poses [ pose_param_t::move_blend_run ] = ( ( g::local && ent == g::local ) ? local_walk_to_run_transition : ent->animstate ( )->m_ground_fraction ) * ( 1.0f - ent->crouch_amount ( ) );
		poses [ pose_param_t::move_blend_walk ] = ( 1.0f - ( ( g::local && ent == g::local ) ? local_walk_to_run_transition : ent->animstate ( )->m_ground_fraction ) ) * ( 1.0f - ent->crouch_amount ( ) );
	}

	__forceinline void predict_movement ( player_t* ent, flags_t& old_flags, vec3_t& origin, vec3_t& vel ) {
		vec3_t start, end, normal;
		trace_t trace;
		trace_filter_t filter;

		start = origin;
		end = start + vel * cs::ticks2time ( 1 );

		cs::util_tracehull ( start, end, ent->mins(), ent->maxs(), mask_playersolid, ent, &trace );

		if ( trace.m_fraction != 1.0f ) {
			for ( auto i = 0; i < 2; ++i ) {
				vel -= trace.m_plane.m_normal * vel.dot_product ( trace.m_plane.m_normal );

				float adjust = vel.dot_product ( trace.m_plane.m_normal );

				if ( adjust < 0.0f )
					vel -= ( trace.m_plane.m_normal * adjust );

				start = trace.m_endpos;
				end = start + ( vel * ( cs::ticks2time ( 1 ) * ( 1.0f - trace.m_fraction ) ) );

				cs::util_tracehull ( start, end, ent->mins ( ), ent->maxs ( ), mask_playersolid, ent, &trace );

				if ( trace.m_fraction == 1.0f )
					break;
			}
		}

		start = end = origin = trace.m_endpos;
		end.z -= 2.0f;

		cs::util_tracehull ( start, end, ent->mins ( ), ent->maxs ( ), mask_playersolid, ent, &trace );

		old_flags &= ~flags_t::on_ground;

		if ( trace.m_fraction != 1.0f && trace.m_plane.m_normal.z > 0.7f )
			old_flags |= flags_t::on_ground;
	}
}

bool anims::get_lagcomp_bones ( player_t* ent, std::array<matrix3x4_t, 128>& out ) {
	const auto nci = cs::i::engine->get_net_channel_info ( );

	auto idx = ent->idx ( );

	if ( !idx || idx > cs::i::globals->m_max_clients )
		return false;

	auto& data = anim_info [ idx ];

	if ( data.empty ( ) )
		return false;

	const auto lerp = lerp_time ( );

	auto time_valid_no_deadtime = [ & ] ( float t ) {
		const auto correct = std::clamp ( nci->get_avg_latency ( 0 ) + nci->get_avg_latency ( 1 ) + lerp, 0.0f, g::cvars::sv_maxunlag->get_float ( ) );
		return abs ( correct - ( cs::ticks2time ( g::local->tick_base ( ) ) - t ) ) <= 0.2f;
	};

	for ( int i = static_cast< int >( data.size ( ) ) - 1; i >= 0; i-- ) {
		auto& it = data [ i ];

		if ( time_valid_no_deadtime ( it.m_simtime ) ) {
			if ( it.m_origin.dist_to ( ent->origin ( ) ) < 1.0f )
				return false;

			bool end = ( i - 1 ) <= 0;
			vec3_t next = end ? ent->origin ( ) : data [ i - 1 ].m_origin;
			float time_next = end ? ent->simtime ( ) : data [ i - 1 ].m_simtime;

			float correct = nci->get_avg_latency ( 0 ) + nci->get_avg_latency ( 1 ) + lerp;
			float time_delta = time_next - it.m_simtime;
			float add = end ? 0.2f : time_delta;
			float deadtime = it.m_simtime + correct + add;

			float delta = deadtime - cs::ticks2time( g::local->tick_base ( ) );

			float mul = 1.0f / add;

			vec3_t lerp = next + ( it.m_origin - next ) * std::clamp ( delta * mul, 0.0f, 1.0f );

			out = it.m_render_bones;

			for ( auto& iter : out )
				iter.set_origin ( iter.origin ( ) - it.m_origin + lerp );

			return true;
		}
	}

	return false;
}

void anims::reset_data ( int idx ) {
	if ( !anim_info [ idx ].empty ( ) )
		anim_info [ idx ].clear ( );

	if ( !predicted_anim_info [ idx ].empty ( ) )
		predicted_anim_info [ idx ].clear ( );

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
	static float last_ground_time = cs::i::globals->m_curtime;

	if ( !!( g::local->flags ( ) & flags_t::on_ground ) )
		last_ground_time = cs::i::globals->m_curtime;

	const auto last_air_time = cs::i::globals->m_curtime - last_ground_time;

	std::array<animlayer_t, 13> networked_layers;
	memcpy ( networked_layers.data(), g::local->layers ( ), sizeof( networked_layers ) );

	auto backup_abs_angles = g::local->abs_angles ( );
	auto backup_poses = g::local->poses ( );
	auto origin = g::local->origin ( );

	fake_anim_state.m_last_clientside_anim_framecount = 0;
	fake_anim_state.m_feet_yaw_rate = 0.0f;
	fake_anim_state.update ( g::angles );

	g::local->abs_angles ( ).y = fake_anim_state.m_abs_yaw;

	auto poses = g::local->poses ( );
	anim_util::calc_poses ( g::local, poses, fake_anim_state.m_abs_yaw );

	const float recalc_air_time = ( last_air_time - 0.72f ) * 1.25f;
	const float clamped = recalc_air_time >= 0.0f ? std::min ( recalc_air_time, 1.0f ) : 0.0f;
	float jump_fall = ( 3.0f - ( clamped + clamped ) ) * ( clamped * clamped );

	if ( jump_fall >= 0.0f )
		jump_fall = std::min ( jump_fall, 1.0f );

	poses [ pose_param_t::jump_fall ] = jump_fall;
	
	anim_util::build_bones ( g::local, fake_matrix.data ( ), 0x7FF00, vec3_t ( 0.0f, fake_anim_state.m_abs_yaw, 0.0f ), origin, cs::i::globals->m_curtime, poses );

	memcpy ( g::local->layers ( ), networked_layers.data ( ), sizeof ( networked_layers ) );
	g::local->poses ( ) = backup_poses;
	g::local->abs_angles ( ).y = backup_abs_angles.y;

	for ( auto& iter : fake_matrix )
		iter.set_origin ( iter.origin ( ) - origin );
	//}
}

std::array< animlayer_t, 13> last_anim_layers_queued {};
std::array< animlayer_t, 13> last_anim_layers {};
float last_local_feet_yaw;
std::array<float, 24> last_local_poses;

void anims::fix_velocity ( player_t* ent, vec3_t& vel ) {
	const auto idx = ent->idx ( );
	const auto anim_layers = ent->layers ( );
	const auto& records = anim_info [ idx ];

	if ( !!( ent->flags ( ) & flags_t::on_ground ) && vel.length_2d() > 0.0f && !anim_layers[6].m_weight )
		vel.zero();

	if ( !records.size ( ) )
		return;

	const auto previous_record = records.front ( );

	auto was_in_air = !!( ent->flags ( ) & flags_t::on_ground ) && !!( previous_record.m_flags & flags_t::on_ground );

	auto time_difference = std::max ( cs::ticks2time(1), ent->simtime ( ) - previous_record.m_simtime );
	auto origin_delta = ent->origin ( ) - previous_record.m_origin;

	auto animation_speed = 0.0f;

	if ( origin_delta.is_zero ( ) || cs::time2ticks ( time_difference ) <= 0 )
		return;

	vel = origin_delta * ( 1.0f / time_difference );

	if ( !!( ent->flags ( ) & flags_t::on_ground ) && anim_layers [ 11 ].m_weight > 0.0f && anim_layers [ 11 ].m_weight < 1.0f && anim_layers [ 11 ].m_cycle > previous_record.m_anim_layers [ 11 ].m_cycle ) {
		auto weapon = ent->weapon ( );

		if ( weapon ) {
			auto max_speed = 260.0f;
			auto weapon_info = weapon->data ( );

			if ( weapon_info )
				max_speed = ent->scoped ( ) ? weapon_info->m_max_speed_alt : weapon_info->m_max_speed;

			auto modifier = 0.35f * ( 1.0f - anim_layers [ 11 ].m_weight );

			if ( modifier > 0.0f && modifier < 1.0f )
				animation_speed = max_speed * ( modifier + 0.55f );
		}
	}

	if ( animation_speed > 0.0f ) {
		animation_speed /= vel.length_2d ( );

		vel.x *= animation_speed;
		vel.y *= animation_speed;
	}

	if ( records.size ( ) >= 2 && time_difference > cs::ticks2time ( 1 ) ) {
		auto previous_velocity = ( previous_record.m_origin - records [ 1 ].m_origin ) * ( 1.0f / time_difference );

		if ( !previous_velocity.is_zero ( ) && !was_in_air ) {
			auto current_direction = cs::normalize ( cs::rad2deg ( atan2 ( vel.y, vel.x ) ) );
			auto previous_direction = cs::normalize ( cs::rad2deg ( atan2 ( previous_velocity.y, previous_velocity.x ) ) );

			auto average_direction = current_direction - previous_direction;
			average_direction = cs::deg2rad ( cs::normalize ( current_direction + average_direction * 0.5f ) );

			auto direction_cos = cos ( average_direction );
			auto dirrection_sin = sin ( average_direction );

			auto velocity_speed = vel.length_2d ( );

			vel.x = direction_cos * velocity_speed;
			vel.y = dirrection_sin * velocity_speed;
		}
	}

	if ( !( ent->flags ( ) & flags_t::on_ground ) ) {
		auto fixed_timing = std::clamp ( time_difference, cs::ticks2time ( 1 ), 1.0f );
		vel.z -= g::cvars::sv_gravity->get_float ( ) * fixed_timing * 0.5f;
	}
	else
		vel.z = 0.0f;
}

void anims::process_networked_anims ( player_t* ent ) {
	const auto idx = ent->idx ( );

	static std::array< matrix3x4_t, 128 > aim_bones;

	/* fix velocity */
	fix_velocity ( ent, ent->vel() );

	update_anims ( ent, ent->angles(), true, &aim_bones );

	/* store animation info */
	auto data_tmp = anim_info_t ( ent, ent->abs_angles().y );

	data_tmp.m_aim_bones = aim_bones;

	anim_info [ idx ].push_front ( data_tmp );

	//const auto nci = cs::i::engine->get_net_channel_info ( );
	//while ( nci && !anim_info [ idx ].empty ( ) && abs ( ent->simtime() - anim_info [ idx ].back ( ).m_simtime ) > nci->get_latency ( 0 ) + nci->get_latency ( 1 ) + g::cvars::sv_maxunlag->get_float ( ) + 0.2f + lerp_time ( ) )
	//	anim_info [ idx ].pop_back ( );

	if ( !predicted_anim_info [ idx ].empty ( ) )
		predicted_anim_info [ idx ].clear ( );

	last_update_time [ idx ] = ent->simtime ( );
	has_been_predicted [ idx ] = false;
}

void anims::process_predicted_anims ( player_t* ent, bool resolve ) {
	const auto idx = ent->idx ( );
	const auto movement_weight = ent->layers ( ) [ 6 ].m_weight;

	/* calculate instantaneous velocity to animate with */
	/*if ( predicted_anim_info [ idx ].size ( ) == 1 && !!( predicted_anim_info [ idx ].front ( ).m_flags & flags_t::on_ground ) ) {
		if ( !movement_weight ) {
			predicted_anim_info [ idx ].front ( ).m_vel.y = predicted_anim_info [ idx ].front ( ).m_vel.x = 0.0f;
		}
		else if ( movement_weight < 1.0f ) {
			const auto max_vel = ent->scoped ( ) ? ent->weapon ( )->data ( )->m_max_speed_alt : ent->weapon ( )->data ( )->m_max_speed;
			const auto new_mag = ( max_vel * CS_PLAYER_SPEED_WALK_MODIFIER ) * movement_weight;
			const auto unit_vec = predicted_anim_info [ idx ].front ( ).m_vel.normalized ( );
			predicted_anim_info [ idx ].front ( ).m_vel.y = unit_vec.y * new_mag;
			predicted_anim_info [ idx ].front ( ).m_vel.x = unit_vec.x * new_mag;
		}
	}*/

	auto last_predicted_data = predicted_anim_info [ idx ].front ( );

	/* apply gravity */
	if ( !( last_predicted_data.m_flags & flags_t::on_ground ) )
		last_predicted_data.m_vel.z -= g::cvars::sv_gravity->get_float ( ) * cs::ticks2time( 1 );
	else if ( predicted_anim_info [ idx ].size() > 1 && !( predicted_anim_info [ idx ][1].m_flags & flags_t::on_ground ) )
		last_predicted_data.m_vel.z = g::cvars::sv_jump_impulse->get_float();

	/* predict new player average velocity (for calculating player movement) */
	if ( anim_info [ idx ].size ( ) > 1 && ( !!( last_predicted_data.m_flags & flags_t::on_ground ) ? movement_weight : true ) ) {
		//const auto delta_ticks = anim_info [ idx ][ 0 ].m_choked_commands + 1;
		//const auto vel_dir = cs::normalize( cs::rad2deg ( atan2 ( anim_info [ idx ][ 0 ].m_vel.y, anim_info [ idx ][ 0 ].m_vel.x )));
		//const auto last_vel_dir = cs::normalize ( cs::rad2deg ( atan2 ( anim_info [ idx ][ 1 ].m_vel.y, anim_info [ idx ][ 1 ].m_vel.x )));
		//const auto last_angle_change = cs::normalize ( vel_dir - last_vel_dir ) / static_cast< float >( delta_ticks );
		//const auto last_vel_mag_change = ( anim_info [ idx ][ 0 ].m_vel.length_2d ( ) - anim_info [ idx ][ 1 ].m_vel.length_2d ( ) ) / static_cast< float >( delta_ticks );
		//
		//const auto new_angle = cs::normalize( cs::normalize ( cs::rad2deg ( atan2 ( last_predicted_data.m_vel.y, last_predicted_data.m_vel.x ) ) ) + last_angle_change );
		//const auto new_mag = last_predicted_data.m_vel.length_2d ( ) + last_vel_mag_change;
		//
		//last_predicted_data.m_vel.y = sin ( cs::deg2rad ( new_angle ) ) * new_mag;
		//last_predicted_data.m_vel.x = cos ( cs::deg2rad ( new_angle ) ) * new_mag;

		const auto delta_vel = (anim_info [ idx ][ 1 ].m_vel - anim_info [ idx ][ 0 ].m_vel) / static_cast<float>( anim_info [ idx ][ 0 ].m_choked_commands + 1 );
		last_predicted_data.m_vel += vec3_t ( delta_vel.x, delta_vel.y, 0.0f );
	}

	if ( !movement_weight && !!( last_predicted_data.m_flags & flags_t::on_ground ) )
		last_predicted_data.m_vel.y = last_predicted_data.m_vel.x = 0.0f;

	/* simulate player movement */
	anim_util::predict_movement ( ent, last_predicted_data.m_flags, last_predicted_data.m_origin, last_predicted_data.m_vel );

	/* update animations with simulated data */
	last_predicted_data.m_simtime = last_update_time [ idx ] + cs::ticks2time ( 1 );
	last_predicted_data.m_old_simtime = last_update_time [ idx ];
	last_predicted_data.m_choked_commands = std::clamp ( cs::time2ticks ( ( last_update_time [ idx ] + cs::ticks2time ( 1 ) ) - ent->simtime ( ) ) - 1, 0, g::cvars::sv_maxusrcmdprocessticks->get_int ( ) );

	/* set important animstate vars */
	last_predicted_data.m_anim_state.m_pitch = last_predicted_data.m_angles.x;
	last_predicted_data.m_anim_state.m_eye_yaw = last_predicted_data.m_angles.y;
	last_predicted_data.m_anim_state.m_last_clientside_anim_update = last_predicted_data.m_old_simtime;
	last_predicted_data.m_anim_state.m_last_clientside_anim_update_time_delta = last_predicted_data.m_simtime - last_predicted_data.m_old_simtime;

	/* backup current data */
	const auto backup_data = anim_info_t ( ent, last_predicted_data.m_anim_state.m_abs_yaw );
	const auto backup_abs_angles = ent->abs_angles ( ).y;
	auto backup_abs_origin = ent->abs_origin ( );

	/* set data to animate with */
	ent->angles ( ) = last_predicted_data.m_angles;
	ent->origin ( ) = last_predicted_data.m_origin;
	ent->lby ( ) = last_predicted_data.m_lby;
	ent->simtime ( ) = last_predicted_data.m_simtime;
	ent->old_simtime ( ) = last_predicted_data.m_old_simtime;
	ent->flags ( ) = last_predicted_data.m_flags;
	memcpy ( ent->layers ( ), last_predicted_data.m_anim_layers.data ( ), sizeof ( last_predicted_data.m_anim_layers ) );
	ent->poses ( ) = last_predicted_data.m_poses;
	ent->vel ( ) = last_predicted_data.m_vel;
	ent->set_abs_origin ( last_predicted_data.m_origin );
	ent->set_abs_angles ( last_predicted_data.m_abs_angles );
	*ent->animstate ( ) = last_predicted_data.m_anim_state;

	/* animate player 1 tick */
	update_anims ( ent, last_predicted_data.m_angles, resolve, &last_predicted_data.m_aim_bones /* update anim layers as well */, true );

	/* store output */
	memcpy ( last_predicted_data.m_anim_layers.data ( ), ent->layers ( ), sizeof ( last_predicted_data.m_anim_layers ) );
	last_predicted_data.m_poses = ent->poses ( );
	last_predicted_data.m_abs_angles = ent->abs_angles ( );
	last_predicted_data.m_anim_state = *ent->animstate ( );
	last_predicted_data.m_predicted = true;

	/* restore backup data */
	ent->angles ( ) = backup_data.m_angles;
	ent->origin ( ) = backup_data.m_origin;
	ent->lby ( ) = backup_data.m_lby;
	ent->simtime ( ) = backup_data.m_simtime;
	ent->old_simtime ( ) = backup_data.m_old_simtime;
	ent->flags ( ) = backup_data.m_flags;
	memcpy ( ent->layers ( ), backup_data.m_anim_layers.data ( ), sizeof ( backup_data.m_anim_layers ) );
	ent->poses ( ) = backup_data.m_poses;
	ent->vel ( ) = backup_data.m_vel;
	ent->set_abs_origin ( backup_abs_origin );
	ent->set_abs_angles ( vec3_t( 0.0f, backup_abs_angles, 0.0f ) );
	*ent->animstate ( ) = backup_data.m_anim_state;

	predicted_anim_info [ idx ].push_front ( last_predicted_data );
	last_update_time [ idx ] += cs::ticks2time ( 1 );
}

namespace lby {
	extern bool in_update;
}

void anims::update_anims ( player_t* ent, vec3_t& angles, bool resolve, std::array< matrix3x4_t, 128 >* bones_out, bool update_anim_layers ) {
	VM_TIGER_BLACK_START
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
		memcpy ( last_anim_layers.data(), ent->layers ( ),sizeof( last_anim_layers ) );
		init_last_data = true;
	}
	
	if ( g::local && ent != g::local ) {
		cs::i::globals->m_frametime = cs::ticks2time ( 1 );
		cs::i::globals->m_curtime = ent->simtime ( );

		*reinterpret_cast< uint32_t* >( reinterpret_cast< uintptr_t > ( ent ) + 0xE8 ) &= ~0x1000;
		*reinterpret_cast< vec3_t* >( reinterpret_cast< uintptr_t > ( ent ) + 0x94 ) = ent->vel();

		state->m_last_clientside_anim_update_time_delta = std::max ( 0.0f, ent->simtime ( ) - ent->old_simtime ( ) );
	}
	else {
		cs::i::globals->m_frametime = cs::ticks2time ( 1 );
		state->m_last_clientside_anim_update_time_delta = cs::ticks2time ( 1 );
	}

	state->m_last_clientside_anim_framecount = 0;
	state->m_feet_yaw_rate = 0.0f;

	std::array<animlayer_t, 13> backup_animlayers;
	
	if (!update_anim_layers )
		memcpy ( backup_animlayers.data(), ent->layers ( ), sizeof( backup_animlayers ) );

	if ( ent == g::local ) {
		ent->layers ( ) [ 3 ].m_weight = 0.0f;
		ent->layers ( ) [ 3 ].m_cycle = 0.0f;
		ent->layers ( ) [ 12 ].m_weight = 0.0f;
	}

	ent->animate ( ) = true;
	state->update ( angles );
	ent->animate ( ) = false;

	if ( ent == g::local ) {
		ent->layers ( ) [ 3 ].m_weight = 0.0f;
		ent->layers ( ) [ 3 ].m_cycle = 0.0f;
		ent->layers ( ) [ 12 ].m_weight = 0.0f;
	}

	if ( !update_anim_layers )
		memcpy ( ent->layers ( ), backup_animlayers.data ( ), sizeof ( backup_animlayers ) );

	cs::i::globals->m_frametime = backup_frametime;
	cs::i::globals->m_curtime = backup_curtime;

	if ( g::local && ent != g::local ) {
		auto yaw_offset = resolver::resolve_yaw ( ent, resolve );

		if ( yaw_offset == std::numeric_limits<float>::max ( ) ) {
			std::array<float, 24> calc_poses {};

			anim_util::calc_poses ( ent, calc_poses, state->m_abs_yaw );
			
			const auto idx = ent->idx ( );

			if ( !update_anim_layers )
				anim_util::build_bones ( ent, bones_out->data ( ), 0x7FF00, vec3_t ( 0.0f, state->m_abs_yaw, 0.0f ), ent->origin ( ), ent->simtime ( ), calc_poses );

			ent->poses ( ) = calc_poses;
			ent->set_abs_angles ( vec3_t ( 0.0f, state->m_abs_yaw, 0.0f ) );
		}
		else {
			const auto resolved_angle = cs::normalize ( cs::normalize( state->m_eye_yaw ) + yaw_offset );

			std::array<float, 24> resolved_poses {};

			anim_util::calc_poses ( ent, resolved_poses, resolved_angle );

			const auto idx = ent->idx ( );

			if ( !update_anim_layers )
				anim_util::build_bones ( ent, bones_out->data(), 0x7FF00, vec3_t ( 0.0f, resolved_angle, 0.0f ), ent->origin ( ), ent->simtime ( ), resolved_poses );

			ent->poses ( ) = resolved_poses;
			ent->set_abs_angles ( vec3_t ( 0.0f, resolved_angle, 0.0f ) );
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
		memcpy ( ent->layers ( ), last_anim_layers.data(), sizeof( last_anim_layers ) );

		ent->set_abs_angles ( vec3_t ( 0.0f, last_local_feet_yaw, 0.0f ) );

		if ( !cs::i::client_state->choked ( ) )
			manage_fake ( );
	}
	VM_TIGER_BLACK_END
}

void anims::apply_anims ( player_t* ent ) {
	if ( ent == g::local && g::local ) {
		ent->poses ( ) = last_local_poses;
		ent->set_abs_angles ( vec3_t( 0.0f, last_local_feet_yaw, 0.0f ) );
		memcpy ( ent->layers ( ), last_anim_layers.data(),sizeof( last_anim_layers ) );
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

	if ( g::local && ent == g::local ) {
		ent->layers ( ) [ 4 ].m_cycle = anim_util::local_poses_airtime;
		ent->layers ( ) [ 4 ].m_playback_rate = ( ( anim_util::local_poses_airtime > 0.0f ) ? cs::ticks2time ( 1 ) : 0.0f );
		ent->layers ( ) [ 4 ].m_weight = ( ( anim_util::local_poses_airtime > 0.0f ) ? 1.0f : 0.0f );

		memcpy ( last_anim_layers_queued.data ( ), ent->layers ( ), sizeof ( last_anim_layers_queued ) );

		return;
	}

	/* temp */
	const auto max_processing_ticks = g::cvars::sv_maxusrcmdprocessticks->get_int ( );

	if ( ent->simtime ( ) > ent->old_simtime ( ) ) {
		process_networked_anims ( ent );
		has_been_predicted[idx] = false;

		if ( anim_info [ idx ].front ( ).m_shot )
			shot_count [ idx ]++;

		if ( g::local && ent->team ( ) != g::local->team ( ) )
			predicted_anim_info [ idx ].push_front ( anim_info [ idx ].front ( ) );
	}
	else if ( g::local && ent->team ( ) != g::local->team ( ) && !anim_info [ idx ].empty() /*&& !has_been_predicted [ idx ] && !anim_info [ idx ].front ( ).m_shot*/ ) {
		//for ( auto i = 0; i < max_processing_ticks; i++ )
		//	process_predicted_anims ( ent, true );
	
		process_predicted_anims ( ent, false );

		has_been_predicted [ idx ] = true;
	}
	
	const auto nci = cs::i::engine->get_net_channel_info ( );
	while ( nci && !anim_info [ idx ].empty ( ) && anim_info [ idx ].back ( ).m_simtime < int ( cs::i::globals->m_curtime - g::cvars::sv_maxunlag->get_float ( ) ) )
		anim_info [ idx ].pop_back ( );
}

void anims::on_render_start ( int idx ) {
	auto ent = cs::i::ent_list->get<player_t*> ( idx );

	if ( !ent || !ent->is_player ( ) || !ent->alive ( ) || ent->dormant ( ) || !ent->animstate ( ) )
		return reset_data ( idx );

	ent->animate ( ) = false;

	apply_anims ( ent );

	//static int tick_count = cs::i::globals->m_tickcount;
	//if ( tick_count != cs::i::globals->m_tickcount && g::local && g::local->alive() && g::local->animstate ( ))
	//	anim_util::build_bones ( g::local, real_matrix.data ( ), 0x7FF00 /* BONE_USED_BY_HITBOX */, vec3_t ( 0.0f, g::local->animstate ( )->m_abs_yaw, 0.0f ), g::local->origin ( ), cs::i::globals->m_curtime, g::local->poses() );
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
		server_time = cs::i::globals->m_curtime;

		for ( auto i = 1; i <= cs::i::globals->m_max_clients; i++ )
			on_net_update_end ( i );
	} break;
	}
}