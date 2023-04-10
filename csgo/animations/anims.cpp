#include "anims.hpp"
#include "resolver.hpp"

#include "../hooks/setup_bones.hpp"

#include "../features/ragebot.hpp"

#include "../menu/options.hpp"

#include "rebuilt.hpp"

#include "../features/antiaim.hpp"
#include "../features/exploits.hpp"
#include "../features/autowall_skeet.hpp"

#undef max
#undef min

bool anims::anim_info_t::valid ( ) {
	const auto nci = cs::i::engine->get_net_channel_info ( );

	if ( !nci || !g::local || m_invalid )
		return false;

	const auto lerp = lerp_time ( );
	const auto correct = std::clamp ( nci->get_latency ( 0 ) + nci->get_latency ( 1 ) + lerp, 0.0f, g::cvars::sv_maxunlag->get_float ( ) );

	auto server_time = g::local->alive() ? cs::ticks2time( g::local->tick_base ( )) : cs::i::globals->m_curtime;
	auto tickbase_as_int = std::clamp<int> ( static_cast< int >( features::ragebot::active_config.max_dt_ticks ), 0, g::cvars::sv_maxusrcmdprocessticks->get_int ( ) - 1 );

	if ( !features::ragebot::active_config.dt_enabled || !utils::keybind_active ( features::ragebot::active_config.dt_key, features::ragebot::active_config.dt_key_mode ) )
		tickbase_as_int = 0;

	//if ( exploits::is_ready ( ) && tickbase_as_int > 0 )
	//	server_time -= cs::ticks2time ( std::max ( tickbase_as_int, 0 ) );

	if ( abs ( correct - ( server_time - m_simtime ) ) > 0.2f )
		return false;

	return true;
}

anims::anim_info_t::anim_info_t ( player_t* ent ) {
	const auto old_simtime = !anims::anim_info [ ent->idx ( ) ].empty ( ) ? anims::anim_info [ ent->idx ( ) ].front ( ).m_simtime : ent->old_simtime ( );

	m_invalid = false;
	m_forward_track = false;
	m_resolved = false;
	m_has_vel = false;
	m_shifted = false;
	m_shot = ent->weapon ( ) && ent->weapon ( )->last_shot_time ( ) > old_simtime && ent->weapon ( )->last_shot_time ( ) <= ent->simtime ( );
	m_angles = ent->angles ( );
	m_origin = ent->origin ( );
	m_mins = ent->mins ( );
	m_maxs = ent->maxs ( );
	m_lby = ent->lby ( );
	m_simtime = ent->simtime ( );
	m_old_simtime = old_simtime;
	m_duck_amount = ent->crouch_amount ( );
	m_choked_commands = std::clamp ( cs::time2ticks ( ent->simtime ( ) - old_simtime ), 0, g::cvars::sv_maxusrcmdprocessticks->get_int ( ) );
	m_flags = ent->flags ( );
	m_vel = ent->vel ( );

	memcpy ( m_anim_layers.data ( ), ent->layers ( ), sizeof ( m_anim_layers ) );
	m_poses= ent->poses ( );
	m_abs_angles= ent->abs_angles ( );
	m_anim_state = *ent->animstate ( );

	*reinterpret_cast< float* >( reinterpret_cast< uintptr_t >( &m_anim_state ) + 0x9C ) = ent->layers ( ) [ 6 ].m_weight;
	*reinterpret_cast< float* >( reinterpret_cast< uintptr_t >( &m_anim_state ) + 0x98 ) = ent->layers ( ) [ 6 ].m_cycle;

	*reinterpret_cast< float* >( reinterpret_cast< uintptr_t >( &m_anim_state ) + 0x19C ) = ent->layers ( ) [ 7 ].m_sequence;
	*reinterpret_cast< float* >( reinterpret_cast< uintptr_t >( &m_anim_state ) + 0x190 ) = ent->layers ( ) [ 7 ].m_weight;
	*reinterpret_cast< float* >( reinterpret_cast< uintptr_t >( &m_anim_state ) + 0x198 ) = ent->layers ( ) [ 7 ].m_cycle;

	*reinterpret_cast< float* >( reinterpret_cast< uintptr_t >( &m_anim_state ) + 0x180 ) = ent->layers ( ) [ 12 ].m_weight;

	memcpy ( m_aim_bones.data ( ), ent->bone_cache ( ), ent->bone_count ( ) * sizeof ( matrix3x4_t ) );
	
	int ticks_delta = cs::time2ticks ( m_simtime ) - cs::time2ticks ( m_old_simtime );
	int simticks_delta = cs::time2ticks ( ent->simtime ( ) ) - cs::time2ticks ( ent->old_simtime ( ) );
	bool shifted_tickbase = ent->old_simtime ( ) <= ent->simtime ( );

	if ( shifted_tickbase && ( simticks_delta - ticks_delta ) > 2 ) {
		m_choked_commands = ticks_delta;
		m_shifted = true;
		//record->player_time = TICKS_TO_TIME ( ticks_delta );
	}
}

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
			if ( it.m_origin.dist_to( ent->origin ( ) ) < 1.0f )
				return false;

			bool end = ( i - 1 ) <= 0;
			vec3_t next = end ? ent->origin ( ) : data[ i - 1 ].m_origin;
			float time_next = end ? ent->simtime( ) : data[ i - 1 ].m_simtime;

			float correct = nci->get_avg_latency( 0 ) + nci->get_avg_latency( 1 ) + lerp;
			float time_delta = time_next - it.m_simtime;
			float add = end ? 0.2f : time_delta;
			float deadtime = it.m_simtime + correct + add;

			float delta = deadtime - cs::i::globals->m_curtime;

			float mul = 1.0f / add;

			vec3_t lerp = next + ( it.m_origin - next ) * std::clamp( delta * mul , 0.0f , 1.0f );

			out = it.m_aim_bones;

			for ( auto& iter : out )
				iter.set_origin( iter.origin ( ) - it.m_origin + lerp );

			return true;
		}
	}

	return false;
}

