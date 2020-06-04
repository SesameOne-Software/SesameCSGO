#include "lagcomp.hpp"
#include <deque>
#include "autowall.hpp"
#include "../menu/menu.hpp"
#include "../globals.hpp"
#include "../animations/animations.hpp"
#include "prediction.hpp"
#include "../animations/resolver.hpp"

namespace features::lagcomp::data {
	std::array< int, 65 > shot_count { 0 };
	std::array< vec3_t, 65 > old_origins;
	std::array< float, 65 > old_shots;
	std::array< features::lagcomp::lag_record_t, 65 > interpolated_oldest;
	std::array< std::deque< features::lagcomp::lag_record_t >, 65 > records;
	std::array< std::deque< features::lagcomp::lag_record_t >, 65 > all_records;
	std::array< features::lagcomp::lag_record_t, 65 > cham_records;
	std::array< features::lagcomp::lag_record_t, 65 > shot_records;
	std::array< std::deque< features::lagcomp::lag_record_t >, 65 > extrapolated_records;
}

float features::lagcomp::lerp( ) {
	constexpr const auto cv_lerp = 0.031000f;
	constexpr const auto cv_ratio = 2.0f;
	return std::max< float >( cv_lerp, 2.0f * csgo::i::globals->m_ipt );
};

const std::pair< std::deque< features::lagcomp::lag_record_t >&, bool > features::lagcomp::get ( player_t* pl ) {
	return { data::records [ pl->idx ( ) ], !data::records [ pl->idx ( ) ].empty ( ) };
}

const std::pair< std::deque< features::lagcomp::lag_record_t >&, bool > features::lagcomp::get_all ( player_t* pl ) {
	return { data::all_records [ pl->idx ( ) ], !data::all_records [ pl->idx ( ) ].empty ( ) };
}

const std::pair< features::lagcomp::lag_record_t&, bool > features::lagcomp::get_extrapolated( player_t* pl ) {
	return { data::extrapolated_records [ pl->idx( ) ][ 0 ], !data::extrapolated_records [ pl->idx ( ) ].empty ( ) && data::extrapolated_records [ pl->idx( ) ][ 0 ].m_pl && !data::extrapolated_records [ pl->idx ( ) ][ 0 ].m_needs_matrix_construction };
}

const std::pair< features::lagcomp::lag_record_t&, bool > features::lagcomp::get_shot( player_t* pl ) {
	return { data::shot_records [ pl->idx( ) ], data::shot_records [ pl->idx ( ) ].m_pl };
}

