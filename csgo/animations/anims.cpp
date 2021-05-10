#include "anims.hpp"
#include "resolver.hpp"

#include "../hooks/setup_bones.hpp"

#include "../features/ragebot.hpp"

#include "../menu/options.hpp"

#include "rebuilt.hpp"

#undef max
#undef min

std::array< animlayer_t , 13> last_anim_layers_server {};
std::array< animlayer_t , 13> last_anim_layers_queued {};
std::array< animlayer_t , 13> last_anim_layers {};
float last_local_feet_yaw;
std::array<float , 24> last_local_poses;

/* from valve sdk */
constexpr auto CSGO_ANIM_WALK_TO_RUN_TRANSITION_SPEED = 2.0f;
constexpr auto CSGO_ANIM_ONGROUND_FUZZY_APPROACH = 8.0f;
constexpr auto CSGO_ANIM_ONGROUND_FUZZY_APPROACH_CROUCH = 16.0f;
constexpr auto CSGO_ANIM_LADDER_CLIMB_COVERAGE = 100.0f;
constexpr auto CSGO_ANIM_RUN_ANIM_PLAYBACK_MULTIPLIER = 0.85f;
constexpr auto ANIM_TRANSITION_WALK_TO_RUN = 0;
constexpr auto ANIM_TRANSITION_RUN_TO_WALK = 1;
constexpr auto CS_PLAYER_SPEED_RUN = 260.0f;
constexpr auto CS_PLAYER_SPEED_WALK_MODIFIER = 0.52f;

bool anims::get_lagcomp_bones( player_t* ent , std::array<matrix3x4_t , 128>& out ) {
	const auto nci = cs::i::engine->get_net_channel_info( );

	auto idx = ent->idx( );

	if ( !idx || idx > cs::i::globals->m_max_clients )
		return false;

	auto& data = anim_info[ idx ];

	if ( data.empty( ) )
		return false;

	const auto lerp = lerp_time( );

	auto time_valid_no_deadtime = [ & ] ( float t ) {
		const auto correct = std::clamp( nci->get_avg_latency( 0 ) + nci->get_avg_latency( 1 ) + lerp , 0.0f , g::cvars::sv_maxunlag->get_float( ) );
		return abs( correct - ( cs::i::globals->m_curtime - t ) ) <= 0.2f;
	};

	for ( int i = static_cast< int >( data.size( ) ) - 1; i >= 0; i-- ) {
		auto& it = data[ i ];

		if ( time_valid_no_deadtime( it.m_simtime ) ) {
			if ( it.m_origin.dist_to( ent->origin( ) ) < 1.0f )
				return false;

			bool end = ( i - 1 ) <= 0;
			vec3_t next = end ? ent->origin( ) : data[ i - 1 ].m_origin;
			float time_next = end ? ent->simtime( ) : data[ i - 1 ].m_simtime;

			float correct = nci->get_avg_latency( 0 ) + nci->get_avg_latency( 1 ) + lerp;
			float time_delta = time_next - it.m_simtime;
			float add = end ? 0.2f : time_delta;
			float deadtime = it.m_simtime + correct + add;

			float delta = deadtime - cs::i::globals->m_curtime;

			float mul = 1.0f / add;

			vec3_t lerp = next + ( it.m_origin - next ) * std::clamp( delta * mul , 0.0f , 1.0f );

			out = it.m_aim_bones[it.m_side];

			for ( auto& iter : out )
				iter.set_origin( iter.origin( ) - it.m_origin + lerp );

			return true;
		}
	}

	return false;
}