// credits cbrs
bool anims::build_bones( player_t* target , matrix3x4_t* mat , int mask , vec3_t rotation , vec3_t origin , float time , std::array<float , 24>& poses ) {
	VMP_BEGINMUTATION ( );
	static auto invalidate_physics_recursive = pattern::search( _( "client.dll" ) , _( "55 8B EC 83 E4 F8 83 EC 0C 53 8B 5D 08 8B C3 56" ) ).get<void( __thiscall* )( player_t* , int )>( );

	/* vfunc indices */
	static auto standard_blending_rules_vfunc_idx = pattern::search( _( "client.dll" ) , _( "FF 90 ? ? ? ? 8B 47 FC" ) ).add( 2 ).deref( ).get< uint32_t >( ) / 4;
	static auto build_transformations_vfunc_idx = pattern::search( _( "client.dll" ) , _( "FF 90 ? ? ? ? A1 ? ? ? ? B9 ? ? ? ? 8B 40 34 FF D0 85 C0 74 41" ) ).add( 2 ).deref( ).get< uint32_t >( ) / 4;
	static auto update_ik_locks_vfunc_idx = pattern::search( _( "client.dll" ) , _( "FF 90 ? ? ? ? 8B 8F ? ? ? ? 8D 84 24 ? ? ? ? 50" ) ).add( 2 ).deref( ).get< uint32_t >( ) / 4;
	static auto calculate_ik_locks_vfunc_idx = pattern::search( _( "client.dll" ) , _( "FF 90 ? ? ? ? 8B 8F ? ? ? ? 8D 84 24 ? ? ? ? 50 FF B7 ? ? ? ? 8D 84 24 ? ? ? ? 50 8D 84 24 ? ? ? ? 50 E8 ? ? ? ? 8B 47 FC 8D 4F FC 8B 80" ) ).add( 2 ).deref( ).get< uint32_t >( ) / 4;

	/* func sigs */
	static auto init_iks = pattern::search( _( "client.dll" ) , _( "55 8B EC 83 EC 08 8B 45 08 56 57 8B F9 8D" ) ).get< void( __thiscall* )( void* , void* , vec3_t& , vec3_t& , float , int , int ) >( );
	static auto update_targets = pattern::search( _( "client.dll" ) , _( "55 8B EC 83 E4 F0 81 EC 18 01 00 00 33" ) ).get< void( __thiscall* )( void* , vec3_t* , void* , void* , uint8_t* ) >( );
	static auto solve_dependencies = pattern::search( _( "client.dll" ) , _( "E8 ? ? ? ? 8B 47 FC 8D 4F FC 8B 80" ) ).resolve_rip().get< void( __thiscall* )( void* , vec3_t* , void* , void* , uint8_t* ) >( );

	/* offset sigs */
	static auto iks_off = pattern::search( _( "client.dll" ) , _( "8D 47 FC 8B 8F" ) ).add( 5 ).deref( ).add( 4 ).get< uint32_t >( );
	static auto effects_off = pattern::search( _( "client.dll" ) , _( "75 0D 8B 87" ) ).add( 4 ).deref( ).add( 4 ).get< uint32_t >( );

	const auto backup_poses = target->poses( );

	auto cstudio = *reinterpret_cast< void** >( uintptr_t( target ) + 0x293C );

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

	auto pos = new vec3_t [ 128 ];
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

	delete [ ] pos;
	free( q );

	//	restore flags and bones
	*reinterpret_cast< int* >( uintptr_t( target ) + effects_off ) &= ~8;
	target->bones( ) = old_bones;

	//  and pop out our new matrix
	memcpy( mat , used , sizeof( matrix3x4_t ) * 128 );

	target->poses( ) = backup_poses;

	return true;
}

void anims::reset_data ( int idx ) {
	if ( !idx || idx > anim_info.size ( ) - 1 )
		return;

	if ( !anim_info [ idx ].empty ( ) )
		anim_info [ idx ].clear ( );

	if ( !lagcomp_track [ idx ].empty ( ) )
		lagcomp_track [ idx ].clear ( );

	auto ent = cs::i::ent_list->get<player_t*> ( idx );

	if ( ent ) {
		auto animstate = ent->animstate ( );

		if ( animstate ) {
			animstate->m_on_ground = !!( ent->flags ( ) & flags_t::on_ground );

			if ( animstate->m_on_ground )
				animstate->m_hit_ground = false;

			animstate->m_time_in_air = 0.0f;
			animstate->m_abs_yaw = animstate->m_feet_yaw = cs::normalize ( animstate->m_eye_yaw );
		}
	}

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

	memcpy ( g::local->layers ( ), last_anim_layers.data ( ), sizeof ( last_anim_layers ) );
	
	build_bones( g::local, fake_matrix.data ( ), 0x7FF00, g::local->abs_angles ( ), origin, cs::i::globals->m_curtime, g::local->poses() );

	memcpy ( g::local->layers ( ), networked_layers.data ( ), sizeof ( networked_layers ) );
	g::local->poses ( ) = backup_poses;

	for ( auto& iter : fake_matrix )
		iter.set_origin ( iter.origin ( ) - origin );
}

bool anims::fix_velocity ( player_t* ent, vec3_t& vel, const std::array<animlayer_t, 13>& animlayers, const vec3_t& origin ) {
	VMP_BEGINMUTATION ( );
	const auto idx = ent->idx ( );
	const auto& records = anim_info [ idx ];

	vel.zero ( );

	if ( !!( ent->flags ( ) & flags_t::on_ground ) && !animlayers [ 6 ].m_weight ) {
		vel.zero ( );
		return true;
	}

	if ( !records.size ( ) )
		return false;

	const auto previous_record = records.front ( );

	auto time_difference = std::max ( cs::ticks2time ( 1 ), ent->simtime ( ) - previous_record.m_simtime );
	auto origin_delta = origin - previous_record.m_origin;

	if ( origin_delta.is_zero ( ) ) {
		vel.zero ( );
		return true;
	}

	//if ( cs::time2ticks ( ent->simtime ( ) - previous_record.m_simtime ) <= 1 ) {
	//	vel = origin_delta / time_difference;
	//	return true;
	//}

	auto fixed = false;

	/* skeet */
	if ( !!( ent->flags ( ) & flags_t::on_ground ) ) {
		vel.z = 0.0f;
		vel = origin_delta / time_difference;

		auto max_speed = 260.0f;
		const auto weapon = g::local->weapon ( );

		if ( weapon && weapon->data ( ) )
			max_speed = g::local->scoped ( ) ? weapon->data ( )->m_max_speed_alt : weapon->data ( )->m_max_speed;

		if ( animlayers [ 6 ].m_weight <= 0.0f ) {
			vel.zero ( );
			fixed = true;
		}
		else {
			if ( animlayers [ 6 ].m_playback_rate > 0.0f ) {
				auto origin_delta_vel_norm = vel.normalized ( );
				origin_delta_vel_norm.z = 0.0f;

				auto origin_delta_vel_len = vel.length_2d ( );

				const auto flMoveWeightWithAirSmooth = animlayers [ 6 ].m_weight /*/ std::max ( 1.0f - animlayers [ 5 ].m_weight, 0.55f )*/;
				const auto flTargetMoveWeight_to_speed2d = std::lerp ( max_speed * 0.52f, max_speed * 0.34f, ent->crouch_amount ( ) ) * flMoveWeightWithAirSmooth;

				const auto speed_as_portion_of_run_top_speed = 0.35f * ( 1.0f - animlayers [ 11 ].m_weight );

				if ( animlayers [ 11 ].m_weight > 0.0f && animlayers [ 11 ].m_weight < 1.0f ) {
					vel = origin_delta_vel_norm * ( max_speed * ( speed_as_portion_of_run_top_speed + 0.55f ) );
					fixed = true;
				}
				else if ( flMoveWeightWithAirSmooth < 0.95f || flTargetMoveWeight_to_speed2d > origin_delta_vel_len ) {
					vel = origin_delta_vel_norm * flTargetMoveWeight_to_speed2d;
					fixed = flMoveWeightWithAirSmooth < 0.95f;
				}
				else {
					const float deployable_limited_max_speed = 260.0f;
					float flTargetMoveWeight_adjusted_speed2d = std::min ( max_speed, deployable_limited_max_speed );

					if ( !!( ent->flags ( ) & flags_t::ducking ) )
						flTargetMoveWeight_adjusted_speed2d *= 0.34f;
					else if ( ent->is_walking() )
						flTargetMoveWeight_adjusted_speed2d *= 0.52f;

					if ( origin_delta_vel_len > flTargetMoveWeight_adjusted_speed2d ) {
						vel = origin_delta_vel_norm * flTargetMoveWeight_adjusted_speed2d;
						fixed = true;
					}
				}
			}
		}
	}
	else {
		if ( animlayers [ 4 ].m_weight > 0.0f
			&& animlayers [ 4 ].m_playback_rate > 0.0f
			&& rebuilt::get_layer_activity ( ent->animstate ( ), 4 ) == 985 /* act_csgo_jump */ ) {
			vel.z = ( ( animlayers [ 4 ].m_cycle / animlayers [ 4 ].m_playback_rate ) / cs::ticks2time ( 1 ) + 0.5f ) * cs::ticks2time ( 1 );
			vel.z = g::cvars::sv_jump_impulse->get_float ( ) - vel.z * g::cvars::sv_gravity->get_float ( );
		}

		vel.x = origin_delta.x / time_difference;
		vel.y = origin_delta.y / time_difference;

		fixed = true;
	}

	/* predict vel dir */
	if ( records.size ( ) >= 2 && records [ 0 ].m_simtime - records [ 1 ].m_simtime > cs::ticks2time ( 1 ) && vel.length_2d ( ) > 0.0f ) {
		const auto last_avg_vel = ( records [ 0 ].m_origin - records [ 1 ].m_origin ) / ( records [ 0 ].m_simtime - records [ 1 ].m_simtime );
	
		if ( last_avg_vel.length_2d ( ) > 0.0f ) {
			float deg_1 = cs::rad2deg ( atan2 ( origin_delta.y, origin_delta.x ) );
			float deg_2 = cs::rad2deg ( atan2 ( last_avg_vel.y, last_avg_vel.x ) );
	
			float deg_delta = cs::normalize ( deg_1 - deg_2 );

			// dont curve too much, then we can get a very very wrong velocity
			if ( abs ( deg_delta ) > 6.0f )
				deg_delta = 0.0f;

			float deg_lerp = cs::normalize ( deg_1 + deg_delta * 0.5f );
			float rad_dir = cs::deg2rad ( deg_lerp );
	
			float sin_dir, cos_dir;
			cs::sin_cos ( rad_dir, &sin_dir, &cos_dir );
	
			float vel_len = vel.length_2d ( );
	
			vel.x = cos_dir * vel_len;
			vel.y = sin_dir * vel_len;
		}
	}

	return fixed;
	VMP_END ( );
}