bool features::lagcomp::extrapolate_record( player_t* pl, lag_record_t& rec, bool shot ) {
	static auto player_rsc_ptr = pattern::search ( _ ( "client.dll" ), _ ( "8B 3D ? ? ? ? 85 FF 0F 84 ? ? ? ? 81 C7" ) ).add ( 2 ).deref ( ).get< uintptr_t > ( );

	auto nci = csgo::i::engine->get_net_channel_info ( );

	if ( !nci
		|| data::all_records [ pl->idx( ) ].empty( )
		|| pl->dormant( )
		|| !pl->animstate( ) 
		|| !pl->bone_cache( ) 
		|| !pl->layers( ) )
		return false;

	const auto extrapolate = [ & ] ( player_t* player, vec3_t& origin, vec3_t& velocity, int& flags, bool on_ground ) -> void {
		constexpr const auto sv_gravity = 800.0f;
		constexpr const auto sv_jump_impulse = 301.993377f;

		if ( !( flags & 1 ) )
			velocity.z -= csgo::ticks2time ( sv_gravity );
		else if ( flags & 1 && !on_ground )
			velocity.z = sv_jump_impulse;

		const auto src = origin;
		auto end = src + velocity * csgo::i::globals->m_ipt;

		ray_t r;
		r.init ( src, end, player->mins ( ), player->maxs ( ) );

		trace_t t { };
		trace_filter_t filter;
		filter.m_skip = player;

		csgo::util_tracehull ( src, end, player->mins ( ), player->maxs ( ), 0x201400B, player, &t );

		if ( t.m_fraction != 1.0f ) {
			for ( auto i = 0; i < 2; i++ ) {
				velocity -= t.m_plane.m_normal * velocity.dot_product ( t.m_plane.m_normal );

				const auto dot = velocity.dot_product ( t.m_plane.m_normal );

				if ( dot < 0.0f )
					velocity -= vec3_t ( dot * t.m_plane.m_normal.x, dot * t.m_plane.m_normal.y, dot * t.m_plane.m_normal.z );

				end = t.m_endpos + velocity * csgo::ticks2time ( 1.0f - t.m_fraction );

				r.init ( t.m_endpos, end, player->mins ( ), player->maxs ( ) );

				csgo::util_tracehull ( t.m_endpos, end, player->mins ( ), player->maxs ( ), 0x201400B, player, &t );

				if ( t.m_fraction == 1.0f )
					break;
			}
		}

		origin = end = t.m_endpos;

		end.z -= 2.0f;

		csgo::util_tracehull ( origin, end, player->mins ( ), player->maxs ( ), 0x201400B, player, &t );

		flags &= 1;

		if ( t.did_hit ( ) && t.m_plane.m_normal.z > 0.7f )
			flags |= 1;
	};

	auto ticks_since = csgo::time2ticks( prediction::predicted_curtime ) - data::all_records [ pl->idx ( ) ][ N ( 0 ) ].m_tick;

	static std::ptrdiff_t off_ping = netvars::get_offset ( _ ( "DT_CSPlayerResource->m_iPing" ) );
	auto pl_ping = 0;

	const auto player_rsc = *reinterpret_cast< uintptr_t* > ( player_rsc_ptr );

	if ( player_rsc )
		pl_ping = *reinterpret_cast< int* > ( player_rsc + off_ping + pl->idx ( ) * sizeof ( int ) );

	auto dtick = ticks_since;
	auto dt = csgo::ticks2time( dtick );

	if ( !dtick )
		return false;

	if ( data::all_records [ pl->idx( ) ].size( ) < 2 ) {
		const auto old_origin = rec.m_origin;
		rec.m_origin += rec.m_vel * dt;

		for ( auto& bone : rec.m_bones )
			bone.set_origin ( bone.origin ( ) - old_origin + rec.m_origin );

		return false;
	}

	auto state = pl->animstate( );
	auto extrap_rec = data::all_records [ pl->idx( ) ][ N( 0 ) ];
	auto previous_rec = data::all_records [ pl->idx ( ) ][ N ( 1 ) ];
	auto vel_per = vec3_t ( 0.0f, 0.0f, 0.0f );
	auto second_vel_per = vec3_t ( 0.0f, 0.0f, 0.0f );
	auto vel = vec3_t ( 0.0f, 0.0f, 0.0f );
	auto second_vel = vec3_t ( 0.0f, 0.0f, 0.0f );
	auto predicted_accel = vec3_t ( 0.0f, 0.0f, 0.0f );

	/* extrapolate with more information... if we have it */
	vel = extrap_rec.m_vel;
	vel_per = vel / static_cast< float > ( extrap_rec.m_simtime - previous_rec.m_simtime );

	if ( data::all_records [ pl->idx ( ) ].size ( ) >= 3 ) {
		auto second_previous_rec = data::all_records [ pl->idx ( ) ][ N ( 2 ) ];
		second_vel = previous_rec.m_vel;
		predicted_accel = ( vel - second_vel ) / ( extrap_rec.m_simtime - previous_rec.m_simtime );
		second_vel_per = second_vel / ( previous_rec.m_simtime - second_previous_rec.m_simtime );
	}

	auto vel_deg = csgo::vec_angle( vel ).y;
	auto vel_ang = 0.0f;

	if ( data::all_records [ pl->idx ( ) ].size ( ) >= 3 )
		vel_ang = csgo::normalize( vel_deg - csgo::vec_angle( second_vel ).y );

	auto vel_ang_per_tick = vel_ang / ( extrap_rec.m_simtime - previous_rec.m_simtime );
	auto last_flags = rec.m_flags;
	auto pred_vel_magnitude = vel_per;
	auto cur_vel = vel;

	/* too much curvature - we will probably miss due to prediction (don't try to predict) */
	//if ( std::fabsf ( vel_ang_per_tick ) > 5.5f )
	//	return false;

	const auto delta_time = csgo::ticks2time( csgo::time2ticks ( nci->get_latency ( 0 ) + nci->get_latency ( 1 ) ) + csgo::time2ticks( prediction::predicted_curtime ) + 1 ) - extrap_rec.m_simtime;
	const auto simtime_delta = extrap_rec.m_simtime - previous_rec.m_simtime;
	const auto simulation_ticks = std::clamp < int > ( csgo::time2ticks ( simtime_delta ), 0, 17 );
	const auto magnitude_delta = ( ( pl->vel ( ).length_2d ( ) * animations::data::overlays [ pl->idx ( ) ][ 6 ].m_weight ) - vel.length_2d ( ) ) / csgo::ticks2time( dtick );

	auto cur_magnitude = vel.length_2d( );

	for ( auto tick = 0; tick < dtick; tick++ ) {
		const auto extrap_vel_yaw = vel_deg + vel_ang_per_tick * csgo::ticks2time ( 1 );

		if ( rec.m_flags & 1 && pl->flags( ) & 1 )
			cur_magnitude += magnitude_delta * csgo::ticks2time ( 1 );
		else if (!( pl->flags ( ) & 1 ))
			cur_magnitude += predicted_accel.length_2d( ) * csgo::ticks2time ( 1 );

		rec.m_vel.x = std::cosf ( csgo::deg2rad ( extrap_vel_yaw ) ) * cur_magnitude;
		rec.m_vel.y = std::sinf ( csgo::deg2rad ( extrap_vel_yaw ) ) * cur_magnitude;

		extrapolate ( pl, rec.m_origin, rec.m_vel, rec.m_flags, last_flags & 1 );

		last_flags = rec.m_flags;
		vel_deg = extrap_vel_yaw;
		cur_magnitude = rec.m_vel.length_2d ( );
	}

	for ( auto& bone : rec.m_bones )
		bone.set_origin ( bone.origin ( ) - extrap_rec.m_origin + rec.m_origin );

	rec.m_tick += dtick;
	rec.m_simtime += csgo::ticks2time( dtick );
	rec.m_lc = true;

	/* make sure to prioritize set data */
	//if ( angle != vec3_t( 0.0f, 0.0f, 0.0f ) )
	//	rec.m_priority = 2;

	return true;
}

