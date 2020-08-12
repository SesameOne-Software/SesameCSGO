#include "lagcomp.hpp"
#include <deque>
#include "autowall.hpp"
#include "../menu/menu.hpp"
#include "../globals.hpp"
#include "../animations/animations.hpp"
#include "prediction.hpp"
#include "../animations/resolver.hpp"
#include "ragebot.hpp"

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

void features::lagcomp::lag_record_t::backtrack ( ucmd_t* ucmd ) {
	if ( ragebot::active_config.fix_fakelag )
		ucmd->m_tickcount = csgo::time2ticks ( m_simtime ) + csgo::time2ticks( lerp ( ) );
}

bool features::lagcomp::lag_record_t::store ( player_t* pl, const vec3_t& last_origin, bool simulated ) {
	m_pl = pl;

	if ( !pl->layers ( ) || !pl->bone_cache ( ) || !pl->animstate ( ) || !g::local )
		return false;

	m_priority = 0;
	m_extrapolated = false;
	m_needs_matrix_construction = false;
	m_tick = csgo::time2ticks ( pl->simtime ( ) );
	m_simtime = pl->simtime ( );
	m_flags = pl->flags ( );
	m_ang = pl->angles ( );
	m_origin = pl->origin ( );
	m_min = pl->mins ( );
	m_max = pl->maxs ( );
	m_vel = pl->vel ( );
	m_lc = pl->origin ( ).dist_to_sqr ( last_origin ) <= 4096.0f;
	m_failed_resolves = features::ragebot::get_misses ( pl->idx ( ) ).bad_resolve;
	m_lby = pl->lby ( );
	m_abs_yaw = pl->abs_angles ( ).y;
	m_unresolved_abs = animations::data::old_abs [ pl->idx ( ) ];

	/*
	@CBRs
		So here's something to think about. Besides all the storage you do for resolving & animation fixing stuff, in reality you only need to store 5 things.
		Mins, Maxs, Origin, player time, and bone data
		Oh also, cached bone data is a cutlvector and the length is set to the model's bonecount. it never changes, so you never have to store bone count (and you don't where many do, yay!)
	*/

	std::memcpy ( m_layers, pl->layers ( ), sizeof animlayer_t * 15 );
	std::memcpy ( m_bones1, &animations::data::fixed_bones [ pl->idx ( ) ], sizeof matrix3x4_t * 128 );
	std::memcpy ( m_bones2, &animations::data::fixed_bones1 [ pl->idx ( ) ], sizeof matrix3x4_t * 128 );
	std::memcpy ( m_bones3, &animations::data::fixed_bones2 [ pl->idx ( ) ], sizeof matrix3x4_t * 128 );
	std::memcpy ( &m_state, pl->animstate ( ), sizeof ( animstate_t ) );
	std::memcpy ( m_poses, &pl->poses ( ), sizeof ( float ) * 24 );

	return true;
}