void anims::update_from( player_t* ent , anim_info_t& from , anim_info_t& to, std::array<animlayer_t, 13>& cur_layers ) {
	const auto delta_ticks = cs::time2ticks( to.m_simtime - from.m_simtime );

	if ( !delta_ticks )
		return;

	to.m_has_vel = fix_velocity ( ent, to.m_vel, to.m_anim_layers, to.m_origin );

	const auto backup_anim_layers = to.m_anim_layers;
	const auto backup_abs_origin = ent->abs_origin( );

	/* update from last sent data */
	ent->angles( ) = to.m_angles;
	ent->origin( ) = to.m_origin;
	ent->mins( ) = to.m_mins;
	ent->maxs( ) = to.m_maxs;
	ent->lby( ) = to.m_lby;
	ent->simtime( ) = to.m_simtime;
	ent->old_simtime( ) = to.m_old_simtime;
	ent->crouch_amount( ) = to.m_duck_amount;
	memcpy ( ent->layers ( ), to.m_anim_layers.data ( ), sizeof ( to.m_anim_layers ) );
	ent->flags( ) = to.m_flags;
	ent->vel( ) = to.m_vel;
	ent->set_abs_origin( ent->origin( ) );

	/* start from last animation data */
	to.m_poses = from.m_poses;
	to.m_anim_layers = from.m_anim_layers;
	to.m_abs_angles = from.m_abs_angles;
	to.m_anim_state = from.m_anim_state;

	// fix flags and legs in air and landing on ground
	//if ( ( backup_anim_layers [ 6 ].m_weight > 0.0f && backup_anim_layers [ 6 ].m_playback_rate > 0.0f )
	//	|| ( backup_anim_layers [ 4 ].m_playback_rate > 0.0f && static_cast< int >( backup_anim_layers [ 4 ].m_cycle / backup_anim_layers [ 4 ].m_playback_rate ) == 0 ) ) {
	//	ent->flags ( ) |= flags_t::on_ground;
	//}
	//else {
	//	ent->flags ( ) &= ~flags_t::on_ground;
	//}

	update_all_anims ( ent, ent->angles ( ), to, cur_layers );

	/* restore data to current */
	memcpy( ent->layers( ) , backup_anim_layers.data( ) , sizeof( backup_anim_layers ) );

	/* try to guess the orientation of their real matrix */
	//resolver::resolve_desync( ent , to, shot );

	/* set animlayers to ones from the server after we are done using the predicted ones */
	to.m_anim_layers = backup_anim_layers;

	/* set ideal animation data to be displayed */
	*reinterpret_cast< float* >( reinterpret_cast< uintptr_t >( &to.m_anim_state ) + 0x9C ) = to.m_anim_layers [ 6 ].m_weight;
	*reinterpret_cast< float* >( reinterpret_cast< uintptr_t >( &to.m_anim_state ) + 0x98 ) = to.m_anim_layers [ 6 ].m_cycle;

	*reinterpret_cast< float* >( reinterpret_cast< uintptr_t >( &to.m_anim_state ) + 0x19C ) = to.m_anim_layers [ 7 ].m_sequence;
	*reinterpret_cast< float* >( reinterpret_cast< uintptr_t >( &to.m_anim_state ) + 0x190 ) = to.m_anim_layers [ 7 ].m_weight;
	*reinterpret_cast< float* >( reinterpret_cast< uintptr_t >( &to.m_anim_state ) + 0x198 ) = to.m_anim_layers [ 7 ].m_cycle;

	*reinterpret_cast< float* >( reinterpret_cast< uintptr_t >( &to.m_anim_state ) + 0x180 ) = to.m_anim_layers [ 12 ].m_weight;

	ent->poses ( ) = to.m_poses;
	*ent->animstate ( ) = to.m_anim_state;
	ent->set_abs_angles ( to.m_abs_angles );
	
	/* build bones for resolved side */
	auto backup_lean = ent->layers ( ) [ 12 ].m_weight;
	ent->layers ( ) [ 12 ].m_weight = 0.0f;
	ent->set_abs_angles ( to.m_abs_angles );
	build_bones ( ent, to.m_aim_bones.data ( ), 0x7FF00, vec3_t ( 0.0f, to.m_anim_state.m_abs_yaw, 0.0f ), to.m_origin, to.m_simtime, to.m_poses );
	ent->layers ( ) [ 12 ].m_weight = backup_lean;
}