void features::lagcomp::cache_shot( event_t* event ) {
	auto shooter = csgo::i::ent_list->get< player_t* >( csgo::i::engine->get_player_for_userid( event->get_int( _( "userid" ) ) ) );

	if ( !shooter->valid( ) || shooter == g::local || data::all_records [ shooter->idx( ) ].empty( ) || !g::local->alive( ) || !shooter->layers( ) || !shooter->animstate( ) || !shooter->bone_cache( ) )
		return;

	auto angle_to = csgo::calc_angle( shooter->origin( ) + shooter->view_offset( ), g::local->origin( ) + g::local->view_offset( ) );
	csgo::clamp( angle_to );

	//extrapolate_record( shooter, data::shot_records [ shooter->idx( ) ], angle_to );
}

template < typename t >
t lerp ( const t& t1, const t& t2, float progress ) {
	return t1 + ( t2 - t1 ) * progress;
}

void features::lagcomp::cache( player_t* pl ) {
	if ( pl && pl->dormant( ) ) {
		if ( !data::records [ pl->idx( ) ].empty( ) )
			data::records [ pl->idx( ) ].clear( );

		if ( !data::all_records [ pl->idx( ) ].empty( ) )
			data::all_records [ pl->idx( ) ].clear( );

		data::shot_records [ pl->idx( ) ].m_pl = nullptr;
		
		if ( !data::extrapolated_records [ pl->idx ( ) ].empty ( ) )
			data::extrapolated_records [ pl->idx ( ) ].clear ( );

		return;
	}

	if ( !pl->valid( ) || !pl->weapon ( ) || pl->team ( ) == g::local->team ( ) )
		return;

	/* store simulated information seperately */
	if ( !data::all_records [ pl->idx ( ) ].empty( ) ) {
		lag_record_t rec_simulated = data::all_records [ pl->idx ( ) ][ 0 ];
	
		if ( extrapolate_record ( pl, rec_simulated ) )
			data::extrapolated_records [ pl->idx ( ) ].push_front ( rec_simulated );
	}

	OPTION ( bool, fix_fakelag, "Sesame->A->Rage Aimbot->Accuracy->Fix Fakelag", oxui::object_checkbox );

	/* interpolate between records */ {
		const auto nci = csgo::i::engine->get_net_channel_info ( );
	
		if ( nci && data::records [ pl->idx ( ) ].size ( ) >= 2 ) {
			data::cham_records [ pl->idx ( ) ] = data::records [ pl->idx ( ) ].back ( );

			const auto& current = data::records [ pl->idx ( ) ][ data::records [ pl->idx ( ) ].size ( ) - 1 ];
			const auto& last_to_current = data::records [ pl->idx ( ) ][ data::records [ pl->idx ( ) ].size ( ) - 2 ];
			const auto delta_pos = last_to_current.m_origin - current.m_origin;
			const auto delta_time = last_to_current.m_simtime - current.m_simtime;
			const auto delta_lag = csgo::i::globals->m_curtime - pl->simtime ( );
			const auto lerp_time = std::clamp ( delta_lag / delta_time, 0.0f, 1.0f );
			const auto lerped_delta_pos = delta_pos * lerp_time;

			for ( auto& bone : data::cham_records [ pl->idx ( ) ].m_bones )
				bone.set_origin ( bone.origin ( ) + lerped_delta_pos );
		}
	}

	if ( pl->simtime ( ) <= pl->old_simtime ( ) && fix_fakelag )
		return;

	lag_record_t rec;

	if ( rec.store( pl, data::old_origins [ pl->idx( ) ] ) ) {
		///* simulate 1 tick of movement */ {
		//	const auto backup_origin = rec.m_origin;
		//
		//	rec.extrapolate ( );
		//
		//	for ( auto& bone : rec.m_bones )
		//		bone.set_origin ( bone.origin ( ) - backup_origin + rec.m_origin );
		//}

		if ( std::fabsf( pl->angles ( ).x ) < 50.0f && pl->weapon ( )->last_shot_time ( ) > pl->old_simtime ( ) ) {
		//if ( animations::data::overlays [ pl->idx ( ) ][ 1 ].m_weight < 0.1f && pl->weapon ( )->last_shot_time ( ) > pl->old_simtime ( ) ) {
			data::shot_records [ pl->idx ( ) ] = rec;
			data::shot_count [ pl->idx ( ) ]++;
			data::shot_records [ pl->idx ( ) ].m_priority = 1;
			//data::shot_records [ pl->idx ( ) ].m_simtime = pl->weapon ( )->last_shot_time ( );
		}

		data::all_records [ pl->idx( ) ].push_front( rec );
		data::records [ pl->idx( ) ].push_front( rec );
	}

	data::old_origins [ pl->idx( ) ] = pl->origin( );
}

