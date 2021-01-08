#include "lagcomp.hpp"
#include <deque>
#include "autowall.hpp"
#include "../menu/menu.hpp"
#include "../globals.hpp"
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
	auto ud_rate = static_cast<float>( g::cvars::cl_updaterate->get_int ( ) );

	if ( g::cvars::sv_minupdaterate && g::cvars::sv_maxupdaterate )
		ud_rate = static_cast< float >( g::cvars::sv_maxupdaterate->get_int ( ));

	auto ratio = g::cvars::cl_interp_ratio->get_float();

	if ( ratio == 0 )
		ratio = 1.0f;

	if ( g::cvars::sv_client_min_interp_ratio && g::cvars::sv_client_max_interp_ratio && g::cvars::sv_client_min_interp_ratio->get_float ( ) != 1 )
		ratio = std::clamp ( ratio, g::cvars::sv_client_min_interp_ratio->get_float ( ), g::cvars::sv_client_max_interp_ratio->get_float ( ) );

	return std::max<float> ( g::cvars::cl_interp->get_float ( ), ratio / ud_rate );
};

const std::pair< std::deque< features::lagcomp::lag_record_t >&, bool > features::lagcomp::get( player_t* pl ) {
	return { data::records [ pl->idx( ) ], !data::records [ pl->idx( ) ].empty( ) };
}

const std::pair< std::deque< features::lagcomp::lag_record_t >&, bool > features::lagcomp::get_all( player_t* pl ) {
	return { data::all_records [ pl->idx( ) ], !data::all_records [ pl->idx( ) ].empty( ) };
}

const std::pair< features::lagcomp::lag_record_t&, bool > features::lagcomp::get_extrapolated( player_t* pl ) {
	return { data::extrapolated_records [ pl->idx( ) ][ 0 ], !data::extrapolated_records [ pl->idx( ) ].empty( ) && data::extrapolated_records [ pl->idx( ) ][ 0 ].m_pl && !data::extrapolated_records [ pl->idx( ) ][ 0 ].m_needs_matrix_construction };
}

const std::pair< features::lagcomp::lag_record_t&, bool > features::lagcomp::get_shot( player_t* pl ) {
	return { data::shot_records [ pl->idx( ) ], data::shot_records [ pl->idx( ) ].m_pl };
}

bool features::lagcomp::get_render_record ( player_t* ent, matrix3x4_t* out ) {
	auto tick_valid_stripped = [ ] ( float t ) {
		const auto nci = cs::i::engine->get_net_channel_info ( );

		if ( !nci || !g::local )
			return false;

		const auto correct = std::clamp ( nci->get_latency ( 0 )
			+ nci->get_latency ( 1 )
			+ lerp ( ), 0.0f, g::cvars::sv_maxunlag->get_float ( ) );

		return abs ( correct - ( cs::i::globals->m_curtime - t ) ) <= 0.2f;
	};

	const auto nci = cs::i::engine->get_net_channel_info ( );

	auto idx = ent->idx ( );

	if ( !idx || idx > cs::i::globals->m_max_clients )
		return false;

	auto& data = data::all_records [ idx ];

	if ( data.empty ( ) )
		return false;

	const auto misses = ragebot::get_misses ( idx ).bad_resolve;

	for ( int i = static_cast<int>(data.size ( )) - 1; i >= 0; i-- ) {
		auto& it = data[i];

		if ( tick_valid_stripped ( it.m_simtime ) ) {
			if ( it.m_origin.dist_to ( ent->origin ( ) ) < 1.0f )
				return false;

			bool end = ( i - 1 ) <= 0;
			vec3_t next = end ? ent->origin ( ) : data [ i - 1 ].m_origin;
			float time_next = end ? ent->simtime ( ) : data [ i - 1 ].m_simtime;

			float correct = nci->get_avg_latency ( 0 )+ nci->get_avg_latency ( 1 ) + lerp ( );
			float time_delta = time_next - it.m_simtime;
			float add = end ? 0.2f : time_delta;
			float deadtime = it.m_simtime + correct + add;

			float curtime = cs::i::globals->m_curtime;
			float delta = deadtime - curtime;

			float mul = 1.0f / add;

			vec3_t lerp = next + ( it.m_origin - next ) * std::clamp ( delta * mul, 0.0f, 1.0f );

			static matrix3x4_t ret[128];

			if ( misses % 3 == 0 )
				memcpy ( ret, it.m_bones1, sizeof ( ret ) );
			else if ( misses % 3 == 1 )
				memcpy ( ret, it.m_bones2, sizeof ( ret ) );
			else
				memcpy ( ret, it.m_bones3, sizeof ( ret ) );

			for ( auto& iter : ret )
				iter.set_origin ( iter.origin ( ) - it.m_origin + lerp );

			memcpy ( out,
				ret,
				sizeof ( ret ) );

			return true;
		}
	}

	return false;
}