void anims::update_all_anims ( player_t* ent, vec3_t& angles, anim_info_t& to, std::array<animlayer_t, 13>& cur_layers ) {
	VMP_BEGINMUTATION ( );
	static auto& resolver = options::vars[ _( "ragebot.resolve_desync" ) ].val.b;

	const auto anim_state = ent->animstate( );
	const auto anim_layers = ent->layers( );

	auto get_should_resolve = [ & ] ( ) {
		if ( !resolver || !g::local || !anim_state || !anim_layers || !g::local->is_enemy ( ent ) )
			return false;

		player_info_t pl_info;
		cs::i::engine->get_player_info( ent->idx( ) , &pl_info );

		if ( pl_info.m_fake_player )
			return false;

		return true;
	};

	auto should_resolve = get_should_resolve( );

	auto update_desync_side = [ & ] ( ) {
		/* update animations for these set of animlayers and animstate */
		*anim_state = to.m_anim_state;
		memcpy ( anim_layers, to.m_anim_layers.data ( ), sizeof ( to.m_anim_layers ) );
		ent->poses ( ) = to.m_poses;

		const auto backup_lby = ent->lby ( );
		vec3_t update_angles = angles;

		/* update animations */
		update_anims ( ent, update_angles );

		ent->lby ( ) = backup_lby;

		/* store new anim data */
		to.m_anim_state = *anim_state;
		memcpy ( to.m_anim_layers.data ( ), anim_layers, sizeof ( to.m_anim_layers ) );
		to.m_abs_angles = vec3_t ( 0.0f, anim_state->m_abs_yaw, 0.0f );
		to.m_poses = ent->poses ( );
	};

	/* backup animation data */
	std::array<animlayer_t , 13> backup_anim_layers {};
	memcpy( backup_anim_layers.data( ) , anim_layers , sizeof( backup_anim_layers ) );

	const auto backup_anim_state = *anim_state;
	const auto backup_poses = ent->poses();

	update_desync_side ( );

	/* restore animation data */
	memcpy( anim_layers , backup_anim_layers.data( ) , sizeof( backup_anim_layers ) );
	*anim_state = backup_anim_state;
	ent->poses( ) = backup_poses;
	VMP_END ( );
}

void anims::update_anims ( player_t* ent, vec3_t& angles, bool force_feet_yaw ) {
	static auto& invalidate_bone_cache = *pattern::search( _( "client.dll" ) , _( "C6 05 ? ? ? ? ? 89 47 70" ) ).add( 2 ).deref( ).get<bool*>( );
	static auto invalidate_physics_recursive = pattern::search( _( "client.dll" ) , _( "55 8B EC 83 E4 F8 83 EC 0C 53 8B 5D 08 8B C3 56" ) ).get<void( __thiscall* )( player_t* , int )>( );
	static auto init_last_data = false;

	static auto& static_legs = options::vars [ _ ( "misc.effects.static_legs" ) ].val.b;
	static auto& slowwalk_slide = options::vars [ _ ( "misc.effects.slowwalk_slide" ) ].val.b;
	static auto& no_pitch_on_land = options::vars [ _ ( "misc.effects.no_pitch_on_land" ) ].val.b;

	static auto& slowwalk_key = options::vars [ _ ( "antiaim.slow_walk_key" ) ].val.i;
	static auto& slowwalk_key_mode = options::vars [ _ ( "antiaim.slow_walk_key_mode" ) ].val.i;

	const auto state = ent->animstate( );
	const auto anim_layers = ent->layers( );

	if ( !state || !anim_layers )
		return;

	auto was_on_ground = state->m_on_ground;

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

		*reinterpret_cast< uint32_t* >( reinterpret_cast< uintptr_t > ( ent ) + 0xE8 ) &= ~0x1800;
		*reinterpret_cast< vec3_t* >( reinterpret_cast< uintptr_t > ( ent ) + 0x94 ) = ent->vel( );
	}

	if ( ent == g::local && g::local ) {
		//*state = last_local_animstate;
		auto new_layers = last_anim_layers_server;
		copy_client_layers ( ent, new_layers, last_anim_layers_queued );
		last_anim_layers_queued = new_layers;

		memcpy( ent->layers( ), last_anim_layers_queued.data( ) , sizeof( last_anim_layers_queued ) );

		cs::i::globals->m_frametime = cs::ticks2time ( 1 );

		//state->m_last_clientside_anim_update = cs::i::globals->m_curtime - cs::ticks2time( 1 );
		//state->m_feet_yaw_rate = 0.0f;
	}
	
	const auto backup_invalidate_bone_cache = invalidate_bone_cache;

	rebuilt::update( state , angles , ent->origin( ), force_feet_yaw );

	invalidate_bone_cache = backup_invalidate_bone_cache;

	if ( ent == g::local && g::local ) {
		memcpy( last_anim_layers_queued.data( ), ent->layers( ) , sizeof( last_anim_layers_queued ) );
		//last_local_animstate = *state;
	}

	cs::i::globals->m_frametime = backup_frametime;
	cs::i::globals->m_curtime = backup_curtime;

	if ( g::local && ent == g::local ) {
		build_bones ( ent, real_matrix.data ( ), 0x7FF00, vec3_t( 0.0f, state->m_abs_yaw, 0.0f ), ent->origin ( ), cs::i::globals->m_curtime, ent->poses ( ) );
		usable_origin_real_local = ent->origin ( );

		/* dont update on local until we send packet */
		/*if ( g::send_packet )*/ {
			/* set new animlayers (server data + ones maintained on client) */
			memcpy( last_anim_layers.data( ) , last_anim_layers_queued.data( ) , sizeof( last_anim_layers_queued ) );

			//last_anim_layers[ 3 ].m_weight = 0.0f;
			//last_anim_layers[ 3 ].m_cycle = 0.0f;
			last_anim_layers[ 12 ].m_weight = 0.0f;

			/* store new data */
			memcpy( ent->layers( ) , last_anim_layers.data( ) , sizeof( last_anim_layers ) );

			last_local_feet_yaw = cs::normalize( state->m_abs_yaw);
			last_local_poses = ent->poses( );
		}

		/* apply last sent data */
		ent->poses( ) = last_local_poses;
		memcpy( ent->layers( ) , last_anim_layers.data( ) , sizeof( last_anim_layers ) );

		ent->set_abs_angles( vec3_t( 0.0f , last_local_feet_yaw , 0.0f ) );

		/*if ( g::send_packet )*/ {
			std::array< animlayer_t, 13> backup_anim_layers {};
			memcpy ( backup_anim_layers.data ( ), ent->layers ( ), sizeof ( backup_anim_layers ) );
			const auto backup_poses = ent->poses ( );

			if ( static_legs ) {
				ent->poses ( ) [ pose_param_t::jump_fall ] = 1.0f;
			}

			if ( no_pitch_on_land && state->m_hit_ground && state->m_on_ground && was_on_ground ) {
				float flPitch = -10.0f;

				if ( flPitch <= 0.0f )
flPitch = ( flPitch / state->m_max_pitch ) * -90.0f;
				else
				flPitch = ( flPitch / state->m_min_pitch ) * 90.0f;

				ent->poses ( ) [ pose_param_t::body_pitch ] = std::clamp ( flPitch, -90.0f, 90.0f ) / 180.0f + 0.5f;
			}

			if ( slowwalk_slide && utils::keybind_active ( slowwalk_key, slowwalk_key_mode ) && state->m_on_ground ) {
				ent->layers ( ) [ 6 ].m_weight = 0.0f;
			}

			build_bones ( ent, usable_bones [ ent->idx ( ) ].data ( ), 0x7FF00, ent->abs_angles ( ), ent->origin ( ), cs::i::globals->m_curtime, ent->poses ( ) );

			ent->poses ( ) = backup_poses;
			memcpy ( ent->layers ( ), backup_anim_layers.data ( ), sizeof ( backup_anim_layers ) );

			usable_origin [ ent->idx ( ) ] = ent->origin ( );

			//manage_fake ( );
		}
	}
}