void features::lagcomp::pop( player_t* pl ) {
	if ( !pl->valid ( ) ) {
		if ( !data::records [ pl->idx ( ) ].empty ( ) )
			data::records [ pl->idx ( ) ].clear ( );

		data::cham_records [ pl->idx ( ) ].m_pl = nullptr;

		if ( !data::all_records [ pl->idx ( ) ].empty ( ) )
			data::all_records [ pl->idx ( ) ].clear ( );

		data::shot_records [ pl->idx ( ) ].m_pl = nullptr;

		if ( !data::extrapolated_records [ pl->idx ( ) ].empty ( ) )
			data::extrapolated_records [ pl->idx ( ) ].clear ( );

		return;
	}

	while ( !data::records [ pl->idx ( ) ].empty ( ) && !data::records [ pl->idx ( ) ].back ( ).valid ( ) )
		data::records [ pl->idx ( ) ].pop_back ( );

	while ( !data::all_records [ pl->idx ( ) ].empty ( ) && data::all_records [ pl->idx ( ) ].size ( ) > 12 )
		data::all_records [ pl->idx ( ) ].pop_back ( );

	if ( !data::shot_records [ pl->idx ( ) ].valid ( ) )
		data::shot_records [ pl->idx ( ) ].m_pl = nullptr;

	if ( !data::cham_records [ pl->idx ( ) ].valid ( ) )
		data::cham_records [ pl->idx ( ) ].m_pl = nullptr;

	while ( !data::extrapolated_records [ pl->idx ( ) ].empty ( ) && !data::extrapolated_records [ pl->idx ( ) ].back().valid ( ) )
		data::extrapolated_records [ pl->idx ( ) ].pop_back ( );
}

bool features::lagcomp::breaking_lc( player_t* pl ) {
	if ( !pl->valid( ) )
		return false;

	const auto& recs = get( pl );

	if ( !recs.second )
		return false;

	return !recs.first.front( ).m_lc;
}

bool features::lagcomp::has_onshot( player_t* pl ) {
	if ( !pl->valid( ) )
		return false;

	//if ( data::shot_records [ pl->idx ( ) ].m_pl )
	//	dbg_print ( "HAS ONSHOT\n" );

	return data::shot_records [ pl->idx( ) ].m_pl;
}