// credits cbrs
bool anims::build_bones( player_t* target , matrix3x4_t* mat , int mask , vec3_t rotation , vec3_t origin , float time , std::array<float , 24>& poses ) {
	static auto invalidate_physics_recursive = pattern::search( _( "client.dll" ) , _( "55 8B EC 83 E4 F8 83 EC 0C 53 8B 5D 08 8B C3 56" ) ).get<void( __thiscall* )( player_t* , int )>( );

	/* vfunc indices */
	static auto standard_blending_rules_vfunc_idx = pattern::search( _( "client.dll" ) , _( "FF 90 ? ? ? ? 8B 47 FC" ) ).add( 2 ).deref( ).get< uint32_t >( ) / 4;
	static auto build_transformations_vfunc_idx = pattern::search( _( "client.dll" ) , _( "FF 90 ? ? ? ? A1 ? ? ? ? B9 ? ? ? ? 8B 40 34 FF D0 85 C0 74 41" ) ).add( 2 ).deref( ).get< uint32_t >( ) / 4;
	static auto update_ik_locks_vfunc_idx = pattern::search( _( "client.dll" ) , _( "FF 90 ? ? ? ? 8B 8F ? ? ? ? 8D 84 24 ? ? ? ? 50" ) ).add( 2 ).deref( ).get< uint32_t >( ) / 4;
	static auto calculate_ik_locks_vfunc_idx = pattern::search( _( "client.dll" ) , _( "FF 90 ? ? ? ? 8B 8F ? ? ? ? 8D 84 24 ? ? ? ? 50 FF B7 ? ? ? ? 8D 84 24 ? ? ? ? 50 8D 84 24 ? ? ? ? 50 E8 ? ? ? ? 8B 47 FC 8D 4F FC 8B 80" ) ).add( 2 ).deref( ).get< uint32_t >( ) / 4;

	/* func sigs */
	static auto init_iks = pattern::search( _( "client.dll" ) , _( "55 8B EC 83 EC 08 8B 45 08 56 57 8B F9 8D" ) ).get< void( __thiscall* )( void* , void* , vec3_t& , vec3_t& , float , int , int ) >( );
	static auto update_targets = pattern::search( _( "client.dll" ) , _( "55 8B EC 83 E4 F0 81 EC 18 01 00 00 33" ) ).get< void( __thiscall* )( void* , vec3_t* , void* , void* , uint8_t* ) >( );
	static auto solve_dependencies = pattern::search( _( "client.dll" ) , _( "55 8B EC 83 E4 F0 81 EC 98 05 00 00 8B 81" ) ).get< void( __thiscall* )( void* , vec3_t* , void* , void* , uint8_t* ) >( );

	/* offset sigs */
	static auto iks_off = pattern::search( _( "client.dll" ) , _( "8D 47 FC 8B 8F" ) ).add( 5 ).deref( ).add( 4 ).get< uint32_t >( );
	static auto effects_off = pattern::search( _( "client.dll" ) , _( "75 0D 8B 87" ) ).add( 4 ).deref( ).add( 4 ).get< uint32_t >( );

	const auto backup_poses = target->poses( );

	auto cstudio = *reinterpret_cast< void** >( uintptr_t( target ) + 0x294C );

	if ( !cstudio )
		return false;

	//if ( target != g::local ) {
	//	invalidate_physics_recursive( target , 2 );
	//	invalidate_physics_recursive( target , 8 );
	//	invalidate_physics_recursive( target , 32 );
	//}

	target->poses( ) = poses;

	//  we need an aligned matrix in the bone accessor, so do this :) bad performance cause memcpy but that's ok
	matrix3x4a_t used[ 128 ];

	//	output shit
	uint8_t bone_computed[ 0x100 ] = { 0 };

	//	needs to be aligned
	matrix3x4a_t base_matrix;

	cs::angle_matrix( rotation , origin , base_matrix );

	//	store shit
	const auto old_bones = target->bones( );

	auto iks = *reinterpret_cast< void** >( uintptr_t( target ) + iks_off );
	*reinterpret_cast< int* >( uintptr_t( target ) + effects_off ) |= 8;

	//	clear iks & re-create them
	if ( iks ) {
		if ( *( uint32_t* ) ( uintptr_t( iks ) + 0xFF0 ) > 0 ) {
			int v1 = 0;
			auto v62 = ( uint32_t* ) ( uintptr_t( iks ) + 0xD0 );

			do {
				*v62 = -9999;
				v62 += 0x55;
				++v1;
			} while ( v1 < *( uint32_t* ) ( uintptr_t( iks ) + 0xFF0 ) );
		}

		init_iks( iks , cstudio , rotation , origin , time , cs::i::globals->m_framecount , 0x1000 );
	}

	static vec3_t pos[ 128 ];
	memset( pos , 0 , sizeof( vec3_t[ 128 ] ) );

	void* q = malloc( /* sizeof quaternion_t */ 48 * 128 );
	memset( q , 0 , /* sizeof quaternion_t */ 48 * 128 );

	//	set flags and bones
	target->bones( ) = used;

	//	build some shit
	// IDX = 205 ( 2020.7.10 )
	vfunc< void( __thiscall* )( player_t* , void* , vec3_t* , void* , float , uint32_t ) >( target , standard_blending_rules_vfunc_idx ) ( target , cstudio , pos , q , time , mask );
	
	//	set iks
	if ( iks ) {
		vfunc< void( __thiscall* )( player_t* , float ) >( target , update_ik_locks_vfunc_idx ) ( target , time );
		update_targets( iks , pos , q , target->bones( ) , bone_computed );
		vfunc< void( __thiscall* )( player_t* , float ) >( target , calculate_ik_locks_vfunc_idx ) ( target , time );
		solve_dependencies( iks , pos , q , target->bones( ) , bone_computed );
	}

	//	build the matrix
	// IDX = 189 ( 2020.7.10 )
	vfunc< void( __thiscall* )( player_t* , void* , vec3_t* , void* , matrix3x4a_t const& , uint32_t , void* ) >( target , build_transformations_vfunc_idx ) ( target , cstudio , pos , q , base_matrix , mask , bone_computed );

	free( q );

	//	restore flags and bones
	*reinterpret_cast< int* >( uintptr_t( target ) + effects_off ) &= ~8;
	target->bones( ) = old_bones;

	//  and pop out our new matrix
	memcpy( mat , used , sizeof( matrix3x4_t ) * 128 );

	target->poses( ) = backup_poses;

	return true;
}