void anims::apply_anims ( player_t* ent ) {
	if ( !g::local )
		return;

	if ( ent == g::local ) {
		ent->poses ( ) = last_local_poses;
		ent->set_abs_angles ( vec3_t ( 0.0f, last_local_feet_yaw, 0.0f ) );
		memcpy ( ent->layers ( ), last_anim_layers.data ( ), sizeof ( last_anim_layers ) );
		return;
	}

	auto& info = anim_info [ ent->idx ( ) ];

	if ( info.empty ( ) )
		return;

	auto& first = info.front ( );

	usable_bones [ ent->idx ( ) ] = first.m_aim_bones;
	usable_origin [ ent->idx ( ) ] = first.m_origin;

	ent->set_abs_angles ( first.m_abs_angles );
	ent->poses ( ) = first.m_poses;
}

namespace lby {
	extern bool in_update;
}

void anims::recalc_poses ( std::array<float, 24>& poses, float ladder_yaw, float move_yaw, float eye_yaw, float feet_yaw ) {
	poses [ pose_param_t::strafe_yaw ] = poses [ pose_param_t::move_yaw ] = std::clamp ( cs::normalize ( move_yaw ), -180.0f, 180.0f ) / 360.0f + 0.5f;
	poses [ pose_param_t::body_yaw ] = std::clamp ( cs::normalize ( eye_yaw - feet_yaw ), -58.0f, 58.0f ) / 116.0f + 0.5f;
	poses [ pose_param_t::ladder_yaw ] = std::clamp ( cs::normalize ( ladder_yaw - feet_yaw ), -180.0f, 180.0f ) / 360.0f + 0.5f;
}

float yaw_diff ( float yaw1, float yaw2 ) {
	return abs ( cs::normalize ( cs::normalize ( yaw2 ) - cs::normalize ( yaw1 ) ) );
}

bool is_angle_within_range ( float yaw1, float yaw2, float range ) {
	return yaw_diff ( yaw1, yaw2 ) < range;
}

bool is_angle_same ( float yaw1, float yaw2 ) {
	return is_angle_within_range ( yaw1, yaw2, 35.0f ); // 35 degrees off center
}