void features::lagcomp::lag_record_t::backtrack( ucmd_t* ucmd ) {
	//if ( ragebot::active_config.fix_fakelag )
	//dbg_print( "1: %d\n", ucmd->m_tickcount );
	ucmd->m_tickcount = cs::time2ticks( m_simtime ) + cs::time2ticks( lerp( ) );
	//dbg_print( "2: %d\n", ucmd->m_tickcount );
}

bool features::lagcomp::lag_record_t::store( player_t* pl, const vec3_t& last_origin, bool simulated ) {
	m_pl = pl;

	if ( !pl->layers( ) || !pl->bone_cache( ) || !pl->animstate( ) || !g::local )
		return false;

	m_priority = 0;
	m_extrapolated = simulated;
	m_needs_matrix_construction = false;
	m_tick = cs::time2ticks( pl->simtime( ) );
	m_simtime = pl->simtime( );
	m_flags = pl->flags( );
	m_ang = pl->angles( );
	m_origin = pl->origin( );
	m_min = pl->mins( );
	m_max = pl->maxs( );
	m_vel = pl->vel( );
	m_lc = pl->origin( ).dist_to_sqr( last_origin ) <= 4096.0f;
	m_failed_resolves = features::ragebot::get_misses( pl->idx( ) ).bad_resolve;
	m_lby = pl->lby( );
	m_abs_yaw = pl->abs_angles( ).y;
	m_unresolved_abs = pl->abs_angles( ).y;

	/*
	@CBRs
		So here's something to think about. Besides all the storage you do for resolving & animation fixing stuff, in reality you only need to store 5 things.
		Mins, Maxs, Origin, player time, and bone data
		Oh also, cached bone data is a cutlvector and the length is set to the model's bonecount. it never changes, so you never have to store bone count (and you don't where many do, yay!)
	*/

	std::memcpy( m_layers, pl->layers( ), sizeof( m_layers ) );
	std::memcpy( m_bones1, anims::players::matricies [ pl->idx( ) ][0].data( ), sizeof ( m_bones1 ) );
	std::memcpy( m_bones2, anims::players::matricies [ pl->idx( ) ][1].data( ), sizeof ( m_bones2 ) );
	std::memcpy( m_bones3, anims::players::matricies [ pl->idx( ) ][2].data( ), sizeof ( m_bones3 ) );
	std::memcpy( &m_state, pl->animstate( ), sizeof( m_state ) );
	std::memcpy( m_poses, pl->poses( ).data(), sizeof( m_poses ) );

	return true;
}

void features::lagcomp::cache( player_t* pl, bool predicted ) {
	if ( !pl->valid ( ) )
		return;

	if ( !pl->valid( ) || !pl->weapon( ) || pl->team( ) == g::local->team( ) )
		return;

	static std::array<float, 65> old_pitch { 0.0f };
	static std::array<float, 65> old_simtimes { 0.0f };

	/*
		@CBRS Great, you won't shoot at bad records!
	*/
	/*if ( pl->simtime( ) <= pl->old_simtime( ) )
		return;*/

	lag_record_t rec;

	if ( rec.store( pl, data::old_origins [ pl->idx( ) ], predicted ) ) {
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

		if ( !predicted ) {
			if ( pl->animstate ( ) && abs( pl->animstate ( )->m_pitch ) < 60.0f ) {
				rec.m_priority = 1;

				data::shot_records [ pl->idx( ) ] = rec;
				data::shot_count [ pl->idx( ) ]++;
				data::shot_records [ pl->idx( ) ].m_priority = 1;
			}

			data::all_records [ pl->idx( ) ].push_front( rec );
		}

		data::records [ pl->idx( ) ].push_front( rec );
	}

	if ( !predicted )
		data::old_origins [ pl->idx( ) ] = pl->origin( );

	pop( pl );

	old_pitch [ pl->idx ( ) ] = pl->angles ( ).x;
	old_simtimes [ pl->idx ( ) ] = pl->simtime ( );
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