float anims::angle_diff( float dst , float src ) {
	auto delta = fmodf( dst - src , 360.0f );

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

void anims::calc_poses( player_t* ent , std::array<float , 24>& poses , float feet_yaw ) {
	/* copy existing poses*/
	poses = ent->poses( );

	const auto anim_state = ent->animstate( );
	const auto vel = ent == g::local ? ent->abs_vel( ) : ent->vel( );

	/* calculate *some* new ones */
	//poses[ pose_param_t::speed ] = vel.length_2d( ) / ( ( ( ent->weapon( ) && ent->weapon( )->data( ) ) ? std::max( ent->weapon( )->data( )->m_max_speed , 0.001f ) : CS_PLAYER_SPEED_RUN ) * CS_PLAYER_SPEED_WALK_MODIFIER );
	//poses[ pose_param_t::stand ] = 1.0f - ent->crouch_amount( );

	//poses[ pose_param_t::body_yaw ] = std::clamp( cs::normalize( angle_diff( cs::normalize( anim_state->m_eye_yaw ) , cs::normalize( feet_yaw ) ) ) , -60.0f , 60.0f ) / 120.0f + 0.5f;
	//poses[ pose_param_t::move_yaw ] = std::clamp( cs::normalize( angle_diff( cs::normalize( cs::rad2deg( atan2( -vel.y , -vel.x ) ) ) , cs::normalize( feet_yaw ) ) ) , -180.0f , 180.0f ) / 360.0f + 0.5f;
	//poses[ pose_param_t::lean_yaw ] = std::clamp( cs::normalize( angle_diff( cs::normalize( feet_yaw ), cs::normalize( cs::rad2deg( atan2( vel.y , vel.x ) ) ) ) ) , -180.0f , 180.0f ) / 360.0f + 0.5f;

	const float recalc_air_time = ( ent->layers( )[ 4 ].m_cycle - 0.72f ) * 1.25f;
	const float clamped = recalc_air_time >= 0.0f ? std::min( recalc_air_time , 1.0f ) : 0.0f;
	float jump_fall = ( 3.0f - ( clamped + clamped ) ) * ( clamped * clamped );

	if ( jump_fall >= 0.0f )
		jump_fall = std::min( jump_fall , 1.0f );

	poses[ pose_param_t::jump_fall ] = jump_fall;
	//poses[ pose_param_t::body_pitch ] = std::clamp( cs::normalize( anim_state->m_pitch ) , -90.0f , 90.0f ) / 180.0f + 0.5f;

	//poses[ pose_param_t::move_blend_crouch ] = ent->crouch_amount( );
	//poses[ pose_param_t::move_blend_run ] = anim_state->m_ground_fraction * ( 1.0f - ent->crouch_amount( ) );
	//poses[ pose_param_t::move_blend_walk ] = ( 1.0f - anim_state->m_ground_fraction ) * ( 1.0f - ent->crouch_amount( ) );
}

void anims::simulate_movement( player_t* ent , flags_t& flags , vec3_t& origin , vec3_t& vel , flags_t& old_flags ) {
	if ( !( flags & flags_t::on_ground ) )
		vel.z -= g::cvars::sv_gravity->get_float( ) * cs::ticks2time(1);
	else if ( !( old_flags & flags_t::on_ground ) )
		vel.z = g::cvars::sv_jump_impulse->get_float( );

	vec3_t start , end , normal;
	trace_t trace;
	trace_filter_t filter;

	filter.m_skip = ent;

	start = origin;
	end = start + vel * cs::ticks2time( 1 );

	ray_t ray;
	ray.init( start , end , ent->mins( ) , ent->maxs( ) );
	cs::i::trace->trace_ray( ray, mask_playersolid, &filter, &trace );

	if ( trace.m_fraction != 1.0f ) {
		for ( auto i = 0; i < 2; ++i ) {
			vel -= trace.m_plane.m_normal * vel.dot_product( trace.m_plane.m_normal );

			float adjust = vel.dot_product( trace.m_plane.m_normal );

			if ( adjust < 0.0f )
				vel -= ( trace.m_plane.m_normal * adjust );

			start = trace.m_endpos;
			end = start + ( vel * ( cs::ticks2time( 1 ) * ( 1.0f - trace.m_fraction ) ) );

			ray.init( start , end , ent->mins( ) , ent->maxs( ) );
			cs::i::trace->trace_ray( ray , mask_playersolid , &filter , &trace );

			if ( trace.m_fraction == 1.0f )
				break;
		}
	}

	start = end = origin = trace.m_endpos;
	end.z -= 2.0f;

	ray.init( start , end , ent->mins( ) , ent->maxs( ) );
	cs::i::trace->trace_ray( ray , mask_playersolid , &filter , &trace );

	old_flags = flags;

	flags &= ~flags_t::on_ground;

	if ( trace.m_fraction != 1.0f && trace.m_plane.m_normal.z > 0.7f )
		flags |= flags_t::on_ground;
}

void anims::reset_data ( int idx ) {
	if ( !idx || idx > anim_info.size() - 1 )
		return;

	if ( !anim_info [ idx ].empty ( ) )
		anim_info [ idx ].clear ( );

	auto ent = cs::i::ent_list->get<player_t*> ( idx );

	features::ragebot::get_misses ( idx ).bad_resolve = 0;

	if ( ent )
		ent->animate ( ) = true;

	if ( ent == g::local && g::local && ent->animstate( ) )
		last_local_animstate = *ent->animstate( );
}

void anims::copy_client_layers ( player_t* ent, std::array<animlayer_t, 13>& to, std::array<animlayer_t, 13>& from ) {
	const auto layers = ent->layers ( );

	for ( auto i = 0; i < to.size ( ); i++ ) {
		if ( i == 3 || i == 4 || i == 5 || i == 6 || i == 9 ) {
			to [ i ].m_sequence = from [ i ].m_sequence;
			to [ i ].m_previous_cycle = from [ i ].m_previous_cycle;
			to [ i ].m_weight = from [ i ].m_weight;
			to [ i ].m_weight_delta_rate = from [ i ].m_weight_delta_rate;
			to [ i ].m_playback_rate = from [ i ].m_playback_rate;
			to [ i ].m_cycle = from [ i ].m_cycle;
		}
	}
}

void anims::manage_fake ( ) {
	static auto spawn_time = 0.0f;

	if ( !g::local ) {
		spawn_time = 0.0f;
		return;
	}

	static animstate_t fake_anim_state { };
	static auto handle = g::local->handle ( );

	if ( !g::local->alive ( ) ) {
		spawn_time = 0.0f;
		return;
	}

	bool reset = g::local->spawn_time ( ) != spawn_time || g::local->handle ( ) != handle;

	if ( reset ) {
		memset( &fake_anim_state, 0, sizeof( fake_anim_state ) );
		g::local->create_animstate ( &fake_anim_state );

		handle = g::local->handle ( );
		spawn_time = g::local->spawn_time ( );
	}

	std::array<animlayer_t, 13> networked_layers;
	memcpy ( networked_layers.data(), g::local->layers ( ), sizeof( networked_layers ) );

	const auto backup_abs_angles = g::local->abs_angles ( );
	const auto backup_poses = g::local->poses ( );
	const auto origin = g::local->origin ( );

	rebuilt::update ( &fake_anim_state, g::angles, g::local->origin ( ) );

	g::local->abs_angles ( ).y = fake_anim_state.m_abs_yaw;

	memcpy ( g::local->layers ( ), last_anim_layers.data ( ), sizeof ( last_anim_layers ) );
	
	build_bones( g::local, fake_matrix.data ( ), 0x7FF00, g::local->abs_angles ( ), origin, cs::i::globals->m_curtime, g::local->poses() );

	memcpy ( g::local->layers ( ), networked_layers.data ( ), sizeof ( networked_layers ) );
	g::local->poses ( ) = backup_poses;
	g::local->abs_angles ( ).y = backup_abs_angles.y;

	for ( auto& iter : fake_matrix )
		iter.set_origin ( iter.origin ( ) - origin );
}

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

	if ( !!( ent->flags ( ) & flags_t::on_ground ) && anim_layers [ 11 ].m_weight > 0.0f && anim_layers [ 11 ].m_weight < 1.0f && anim_layers [ 11 ].m_cycle > previous_record.m_anim_layers [0][ 11 ].m_cycle ) {
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

void anims::update_from( player_t* ent , const anim_info_t& from , anim_info_t& to, std::array<animlayer_t, 13>& cur_layers ) {
	const auto delta_ticks = cs::time2ticks( to.m_simtime - from.m_simtime );

	if ( !delta_ticks )
		return;

	const auto backup_anim_layers = to.m_anim_layers;
	const auto backup_abs_origin = ent->abs_origin( );

	/* update from last sent data */
	ent->angles( ) = from.m_angles;
	ent->origin( ) = from.m_origin;
	ent->mins( ) = from.m_mins;
	ent->maxs( ) = from.m_maxs;
	ent->lby( ) = from.m_lby;
	ent->simtime( ) = from.m_simtime;
	ent->old_simtime( ) = from.m_old_simtime;
	ent->crouch_amount( ) = from.m_duck_amount;
	memcpy( ent->layers(), from.m_anim_layers[0].data(), sizeof( from.m_anim_layers[ 0 ] ) );
	ent->flags( ) = from.m_flags;
	ent->vel( ) = from.m_vel;
	ent->set_abs_origin( ent->origin( ) );
	
	/* start from last animation data */
	to.m_poses = from.m_poses;
	to.m_anim_layers = from.m_anim_layers;
	to.m_abs_angles = from.m_abs_angles;
	to.m_anim_state = from.m_anim_state;

	auto last_flags = from.m_flags;
	const auto delta_yaw = cs::normalize( cs::normalize( to.m_angles.y ) - cs::normalize( from.m_angles.y ) );

	for ( auto i = 0; i < delta_ticks; i++ ) {
		const float simtime = from.m_simtime + cs::ticks2time( i + 1 );
		auto should_desync = false;

		/* interpolate angles */
		if ( !to.m_shot ) {
			ent->angles( ) = ( i + 1 == delta_ticks ) ? to.m_angles /*real*/ : from.m_angles /*fake*/;

			if ( !( i + 1 == delta_ticks ) )
				should_desync = true;
		}
		/* set onshot angles when enemy supposedly shot */
		else {
			ent->angles( ) = from.m_angles;

			if ( ent->weapon( ) && ent->weapon( )->last_shot_time() <= simtime )
				ent->angles( ) = to.m_angles;
			else
				should_desync = true;
		}

		/* interpolate crouch amount */
		ent->crouch_amount( ) = std::lerp( from.m_duck_amount , to.m_duck_amount , static_cast< float >( i + 1 ) / static_cast< float >( delta_ticks ) );

		/* interpolate velocity */
		ent->vel( ).x = std::lerp( from.m_vel.x , to.m_vel.x , static_cast< float >( i + 1 ) / static_cast< float >( delta_ticks ) );
		ent->vel( ).y = std::lerp( from.m_vel.y , to.m_vel.y , static_cast< float >( i + 1 ) / static_cast< float >( delta_ticks ) );
		ent->vel( ).z = std::lerp( from.m_vel.z , to.m_vel.z , static_cast< float >( i + 1 ) / static_cast< float >( delta_ticks ) );

		/* use avg velocity (based on position, not instantaneous) to simulate movement */
		auto anim_velocity = ( to.m_origin - from.m_origin ) / ( to.m_simtime - from.m_simtime );

		/* predict origin */
		simulate_movement( ent , ent->flags( ) , ent->origin( ) , anim_velocity, last_flags );

		ent->set_abs_origin( ent->origin( ) );

		/* set simtime */
		const float backup_simtime = ent->simtime( );
		const float backup_old_simtime = ent->old_simtime( );

		ent->simtime( ) = simtime;
		ent->old_simtime( ) = simtime - cs::ticks2time( 1 );

		update_all_anims( ent, ent->angles( ), to, cur_layers, should_desync, i + 1 == delta_ticks /* only build bones for latest tick */ );

		ent->simtime( ) = backup_simtime;
		ent->old_simtime( ) = backup_old_simtime;
	}

	/* restore data to current */
	ent->angles( ) = to.m_angles;
	ent->origin( ) = to.m_origin;
	ent->mins( ) = to.m_mins;
	ent->maxs( ) = to.m_maxs;
	ent->lby( ) = to.m_lby;
	ent->simtime( ) = to.m_simtime;
	ent->old_simtime( ) = to.m_old_simtime;
	memcpy( ent->layers( ) , backup_anim_layers[0].data( ) , sizeof( backup_anim_layers[ 0 ] ) );
	ent->crouch_amount( ) = to.m_duck_amount;
	ent->flags( ) = to.m_flags;
	ent->vel( ) = to.m_vel;
	ent->set_abs_origin( backup_abs_origin );

	/* try to guess the orientation of their real matrix */
	resolver::resolve_desync( ent , to );

	/* set animlayers to ones from the server after we are done using the predicted ones */
	to.m_anim_layers = backup_anim_layers;

	/* set ideal animation data to be displayed */
	ent->poses( ) = to.m_poses[ to.m_side ];
	*ent->animstate( ) = to.m_anim_state[ to.m_side ];
	ent->set_abs_angles( to.m_abs_angles[ to.m_side ] );

	const auto backup_lean = ent->layers ( ) [ 12 ].m_weight;
	ent->layers ( ) [ 12 ].m_weight = 0.0f;
	build_bones ( ent, to.m_aim_bones [ to.m_side ].data ( ), 0x7FF00, vec3_t ( 0.0f, to.m_anim_state [ to.m_side ].m_abs_yaw, 0.0f ), to.m_origin, to.m_simtime, to.m_poses [ to.m_side ] );
	ent->layers ( ) [ 12 ].m_weight = backup_lean;

	for ( auto& bones : to.m_aim_bones )
		bones = to.m_aim_bones [ to.m_side ];
}

void anims::update_all_anims ( player_t* ent, vec3_t& angles, anim_info_t& to, std::array<animlayer_t, 13>& cur_layers, bool should_desync, bool build_matrix ) {
	static auto& resolver = options::vars[ _( "ragebot.resolve_desync" ) ].val.b;

	const auto anim_state = ent->animstate( );
	const auto anim_layers = ent->layers( );

	auto get_should_resolve = [ & ] ( ) {
		if ( !resolver || !g::local || !g::local->alive( ) || !anim_state || !anim_layers || ent->team( ) == g::local->team( ) )
			return false;

		player_info_t pl_info;
		cs::i::engine->get_player_info( ent->idx( ) , &pl_info );

		if ( pl_info.m_fake_player )
			return false;

		return true;
	};

	auto should_resolve = get_should_resolve( );

	auto update_desync_side = [ & ] ( desync_side_t side ) {
		/* update animations for these set of animlayers and animstate */
		*anim_state = to.m_anim_state[ side ];
		memcpy( anim_layers , to.m_anim_layers[ side ].data( ) , sizeof ( to.m_anim_layers [ side ] ) );
		ent->poses( ) = to.m_poses [ side ];

		const auto offset = -60.0f + static_cast<float>( side ) * 30.0f;
		//const auto backup_angles = angles;

		if ( should_resolve && side != desync_side_t::desync_max )
			//angles.y = cs::normalize( angles.y + offset );
			anim_state->m_abs_yaw = cs::normalize ( anim_state->m_eye_yaw + offset );

		/* update animations */
		update_anims( ent , angles );

		//angles = backup_angles;

		/* calculate new poses */
		/* NOT NEEDED */
		calc_poses( ent , to.m_poses[ side ] , anim_state->m_abs_yaw );

		//if ( build_matrix && side != desync_side_t::desync_max /*don't build bones for unresolved matrix, we only want its animstate */ )
		//	build_bones( ent, to.m_aim_bones[side].data(), 0x7FF00, vec3_t( 0.0f , anim_state->m_abs_yaw, 0.0f ) , ent->origin(), ent->simtime(), to.m_poses[ side ] );
		
		/* store new anim data */
		to.m_anim_state[ side ] = *anim_state;
		memcpy( to.m_anim_layers[ side ].data( ) , anim_layers , sizeof( to.m_anim_layers [ side ] ) );
		to.m_abs_angles[ side ] = vec3_t( 0.0f , anim_state->m_abs_yaw, 0.0f );
		//to.m_poses[ side ] = ent->poses( );
	};

	/* backup animation data */
	std::array<animlayer_t , 13> backup_anim_layers {};
	memcpy( backup_anim_layers.data( ) , anim_layers , sizeof( backup_anim_layers ) );
	const auto backup_anim_state = *anim_state;
	const auto backup_poses = ent->poses();

	/* animate with different desync ranges */
	if ( should_resolve ) {
		update_desync_side( desync_left_max );
		update_desync_side( desync_left_half );
		update_desync_side( desync_middle );
		update_desync_side( desync_right_half );
		update_desync_side( desync_right_max );

		/* used for reference to fake */
		update_desync_side( desync_max );
	}
	else {
		update_desync_side( desync_middle );

		/* copy single updated data to all others */
		for ( auto& anim_layer : to.m_anim_layers )
			anim_layer = to.m_anim_layers[ desync_middle ];

		for ( auto& pose : to.m_poses )
			pose = to.m_poses[ desync_middle ];

		for ( auto& abs_angle : to.m_abs_angles )
			abs_angle = to.m_abs_angles[ desync_middle ];

		for ( auto& anim_state : to.m_anim_state )
			anim_state = to.m_anim_state[ desync_middle ];

		for ( auto& bones : to.m_aim_bones )
			bones = to.m_aim_bones[ desync_middle ];
	}

	/* restore animation data */
	memcpy( anim_layers , backup_anim_layers.data( ) , sizeof( backup_anim_layers ) );
	*anim_state = backup_anim_state;
	ent->poses( ) = backup_poses;
}

void anims::update_anims ( player_t* ent, vec3_t& angles, bool force_feet_yaw ) {
	static auto& invalidate_bone_cache = *pattern::search( _( "client.dll" ) , _( "C6 05 ? ? ? ? ? 89 47 70" ) ).add( 2 ).deref( ).get<bool*>( );
	static auto invalidate_physics_recursive = pattern::search( _( "client.dll" ) , _( "55 8B EC 83 E4 F8 83 EC 0C 53 8B 5D 08 8B C3 56" ) ).get<void( __thiscall* )( player_t* , int )>( );
	static auto init_last_data = false;

	VM_TIGER_BLACK_START
		const auto state = ent->animstate( );
	const auto anim_layers = ent->layers( );

	if ( !state || !anim_layers )
		return;

	if ( !init_last_data ) {
		last_local_feet_yaw = state->m_abs_yaw;
		last_local_poses = ent->poses ( );
		memcpy ( last_anim_layers.data(), ent->layers ( ),sizeof( last_anim_layers ) );
		init_last_data = true;
	}

	const float backup_frametime = cs::i::globals->m_frametime;
	const float backup_curtime = cs::i::globals->m_curtime;
	
	if ( g::local && ent != g::local ) {
		cs::i::globals->m_frametime = cs::ticks2time ( 1 );
		cs::i::globals->m_curtime = ent->simtime ( );

		state->m_last_clientside_anim_update = ent->old_simtime( );

		*reinterpret_cast< uint32_t* >( reinterpret_cast< uintptr_t > ( ent ) + 0xE8 ) &= ~0x1000;
		*reinterpret_cast< vec3_t* >( reinterpret_cast< uintptr_t > ( ent ) + 0x94 ) = ent->vel( );
	}

	if ( ent == g::local && g::local ) {
		//*state = last_local_animstate;
		auto new_layers = last_anim_layers_server;
		copy_client_layers ( ent, new_layers, last_anim_layers_queued );
		last_anim_layers_queued = new_layers;

		memcpy( ent->layers( ), last_anim_layers_queued.data( ) , sizeof( last_anim_layers_queued ) );

		cs::i::globals->m_frametime = cs::ticks2time( 1 );
		state->m_last_clientside_anim_update = cs::i::globals->m_curtime - cs::ticks2time( 1 );
		state->m_feet_yaw_rate = 0.0f;
	}
	
	const auto backup_invalidate_bone_cache = invalidate_bone_cache;

	ent->animate ( ) = true;
	rebuilt::update( state , angles , ent->origin( ), force_feet_yaw );
	ent->animate ( ) = false;

	invalidate_bone_cache = backup_invalidate_bone_cache;

	if ( ent == g::local && g::local ) {
		memcpy( last_anim_layers_queued.data( ), ent->layers( ) , sizeof( last_anim_layers_queued ) );
		//last_local_animstate = *state;
	}

	cs::i::globals->m_frametime = backup_frametime;
	cs::i::globals->m_curtime = backup_curtime;

	if ( g::local && ent == g::local ) {
		/* dont update on local until we send packet */
		if ( g::send_packet ) {
			/* set new animlayers (server data + ones maintained on client) */
			memcpy( last_anim_layers.data( ) , last_anim_layers_queued.data( ) , sizeof( last_anim_layers_queued ) );

			//last_anim_layers[ 3 ].m_weight = 0.0f;
			//last_anim_layers[ 3 ].m_cycle = 0.0f;
			last_anim_layers[ 12 ].m_weight = 0.0f;

			/* store new data */
			memcpy( ent->layers( ) , last_anim_layers.data( ) , sizeof( last_anim_layers ) );

			last_local_feet_yaw = state->m_abs_yaw;
			last_local_poses = ent->poses( );
		}

		/* apply last sent data */
		ent->poses( ) = last_local_poses;
		memcpy( ent->layers( ) , last_anim_layers.data( ) , sizeof( last_anim_layers ) );

		ent->set_abs_angles( vec3_t( 0.0f , last_local_feet_yaw , 0.0f ) );

		build_bones ( ent, real_matrix.data ( ), 0x7FF00, ent->abs_angles(), ent->origin ( ), cs::i::globals->m_curtime, ent->poses ( ) );

		if ( g::send_packet ) {
			usable_bones [ ent->idx ( ) ] = real_matrix;
			usable_origin [ ent->idx ( ) ] = ent->origin ( );

			manage_fake ( );
		}
	}

	VM_TIGER_BLACK_END
}

void anims::apply_anims ( player_t* ent ) {
	if ( !g::local )
		return;

	if ( ent == g::local ) {
		ent->poses ( ) = last_local_poses;
		ent->set_abs_angles ( vec3_t( 0.0f, last_local_feet_yaw, 0.0f ) );
		memcpy ( ent->layers ( ), last_anim_layers.data(),sizeof( last_anim_layers ) );
		return;
	}

	auto& info = anim_info[ ent->idx( ) ];

	if ( info.empty( ) )
		return;

	auto& first = info.front( );

	usable_bones [ ent->idx ( ) ] = first.m_aim_bones [ first.m_side ];
	usable_origin [ ent->idx ( ) ] = first.m_origin;

	ent->set_abs_angles( first.m_abs_angles[ first.m_side ] );
	ent->poses( ) = first.m_poses[ first.m_side ];
}

namespace lby {
	extern bool in_update;
}

void anims::on_net_update_end ( int idx ) {
	auto ent = cs::i::ent_list->get<player_t*> ( idx );
	
	if ( !ent || !ent->is_player ( ) || !ent->alive ( ) || ent->dormant ( ) || !ent->animstate( ) || !ent->layers( ) )
		return reset_data ( idx );

	//ent->animate ( ) = false;

	if ( g::local && ent == g::local ) {
		memcpy ( last_anim_layers_server.data ( ), ent->layers ( ), sizeof ( last_anim_layers_server ) );
		return;
	}

	if ( ent->simtime ( ) > ent->old_simtime ( ) ) {
		if ( !anim_info[ idx ].empty( ) && ent->origin( ).dist_to_sqr( anim_info[ idx ].front( ).m_origin ) > 4096.0f )
			for ( auto& rec : anim_info[ idx ] )
				rec.m_invalid = true;

		if ( !!( ent->flags ( ) & flags_t::on_ground ) && ent->vel ( ).length_2d ( ) > 0.0f && !ent->layers()[ 6 ].m_weight )
			ent->vel ( ).zero ( );

		//fix_velocity( ent, ent->vel() );

		static anim_info_t rec {};

		/* if we don't have any information from the player, start animating from what we have now */
		if ( anim_info[ idx ].empty( ) ) {
			rec = anim_info_t( ent );

			if ( rec.m_shot )
				shot_count[ idx ]++;

			const auto backup_anim_layers = rec.m_anim_layers;

			update_all_anims( ent, ent->angles(), rec, rec.m_anim_layers[desync_side_t::desync_middle], true, true );

			/* try to guess the orientation of their real matrix */
			resolver::resolve_desync( ent , rec );

			/* set animlayers to ones from the server after we are done using the predicted ones */
			rec.m_anim_layers = backup_anim_layers;

			anim_info[ idx ].push_front( rec );

			ent->poses( ) = rec.m_poses[ rec.m_side ];
			*ent->animstate( ) = rec.m_anim_state[ rec.m_side ];
			ent->set_abs_angles( rec.m_abs_angles[ rec.m_side ] );
		}
		else {
			rec = anim_info_t( ent );

			if ( rec.m_shot )
				shot_count[ idx ]++;

			update_from( ent, anim_info[ idx ].front(), rec, rec.m_anim_layers [ desync_side_t::desync_middle ] );

			anim_info[ idx ].push_front( rec );
		}
	}
	
	while ( !anim_info [ idx ].empty ( ) && ( anim_info[ idx ].size() >= 24 || abs( anim_info[ idx ].back( ).m_simtime - cs::i::globals->m_curtime ) > 1.0f ) )
		anim_info [ idx ].pop_back ( );
}

void anims::on_render_start ( int idx ) {
	auto ent = cs::i::ent_list->get<player_t*> ( idx );

	if ( !ent || !ent->is_player ( ) || !ent->alive ( ) || ent->dormant ( ) || !ent->animstate ( ) )
		return reset_data ( idx );

	//ent->animate ( ) = false;

	apply_anims ( ent );
}

void anims::pre_fsn ( int stage ) {
	if ( g::round == round_t::starting ) {
		for ( auto i = 1; i <= cs::i::globals->m_max_clients; i++ )
			features::ragebot::get_misses ( i ).bad_resolve = 0;
	}

	if ( !g::local ) {
		g::round = round_t::in_progress;

		anims::manage_fake ( );

		for ( auto i = 1; i <= cs::i::globals->m_max_clients; i++ )
			reset_data ( i );

		return;
	}
	else if ( !g::local->alive ( ) ) {
		g::round = round_t::in_progress;

		anims::manage_fake ( );

		memcpy ( last_anim_layers_server.data ( ), g::local->layers ( ), sizeof ( last_anim_layers_server ) );
		memcpy ( last_anim_layers_queued.data ( ), g::local->layers ( ), sizeof ( last_anim_layers_queued ) );
		memcpy ( last_anim_layers.data ( ), g::local->layers ( ), sizeof ( last_anim_layers ) );
	}

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
	if ( !g::local ) {
		g::round = round_t::in_progress;

		anims::manage_fake( );

		for ( auto i = 1; i <= cs::i::globals->m_max_clients; i++ )
			reset_data( i );

		return;
	}
	else if ( !g::local->alive ( ) ) {
		g::round = round_t::in_progress;

		anims::manage_fake ( );

		memcpy ( last_anim_layers_server.data ( ), g::local->layers ( ), sizeof ( last_anim_layers_server ) );
		memcpy ( last_anim_layers_queued.data ( ), g::local->layers ( ), sizeof ( last_anim_layers_queued ) );
		memcpy ( last_anim_layers.data ( ), g::local->layers ( ), sizeof ( last_anim_layers ) );
	}

	using update_all_viewmodel_addons_t = int( __fastcall* )( void* );
	static auto update_all_viewmodel_addons = pattern::search( _( "client.dll" ) , _( "55 8B EC 83 E4 ? 83 EC ? 53 8B D9 56 57 8B 03 FF 90 ? ? ? ? 8B F8 89 7C 24 ? 85 FF 0F 84 ? ? ? ? 8B 17 8B CF" ) ).get< update_all_viewmodel_addons_t >( );

	switch ( stage ) {
	case 4: {
		for ( auto i = 1; i <= cs::i::globals->m_max_clients; i++ )
			on_net_update_end( i );
	} break;
	case 5: {
		if ( g::local && g::local->alive() ) {
			/* update viewmodel manually to fix it dissappearing */
			const auto viewmodel = g::local->viewmodel_handle( );

			if ( viewmodel != -1 && cs::i::ent_list->get_by_handle< void* >( viewmodel ) )
				update_all_viewmodel_addons( cs::i::ent_list->get_by_handle< void* >( viewmodel ) );
		}
	} break;
	}
}