void anims::resolve_player ( player_t* player, bool& lby_updated_out ) {
	static auto& resolver_enabled = options::vars [ _ ( "ragebot.resolve_desync" ) ].val.b;

	const auto index = player->idx ( );
	const float simtime = player->simtime ( );
	const float lby = cs::normalize ( player->lby ( ) );
	const auto on_ground = !!( player->flags ( ) & flags_t::on_ground );
	const auto animlayers = player->layers ( );
	
	lby_updated_out = false;

	// Store resolver info always
	if ( on_ground && animlayers ) {
		layer3 [ index - 1 ] = animlayers [ 3 ];

		// If player is moving, store last moving lby angle
		if ( animlayers [ 6 ].m_weight > 0.0f ) {
			// Detect real jitter while the player is moving
			// last moving lby history are all valid
			if ( last_moving_lby [ index - 1 ] != std::numeric_limits<float>::max ( )
				&& last_last_moving_lby [ index - 1 ] != std::numeric_limits<float>::max ( ) ) {
				// Previous 3 angles are within range of eachother
				if ( !is_angle_within_range ( lby, last_moving_lby [ index - 1 ], 2.0f )
					//&& !is_angle_within_range ( lby, last_last_moving_lby [ index - 1 ], 2.0f )
					&& !is_angle_within_range ( last_moving_lby [ index - 1 ], last_last_moving_lby [ index - 1 ], 2.0f )
					&& is_angle_within_range ( lby, last_moving_lby [ index - 1 ], 35.0f )
					&& is_angle_within_range ( lby, last_last_moving_lby [ index - 1 ], 35.0f )
					&& is_angle_within_range ( last_moving_lby [ index - 1 ], last_last_moving_lby [ index - 1 ], 35.0f ) ) {
					has_real_jitter [ index - 1 ] = true;
				}
				else if ( is_angle_within_range ( lby, last_moving_lby [ index - 1 ], 5.0f )
					&& is_angle_within_range ( lby, last_last_moving_lby [ index - 1 ], 5.0f )
					&& is_angle_within_range ( last_moving_lby [ index - 1 ], last_last_moving_lby [ index - 1 ], 5.0f ) ) {
					has_real_jitter [ index - 1 ] = false;
				}
			}

			last_moving_time [ index - 1 ] = simtime;
			last_last_moving_lby [ index - 1 ] = last_moving_lby [ index - 1 ];
			last_moving_lby [ index - 1 ] = lby;

			last_moving_lby_time [ index - 1 ] = simtime;
			next_lby_update_time [ index - 1 ] = simtime + 0.22f;
			triggered_balance_adjust [ index - 1 ] = false;
			lby_updates_within_jitter_range [ index - 1 ] = 0;
			//has_real_jitter [ index - 1 ] = false;

			lby_updated_out = true;
		}
		else if ( !is_angle_within_range ( lby, last_lby [ index - 1 ], 35.0f ) // lby != last_lby [ index - 1 ] // If LBY changed, we know it updated, and the next update must be 1.1f second from this one
			// ^^ Also, we can correct the LBY timer if it goes out of sync
			|| simtime >= next_lby_update_time [ index - 1 ] ) { // If time since updated elapsed, this is probably an update
			next_lby_update_time [ index - 1 ] = simtime + 1.1f;
			lby_updated_out = true;

			// If the LBY changed but still within 35 degrees of the original value,
			// The angle was probably around the same but within 35 degrees
			// So they probably have something like jitter on their real
			const bool lby_update_is_close = is_angle_within_range ( lby, last_lby [ index - 1 ], 35.0f );

			if ( lby != last_lby [ index - 1 ]
				&& lby_update_is_close ) {
				lby_updates_within_jitter_range [ index - 1 ]++;
			}
			else if ( !lby_update_is_close ) {
				lby_updates_within_jitter_range [ index - 1 ] = 0;
			}

			// LBY changed to very close amounts 3 times in a row, so they might have jitter on their real
			if ( lby_updates_within_jitter_range [ index - 1 ] >= 3 ) {
				has_real_jitter [ index - 1 ] = true;
			}

			// Sequence 4 on layer 3 is balance adjust
			const float time_since_moved = abs ( simtime - last_moving_lby_time [ index - 1 ] );

			// Balance adjust was triggered after the first LBY update
			// and before too long (so we don't record balance adjusts for the player simply rotating their view)
			if ( animlayers [ 3 ].m_sequence == 4 && time_since_moved > 0.22f && time_since_moved < 3.0f ) {
				triggered_balance_adjust [ index - 1 ] = true;
			}
		}
	}
	else {
		// Reset LBY updates information
		last_moving_lby_time [ index - 1 ] = next_lby_update_time [ index - 1 ] = std::numeric_limits<float>::max ( );
		last_last_moving_lby [ index - 1 ] = last_moving_lby [ index - 1 ] = std::numeric_limits<float>::max ( );
		triggered_balance_adjust [ index - 1 ] = false;
		lby_updates_within_jitter_range [ index - 1 ] = 0;
		//has_real_jitter [ index - 1 ] = false;
	}

	// Store old values for later
	last_lby [ index - 1 ] = lby;

	// If player is occluded on one side, store freestand angle
	if ( const auto weapon = g::local->weapon ( ) ) {
		const auto src = g::local->origin ( ) + vec3_t ( 0.0f, 0.0f, 64.0f );

		const auto eyes_max = player->origin ( ) + vec3_t ( 0.0f, 0.0f, 64.0f );
		auto fwd = eyes_max - src;

		fwd.normalize ( );

		if ( fwd.is_valid ( ) ) {
			const auto at_target_yaw = cs::calc_angle ( g::local->origin ( ), player->origin ( ) ).y;
			const auto right_dir = fwd.cross_product ( vec3_t ( 0.0f, 0.0f, 1.0f ) );
			const auto left_dir = -right_dir;

			const auto dmg_left = awall_skeet::dmg ( g::local, src + left_dir * 35.0f, eyes_max + left_dir * 35.0f, weapon, player, 1.0f, true );
			const auto dmg_left_far = awall_skeet::dmg ( g::local, src + left_dir * 64.0f, eyes_max + left_dir * 64.0f, weapon, player, 1.0f, true );

			const auto dmg_right = awall_skeet::dmg ( g::local, src + right_dir * 35.0f, eyes_max + right_dir * 35.0f, weapon, player, 1.0f, true );
			const auto dmg_right_far = awall_skeet::dmg ( g::local, src + right_dir * 64.0f, eyes_max + right_dir * 64.0f, weapon, player, 1.0f, true );

			const float most_dmg_left = std::max ( dmg_left, dmg_left_far );
			const float most_dmg_right = std::max ( dmg_right, dmg_right_far );

			if ( most_dmg_left > 0.0f && most_dmg_right <= 0.0f ) {
				last_freestanding [ index - 1 ] = cs::normalize ( at_target_yaw - 90.0f );
				last_freestand_time [ index - 1 ] = simtime;
			}
			else if ( most_dmg_left <= 0.0f && most_dmg_right > 0.0f ) {
				last_freestanding [ index - 1 ] = cs::normalize ( at_target_yaw + 90.0f );
				last_freestand_time [ index - 1 ] = simtime;
			}
			else {
				last_freestand_time [ index - 1 ] = std::numeric_limits<float>::max ( );
			}
		}
	}

	// Run resolver logic / smart angle selection

	// LBY updated, tap them
	float final_angle = player->angles ( ).y;

	std::vector<float> possible_resolver_angles = {};

	possible_resolver_angles.push_back ( final_angle );
	possible_resolver_angles.push_back ( lby );
	possible_resolver_angles.push_back ( cs::normalize ( lby + 120.0f ) );
	possible_resolver_angles.push_back ( cs::normalize ( lby - 120.0f ) );
	possible_resolver_angles.push_back ( cs::normalize ( lby + 0.0f ) ); // !!!!
	possible_resolver_angles.push_back ( last_freestanding [ index - 1 ] );
	possible_resolver_angles.push_back ( last_last_moving_lby [ index - 1 ] );
	possible_resolver_angles.push_back ( cs::calc_angle ( g::local->origin ( ), player->origin ( ) ).y );

	// Updated LBY is nearby last moving lby within first update, they are probably breaking with low delta
	if ( last_moving_lby_time [ index - 1 ] != std::numeric_limits<float>::max ( )
		&& yaw_diff ( lby, last_moving_lby [ index - 1 ] ) < 60.0f ) {
		// Pull LBY towards last moving LBY
		possible_resolver_angles [ 4 ] = cs::normalize ( lby + copysign ( 60.0f, cs::normalize ( last_moving_lby [ index - 1 ] - lby ) ) );
		resolver_mode [ index - 1 ] = ResolveMode::LowLBY;
	}

	//// remove duplicate yaws
	//for ( auto& yaw : possible_resolver_angles ) {
	//	for ( auto other_yaw : possible_resolver_angles ) {
	//		if ( yaw != other_yaw && is_angle_same ( yaw, other_yaw ) )
	//			yaw = std::numeric_limits<float>::max ( );
	//	}
	//}

	if ( lby_updated_out ) {
		final_angle = lby;

		// Lby update when standing, and triggered balanceadjust
		// Real may be offset from LBY
		if ( animlayers [ 6 ].m_weight == 0.0f /*&& animlayers [ 3 ].m_sequence == 4*/ ) {
			// Point LBY towards freestand side
			if ( const auto weapon = g::local->weapon ( ) ) {
				auto angle_to_dir = [ ] ( float angle_deg ) {
					float s, c;
					cs::sin_cos ( cs::deg2rad ( angle_deg ), &s, &c );
					return vec3_t ( c, s, 0.0f );
				};

				const auto src = g::local->origin ( ) + vec3_t ( 0.0f, 0.0f, 64.0f );

				const auto eyes_max = player->origin ( ) + vec3_t ( 0.0f, 0.0f, 64.0f );
				auto fwd = eyes_max - src;

				fwd.normalize ( );

				if ( fwd.is_valid ( ) ) {
					const auto right_dir = fwd.cross_product ( vec3_t ( 0.0f, 0.0f, 1.0f ) );
					const auto left_dir = -right_dir;

					const auto right_dir = angle_to_dir ( possible_resolver_angles [ ResolveMode::PositiveLBY ] );
					const auto left_dir = angle_to_dir ( possible_resolver_angles [ ResolveMode::NegativeLBY ] );

					const auto dmg_left = awall_skeet::dmg ( g::local, src + left_dir * 35.0f, eyes_max + left_dir * 35.0f, weapon, player, 1.0f, true );
					const auto dmg_left_far = awall_skeet::dmg ( g::local, src + left_dir * 64.0f, eyes_max + left_dir * 64.0f, weapon, player, 1.0f, true );

					const auto dmg_right = awall_skeet::dmg ( g::local, src + right_dir * 35.0f, eyes_max + right_dir * 35.0f, weapon, player, 1.0f, true );
					const auto dmg_right_far = awall_skeet::dmg ( g::local, src + right_dir * 64.0f, eyes_max + right_dir * 64.0f, weapon, player, 1.0f, true );

					const float most_dmg_left = std::max ( dmg_left, dmg_left_far );
					const float most_dmg_right = std::max ( dmg_right, dmg_right_far );

					if ( most_dmg_left > 0.0f && most_dmg_right <= 0.0f ) {
						resolver_mode [ index - 1 ] = ResolveMode::PositiveLBY;
					}
					else if ( most_dmg_left <= 0.0f && most_dmg_right > 0.0f ) {
						resolver_mode [ index - 1 ] = ResolveMode::NegativeLBY;
					}
				}
			}

			// Updated LBY is nearby last moving lby within first update, they are probably breaking with low delta
			if ( last_moving_lby_time [ index - 1 ] != std::numeric_limits<float>::max ( )
				&& yaw_diff ( lby, last_moving_lby [ index - 1 ] ) < 60.0f ) {
				resolver_mode [ index - 1 ] = ResolveMode::LowLBY;
			}
		}
	}
	// Normal resolve
	else {
		// If freestand is the same as last moving lby, use last moving lby as more accurate guess
		if ( last_freestand_time [ index - 1 ] != std::numeric_limits<float>::max ( )
			&& last_moving_lby_time [ index - 1 ] != std::numeric_limits<float>::max ( )
			&& is_angle_same ( last_freestanding [ index - 1 ], last_moving_lby [ index - 1 ] ) ) {
			resolver_mode [ index - 1 ] = ResolveMode::LastMovingLBY;
		}

		// No lby breaker, so they probably have no fake
		if ( on_ground
			&& last_moving_lby_time [ index - 1 ] != std::numeric_limits<float>::max ( )
			&& abs ( simtime - last_moving_lby_time [ index - 1 ] ) > 0.5f
			&& is_angle_same ( lby, last_moving_lby [ index - 1 ] ) ) {
			if ( is_angle_same ( lby, player->angles ( ).y ) )
				resolver_mode [ index - 1 ] = ResolveMode::None;
			else
				resolver_mode [ index - 1 ] = ResolveMode::LBY;
		}
		
		// Real while moving has low variance, and we don't know their resolve mode
		// Just force LBY and hope for the best
		if ( on_ground
			&& !has_real_jitter [ index - 1 ]
			&& ( resolver_mode [ index - 1 ] == ResolveMode::None || resolver_mode [ index - 1 ] == ResolveMode::Backwards ) ) {
			resolver_mode [ index - 1 ] = ResolveMode::LBY;
		}

		// In air for more than 3 seconds or don't have a good side to hide the head on
		if ( ( !on_ground && abs ( simtime - last_moving_time [ index - 1 ] ) > 3.0f )
			|| last_freestand_time [ index - 1 ] == std::numeric_limits<float>::max ( ) ) {
			resolver_mode [ index - 1 ] = ResolveMode::Backwards;
		}
		
		// Apply wanted yaw
		final_angle = possible_resolver_angles [ resolver_mode [ index - 1 ] ];

		// Apply jitter if needed
		if ( has_real_jitter [ index - 1 ] ) {
			const bool rand_sign = ( rand ( ) % 2 == 0 ) ? -1.0f : 1.0f;
			const float jitter_range = 35.0f;
			const float rand_factor = static_cast< float >( rand ( ) ) / static_cast< float >( RAND_MAX );
		
			final_angle += rand_factor * ( jitter_range / 2.0f ) * rand_sign;
		}
	}

	// Only apply final angle if resolver is enabled
	if ( resolver_enabled ) {
		player->angles ( ).y = cs::normalize ( final_angle );
	}

	// TODO pitch resolver
}