bool features::lagcomp::extrapolate_record( player_t* pl, lag_record_t& rec, bool shot ) {
	static auto player_rsc_ptr = pattern::search ( _ ( "client.dll" ), _ ( "8B 3D ? ? ? ? 85 FF 0F 84 ? ? ? ? 81 C7" ) ).add ( 2 ).deref ( ).get< uintptr_t > ( );

	auto nci = csgo::i::engine->get_net_channel_info ( );

	if ( !nci
		|| data::all_records [ pl->idx( ) ].empty( )
		|| pl->dormant( )
		|| !pl->animstate( ) 
		|| !pl->bone_cache()
		|| !pl->layers( )
		|| data::all_records [ pl->idx ( ) ].size ( ) < 2 )
		return false;

	//const auto simulation_tick_delta = std::clamp( csgo::time2ticks ( pl->simtime( ) - pl->old_simtime( ) ), 0, 17 );
	//auto delta_ticks = ( std::clamp ( csgo::time2ticks( nci->get_latency ( 1 ) + nci->get_latency ( 0 ) ) + csgo::time2ticks( prediction::predicted_curtime ) - csgo::time2ticks ( pl->simtime ( ) + lerp ( ) ), 0, 100 ) ) - simulation_tick_delta;

	if ( /*delta_ticks <= 0 || simulation_tick_delta <= 0 ||*/ data::all_records [ pl->idx ( ) ].size ( ) < 2 )
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

		for ( auto i = 0; i < 128; i++ ) {
			rec.m_bones1 [ i ].set_origin ( rec.m_bones1 [ i ].origin ( ) - old_origin + rec.m_origin );
			rec.m_bones2 [ i ].set_origin ( rec.m_bones2 [ i ].origin ( ) - old_origin + rec.m_origin );
			rec.m_bones3 [ i ].set_origin ( rec.m_bones3 [ i ].origin ( ) - old_origin + rec.m_origin );
		}

		rec.m_simtime = pl->simtime ( ) + csgo::ticks2time ( dtick );
		rec.m_tick = csgo::time2ticks ( rec.m_simtime );

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

		if ( rec.m_flags & 1 && pl->flags ( ) & 1 )
			cur_magnitude += magnitude_delta * csgo::ticks2time ( 1 );
		else if ( !( pl->flags ( ) & 1 ) )
			cur_magnitude += predicted_accel.length_2d ( ) * csgo::ticks2time ( 1 );

		rec.m_vel.x = std::cosf ( csgo::deg2rad ( extrap_vel_yaw ) ) * cur_magnitude;
		rec.m_vel.y = std::sinf ( csgo::deg2rad ( extrap_vel_yaw ) ) * cur_magnitude;

		extrapolate ( pl, rec.m_origin, rec.m_vel, rec.m_flags, pl->flags ( ) & 1 );

		vel_deg = extrap_vel_yaw;
		cur_magnitude = rec.m_vel.length_2d ( );
	}

	rec.m_simtime = pl->simtime ( ) + csgo::ticks2time ( dtick );
	rec.m_tick = csgo::time2ticks ( rec.m_simtime );

	for ( auto i = 0; i < 128; i++ ) {
		rec.m_bones1 [ i ].set_origin ( rec.m_bones1 [ i ].origin ( ) - extrap_rec.m_origin + rec.m_origin );
		rec.m_bones2 [ i ].set_origin ( rec.m_bones2 [ i ].origin ( ) - extrap_rec.m_origin + rec.m_origin );
		rec.m_bones3 [ i ].set_origin ( rec.m_bones3 [ i ].origin ( ) - extrap_rec.m_origin + rec.m_origin );
	}

	rec.m_lc = true;
	rec.m_extrapolated = true;

	/* make sure to prioritize set data */
	//if ( angle != vec3_t( 0.0f, 0.0f, 0.0f ) )
	//	rec.m_priority = 2;

	return true;
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
	//if ( !data::all_records [ pl->idx ( ) ].empty( ) ) {
	//	lag_record_t rec_simulated = data::all_records [ pl->idx ( ) ][ 0 ];
	//
	//	if ( extrapolate_record ( pl, rec_simulated, false ) )
	//		data::extrapolated_records [ pl->idx ( ) ].push_front ( rec_simulated );
	//}

	/* interpolate between records */ {
		const auto nci = csgo::i::engine->get_net_channel_info ( );
	
		if ( nci && data::all_records [ pl->idx ( ) ].size ( ) >= 2 ) {
			data::cham_records [ pl->idx ( ) ] = data::all_records [ pl->idx ( ) ].front ( );
	
			/* finding last valid visible record */
			auto visible_idx = 0;

			for ( auto i = 0; i < data::all_records [ pl->idx ( ) ].size ( ); i++ ) {
				if ( data::all_records [ pl->idx ( ) ][ i ].valid ( ) )
					visible_idx = i;
			}

			if ( visible_idx && visible_idx + 1 < data::all_records [ pl->idx ( ) ].size ( ) ) {
				const auto itp1 = &data::all_records [ pl->idx ( ) ][ visible_idx ];
				const auto it = &data::all_records [ pl->idx ( ) ][ visible_idx + 1 ];

				data::cham_records [ pl->idx ( ) ] = *it;

				const auto correct = nci->get_latency ( 0 ) + nci->get_latency ( 1 ) + lerp ( ) - ( it->m_simtime - ( itp1 )->m_simtime );
				const auto time_delta = ( itp1 )->m_simtime - it->m_simtime;
				const auto add = time_delta;
				const auto deadtime = it->m_simtime + correct + add;
				const auto curtime = csgo::i::globals->m_curtime;
				const auto delta = deadtime - curtime;
				const auto mul = 1.0f / add;
				
				const auto lerp_fraction = std::clamp ( delta * mul, 0.0f, 1.0f );

				vec3_t lerp = vec3_t ( std::lerp ( ( itp1 )->m_origin.x, it->m_origin.x, lerp_fraction ),
					std::lerp ( ( itp1 )->m_origin.y, it->m_origin.y, lerp_fraction ),
					std::lerp ( ( itp1 )->m_origin.z, it->m_origin.z, lerp_fraction ) );

				matrix3x4_t ret [ 128 ];

				/*if ( (it->m_bones1 [ 1 ].origin ( ) + it->m_origin + lerp ).dist_to ( pl->abs_origin ( ) ) >= 7.5f )*/ {
					memcpy ( ret, it->m_bones1, sizeof ( matrix3x4_t ) * 128 );

					for ( auto i = 0; i < 128; i++ ) {
						const vec3_t matrix_delta = it->m_bones1 [ i ].origin ( ) - it->m_origin;
						it->m_bones1 [ i ].set_origin ( matrix_delta + lerp );
					}

					memcpy ( &data::cham_records [ pl->idx ( ) ].m_bones1, ret, sizeof ( matrix3x4_t ) * 128 );
				}

				/*if ( (it->m_bones2 [ 1 ].origin ( ) + it->m_origin + lerp ).dist_to ( pl->abs_origin ( ) ) >= 7.5f )*/ {
					memcpy ( ret, it->m_bones2, sizeof ( matrix3x4_t ) * 128 );

					for ( auto i = 0; i < 128; i++ ) {
						const vec3_t matrix_delta = it->m_bones2 [ i ].origin ( ) - it->m_origin;
						it->m_bones2 [ i ].set_origin ( matrix_delta + lerp );
					}

					memcpy ( &data::cham_records [ pl->idx ( ) ].m_bones2, ret, sizeof ( matrix3x4_t ) * 128 );
				}

				/*if ( (it->m_bones3 [ 1 ].origin ( ) + it->m_origin + lerp ).dist_to ( pl->abs_origin ( ) ) >= 7.5f )*/ {
					memcpy ( ret, it->m_bones3, sizeof ( matrix3x4_t ) * 128 );

					for ( auto i = 0; i < 128; i++ ) {
						const vec3_t matrix_delta = it->m_bones3 [ i ].origin ( ) - it->m_origin;
						it->m_bones3 [ i ].set_origin ( matrix_delta + lerp );
					}

					memcpy ( &data::cham_records [ pl->idx ( ) ].m_bones3, ret, sizeof ( matrix3x4_t ) * 128 );
				}
			}
		}
	}

	/*
		@CBRS Great, you won't shoot at bad records!
	*/
	if ( pl->simtime ( ) <= pl->old_simtime ( ) )
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


		/*
			@CBRS Ok, so here's the thing: csgo only pops lag records when new player data has arrived, and thus we should only actually pop records in here.
			additionally, they cast the "is it dead yet" to an integer, which means some pretty funky stuff happens

			if the record arrived @ time 15.19, then technically all records between time 14-15.19 are stored on the server
			but if it arrives at 15.21, then only between 15-15.21 are stored. whack, right?

			anyways, here's some code for popping records correctly.

			static auto max_unlag = cs::cvars->find_var( "sv_maxunlag" );
			int dead_time = player->sim( ) - max_unlag->get_float( );
			while ( track.size( ) > 1 ) {	//	technically this isn't proper, but it's so there's always a record for me to shoot @
				if ( track.back( ).sim >= dead_time )
					break;

				track.pop_back( );
			}
		*/

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

bool features::lagcomp::breaking_lc( player_t* pl ) {
	if ( !pl->valid( ) )
		return false;

	/*
	@CBRs
		walk every event in the deque (this is a big reason why we need to pop correctly) and check if the origin difference .lengthsqr is > 64^2
		if so, we can't backtrack
		so therefore, we have to shoot at where the server last set their bones up, otherwise we miss.
		technically, extrapolation works. but if ur smart and the player has low ping, then just only shoot soon after receiving data - aka, their data is correct because
		the server won't have received another batch of clcmsgmove packets
	*/

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