void anims::on_net_update_end ( int idx ) {
	VMP_BEGINMUTATION ( );
	auto ent = cs::i::ent_list->get<player_t*> ( idx );
	
	if ( !ent || !ent->is_player ( ) || !ent->alive ( ) || ent->dormant ( ) || !ent->animstate( ) || !ent->layers( ) )
		return reset_data ( idx );

	if ( g::local && ent == g::local ) {
		memcpy ( last_anim_layers_server.data ( ), ent->layers ( ), sizeof ( last_anim_layers_server ) );

		//dbg_print ( "TEST: %.2f\n", last_anim_layers_server [ 11 ].m_weight );

		return;
	}

	if ( anim_info [ idx ].empty ( ) || ent->simtime ( ) > ent->old_simtime() ) {
		/* fixes crashing caused by animation fix when there are severe frame drops (dont animate between choked commands if gaps between simtime is too big) */
		if ( !anim_info [ idx ].empty ( ) && abs( ent->simtime( ) - anim_info [ idx ].front ( ).m_simtime ) > cs::ticks2time( 64 ) )
			reset_data ( idx );

		anim_info_t rec;

		// massive resolver!!!
		bool lby_update = false;
		resolve_player ( ent, lby_update );

		/* if we don't have any information from the player, start animating from what we have now */
		if ( anim_info[ idx ].empty( ) ) {
			rec = anim_info_t( ent );

			rec.m_lby_update = lby_update;

			if ( rec.m_shot )
				shot_count[ idx ]++;

			const auto backup_anim_layers = rec.m_anim_layers;

			rec.m_has_vel = fix_velocity ( ent, ent->vel ( ), backup_anim_layers, ent->origin ( ) );

			*reinterpret_cast< float* >( reinterpret_cast< uintptr_t >( &rec.m_anim_state ) + 0x9C ) = backup_anim_layers [ 6 ].m_weight;
			*reinterpret_cast< float* >( reinterpret_cast< uintptr_t >( &rec.m_anim_state ) + 0x98 ) = backup_anim_layers [ 6 ].m_cycle;

			*reinterpret_cast< float* >( reinterpret_cast< uintptr_t >( &rec.m_anim_state ) + 0x19C ) = backup_anim_layers [ 7 ].m_sequence;
			*reinterpret_cast< float* >( reinterpret_cast< uintptr_t >( &rec.m_anim_state ) + 0x190 ) = backup_anim_layers [ 7 ].m_weight;
			*reinterpret_cast< float* >( reinterpret_cast< uintptr_t >( &rec.m_anim_state ) + 0x198 ) = backup_anim_layers [ 7 ].m_cycle;

			*reinterpret_cast< float* >( reinterpret_cast< uintptr_t >( &rec.m_anim_state ) + 0x180 ) = backup_anim_layers [ 12 ].m_weight;

			update_all_anims ( ent, ent->angles ( ), rec, rec.m_anim_layers );

			*reinterpret_cast< float* >( reinterpret_cast< uintptr_t >( &rec.m_anim_state ) + 0x9C ) = backup_anim_layers [ 6 ].m_weight;
			*reinterpret_cast< float* >( reinterpret_cast< uintptr_t >( &rec.m_anim_state ) + 0x98 ) = backup_anim_layers [ 6 ].m_cycle;

			*reinterpret_cast< float* >( reinterpret_cast< uintptr_t >( &rec.m_anim_state ) + 0x19C ) = backup_anim_layers [ 7 ].m_sequence;
			*reinterpret_cast< float* >( reinterpret_cast< uintptr_t >( &rec.m_anim_state ) + 0x190 ) = backup_anim_layers [ 7 ].m_weight;
			*reinterpret_cast< float* >( reinterpret_cast< uintptr_t >( &rec.m_anim_state ) + 0x198 ) = backup_anim_layers [ 7 ].m_cycle;

			*reinterpret_cast< float* >( reinterpret_cast< uintptr_t >( &rec.m_anim_state ) + 0x180 ) = backup_anim_layers [ 12 ].m_weight;

			/* try to guess the orientation of their real matrix */
			//resolver::resolve_desync ( ent , rec, false );

			/* set animlayers to ones from the server after we are done using the predicted ones */
			rec.m_anim_layers = backup_anim_layers;

			ent->poses ( ) = rec.m_poses;
			*ent->animstate ( ) = rec.m_anim_state ;
			ent->set_abs_angles ( rec.m_abs_angles );

			/* build bones for resolved side */
			auto backup_lean = ent->layers ( ) [ 12 ].m_weight;
			ent->layers ( ) [ 12 ].m_weight = 0.0f;
			ent->set_abs_angles ( rec.m_abs_angles );
			build_bones ( ent, rec.m_aim_bones.data ( ), 0x7FF00, vec3_t ( 0.0f, rec.m_anim_state.m_abs_yaw, 0.0f ), rec.m_origin, rec.m_simtime, rec.m_poses );
			ent->layers ( ) [ 12 ].m_weight = backup_lean;

			if ( rec.m_simtime <= rec.m_old_simtime )
				rec.m_invalid = true;

			anim_info[ idx ].push_front( rec );

			if ( !rec.m_invalid )
				lagcomp_track [ idx ].push_front ( &anim_info [ idx ].front ( ) );
		}
		else {
			rec = anim_info_t( ent );

			rec.m_lby_update = lby_update;

			if ( rec.m_shot )
				shot_count[ idx ]++;

			if ( !anim_info [ idx ].empty ( ) && ent->origin ( ).dist_to_sqr ( anim_info [ idx ].front ( ).m_origin ) > 4096.0f ) {
				/* invalid new record */
				rec.m_invalid = true;
				
				/* invalidate all previous records */
				for ( auto& rec : anim_info [ idx ] )
					rec.m_invalid = true;
			}

			update_from ( ent, anim_info [ idx ].front ( ), rec, rec.m_anim_layers );

			if ( rec.m_simtime <= rec.m_old_simtime )
				rec.m_invalid = true;

			anim_info[ idx ].push_front( rec );

			if ( !rec.m_invalid )
				lagcomp_track [ idx ].push_front ( &anim_info [ idx ].front ( ) );
		}
	}
	
	const auto server_time = cs::i::globals->m_curtime;

	while ( !anim_info [ idx ].empty ( ) && abs ( anim_info [ idx ].back ( ).m_simtime - server_time ) > 1.0f )
		anim_info [ idx ].pop_back ( );

	/* https://github.com/ValveSoftware/source-sdk-2013/blob/master/mp/src/game/server/player_lagcompensation.cpp#L246 */
	int dead_time = server_time - g::cvars::sv_maxunlag->get_float( );
	int tail_index = lagcomp_track [ idx ].size( ) - 1;

	while ( tail_index < lagcomp_track [ idx ].size ( ) ) {
		const auto tail = lagcomp_track [ idx ][ tail_index ];

		/* if tail is within limits, stop */
		if ( tail->m_simtime >= dead_time )
			break;

		/* remove tail, get new tail */
		lagcomp_track [ idx ].pop_back( );
		tail_index = lagcomp_track [ idx ].size ( ) - 1;
	}
	VMP_END ( );
}

void anims::on_render_start ( int idx ) {
	auto ent = cs::i::ent_list->get<player_t*> ( idx );

	if ( !ent || !ent->is_player ( ) || !ent->alive ( ) || ent->dormant ( ) || !ent->animstate ( ) )
		return reset_data ( idx );

	apply_anims ( ent );
}

void anims::pre_fsn ( int stage ) {
	if ( g::round == round_t::starting || !g::local || !g::local->alive ( ) ) {
		for ( auto i = 1; i <= cs::i::globals->m_max_clients; i++ )
			features::ragebot::get_misses ( i ) = { 0 };

		if ( g::local && g::local->layers ( ) ) {
			memcpy ( last_anim_layers_server.data ( ), g::local->layers ( ), sizeof ( last_anim_layers_server ) );
			memcpy ( last_anim_layers_queued.data ( ), g::local->layers ( ), sizeof ( last_anim_layers_queued ) );
			memcpy ( last_anim_layers.data ( ), g::local->layers ( ), sizeof ( last_anim_layers ) );

			anims::manage_fake ( );
		}
	}

	if ( !g::local ) {
		anims::manage_fake ( );

		for ( auto i = 1; i <= cs::i::globals->m_max_clients; i++ )
			reset_data ( i );

		memset ( last_anim_layers_server.data ( ), 0, sizeof ( last_anim_layers_server ) );
		memset ( last_anim_layers_queued.data ( ), 0, sizeof ( last_anim_layers_queued ) );
		memset ( last_anim_layers.data ( ), 0, sizeof ( last_anim_layers ) );

		return;
	}
	else if ( !g::local->alive ( ) ) {
		anims::manage_fake ( );

		if ( g::local->layers ( ) ) {
			memcpy ( last_anim_layers_server.data ( ), g::local->layers ( ), sizeof ( last_anim_layers_server ) );
			memcpy ( last_anim_layers_queued.data ( ), g::local->layers ( ), sizeof ( last_anim_layers_queued ) );
			memcpy ( last_anim_layers.data ( ), g::local->layers ( ), sizeof ( last_anim_layers ) );
		}
	}

	switch ( stage ) {
	case 5: {
		for ( auto i = 1; i <= cs::i::globals->m_max_clients; i++ )
			on_render_start ( i );
	} break;
	}
}

void anims::fsn ( int stage ) {
	VMP_BEGINMUTATION ( );
	if ( stage == 4 )
		resolver::process_event_buffer ( );

	if ( !g::local ) {
		anims::manage_fake ( );

		for ( auto i = 1; i <= cs::i::globals->m_max_clients; i++ )
			reset_data ( i );

		memset ( last_anim_layers_server.data ( ), 0, sizeof ( last_anim_layers_server ) );
		memset ( last_anim_layers_queued.data ( ), 0, sizeof ( last_anim_layers_queued ) );
		memset ( last_anim_layers.data ( ), 0, sizeof ( last_anim_layers ) );

		return;
	}
	else if ( !g::local->alive ( ) ) {
		anims::manage_fake ( );

		if ( g::local->layers ( ) ) {
			memcpy ( last_anim_layers_server.data ( ), g::local->layers ( ), sizeof ( last_anim_layers_server ) );
			memcpy ( last_anim_layers_queued.data ( ), g::local->layers ( ), sizeof ( last_anim_layers_queued ) );
			memcpy ( last_anim_layers.data ( ), g::local->layers ( ), sizeof ( last_anim_layers ) );
		}
	}

	using update_all_viewmodel_addons_t = int( __fastcall* )( void* );
	static auto update_all_viewmodel_addons = pattern::search( _( "client.dll" ) , _( "55 8B EC 83 E4 ? 83 EC ? 53 8B D9 56 57 8B 03 FF 90 ? ? ? ? 8B F8 89 7C 24 ? 85 FF 0F 84 ? ? ? ? 8B 17 8B CF" ) ).get< update_all_viewmodel_addons_t >( );

	switch ( stage ) {
	case 4: {
		/*
		threads::thread_mgr_t<int> anim_threads;

		for ( auto i = 1; i <= cs::i::globals->m_max_clients; i++ )
			anim_threads.create_thread ( on_net_update_end, i );

		anim_threads.join_all ( );
		*/

		for ( auto i = 1; i <= cs::i::globals->m_max_clients; i++ )
			on_net_update_end ( i );
	} break;
	case 5: {
		if ( g::local ) {
			/* update viewmodel manually to fix it dissappearing */
			const auto viewmodel = g::local->viewmodel_handle( );

			if ( viewmodel != -1 && cs::i::ent_list->get_by_handle< void* >( viewmodel ) )
				update_all_viewmodel_addons( cs::i::ent_list->get_by_handle< void* >( viewmodel ) );
		}
	} break;
	}
	VMP_END ( );
}