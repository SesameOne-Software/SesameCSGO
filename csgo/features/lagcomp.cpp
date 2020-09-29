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

void features::lagcomp::lag_record_t::backtrack( ucmd_t* ucmd ) {
	//if ( ragebot::active_config.fix_fakelag )
	//dbg_print( "1: %d\n", ucmd->m_tickcount );
	ucmd->m_tickcount = csgo::time2ticks( m_simtime ) + csgo::time2ticks( lerp( ) );
	//dbg_print( "2: %d\n", ucmd->m_tickcount );
}

bool features::lagcomp::lag_record_t::store( player_t* pl, const vec3_t& last_origin, bool simulated ) {
	m_pl = pl;

	if ( !pl->layers( ) || !pl->bone_cache( ) || !pl->animstate( ) || !g::local || anims::frames [ pl->idx( ) ].empty( ) )
		return false;

	m_priority = 0;
	m_extrapolated = simulated;
	m_needs_matrix_construction = false;
	m_tick = csgo::time2ticks( pl->simtime( ) );
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

	std::memcpy( m_layers, pl->layers( ), sizeof animlayer_t * 15 );
	std::memcpy( m_bones1, anims::frames [ pl->idx( ) ].back( ).m_matrix1.data( ), sizeof matrix3x4_t * 128 );
	std::memcpy( m_bones2, anims::frames [ pl->idx( ) ].back( ).m_matrix2.data( ), sizeof matrix3x4_t * 128 );
	std::memcpy( m_bones3, anims::frames [ pl->idx( ) ].back( ).m_matrix3.data( ), sizeof matrix3x4_t * 128 );
	std::memcpy( &m_state, pl->animstate( ), sizeof( animstate_t ) );
	std::memcpy( m_poses, &pl->poses( ), sizeof( float ) * 24 );

	return true;
}

void features::lagcomp::cache( player_t* pl, bool predicted ) {
	if ( pl && pl->dormant( ) ) {
		if ( !data::records [ pl->idx( ) ].empty( ) )
			data::records [ pl->idx( ) ].clear( );

		if ( !data::all_records [ pl->idx( ) ].empty( ) )
			data::all_records [ pl->idx( ) ].clear( );

		data::shot_records [ pl->idx( ) ].m_pl = nullptr;

		if ( !data::extrapolated_records [ pl->idx( ) ].empty( ) )
			data::extrapolated_records [ pl->idx( ) ].clear( );

		return;
	}

	if ( !pl->valid( ) || !pl->weapon( ) || pl->team( ) == g::local->team( ) )
		return;

	/* store simulated information seperately */
	//if ( !data::all_records [ pl->idx ( ) ].empty( ) ) {
	//	lag_record_t rec_simulated = data::all_records [ pl->idx ( ) ][ 0 ];
	//
	//	if ( extrapolate_record ( pl, rec_simulated, false ) )
	//		data::extrapolated_records [ pl->idx ( ) ].push_front ( rec_simulated );
	//}

	///* interpolate between records */ {
	//	const auto nci = csgo::i::engine->get_net_channel_info( );
//
	//	if ( nci && data::all_records [ pl->idx( ) ].size( ) >= 2 ) {
	//		data::cham_records [ pl->idx( ) ] = data::all_records [ pl->idx( ) ].front( );
//
	//		/* finding last valid visible record */
	//		auto visible_idx = 0;
//
	//		for ( auto i = 0; i < data::all_records [ pl->idx( ) ].size( ); i++ ) {
	//			if ( data::all_records [ pl->idx( ) ][ i ].valid( ) )
	//				visible_idx = i;
	//		}
//
	//		if ( visible_idx && visible_idx + 1 < data::all_records [ pl->idx( ) ].size( ) ) {
	//			const auto itp1 = &data::all_records [ pl->idx( ) ][ visible_idx ];
	//			const auto it = &data::all_records [ pl->idx( ) ][ visible_idx + 1 ];
//
	//			data::cham_records [ pl->idx( ) ] = *it;
//
	//			const auto correct = nci->get_latency( 0 ) + nci->get_latency( 1 ) + lerp( ) - ( it->m_simtime - ( itp1 )->m_simtime );
	//			const auto time_delta = ( itp1 )->m_simtime - it->m_simtime;
	//			const auto add = time_delta;
	//			const auto deadtime = it->m_simtime + correct + add;
	//			const auto curtime = csgo::i::globals->m_curtime;
	//			const auto delta = deadtime - curtime;
	//			const auto mul = 1.0f / add;
//
	//			const auto lerp_fraction = std::clamp( delta * mul, 0.0f, 1.0f );
//
	//			static auto lerpf = [ ] ( float a, float b, float x ) {
	//				return a + ( b - a ) * x;
	//			};
//
	//			vec3_t lerp = vec3_t( lerpf( it->m_origin.x, ( itp1 )->m_origin.x, lerp_fraction ),
	//				lerpf( it->m_origin.y, ( itp1 )->m_origin.y, lerp_fraction ),
	//				lerpf( it->m_origin.z, ( itp1 )->m_origin.z, lerp_fraction ) );
//
	//			matrix3x4_t ret [ 128 ];
//
	//			/*if ( (it->m_bones1 [ 1 ].origin ( ) + it->m_origin + lerp ).dist_to ( pl->abs_origin ( ) ) >= 7.5f )*/ {
	//				memcpy( ret, it->m_bones1, sizeof( matrix3x4_t ) * 128 );
//
	//				for ( auto i = 0; i < 128; i++ ) {
	//					const vec3_t matrix_delta = it->m_bones1 [ i ].origin( ) - it->m_origin;
	//					it->m_bones1 [ i ].set_origin( matrix_delta + lerp );
	//				}
//
	//				memcpy( &data::cham_records [ pl->idx( ) ].m_bones1, ret, sizeof( matrix3x4_t ) * 128 );
	//			}
//
	//			/*if ( (it->m_bones2 [ 1 ].origin ( ) + it->m_origin + lerp ).dist_to ( pl->abs_origin ( ) ) >= 7.5f )*/ {
	//				memcpy( ret, it->m_bones2, sizeof( matrix3x4_t ) * 128 );
//
	//				for ( auto i = 0; i < 128; i++ ) {
	//					const vec3_t matrix_delta = it->m_bones2 [ i ].origin( ) - it->m_origin;
	//					it->m_bones2 [ i ].set_origin( matrix_delta + lerp );
	//				}
//
	//				memcpy( &data::cham_records [ pl->idx( ) ].m_bones2, ret, sizeof( matrix3x4_t ) * 128 );
	//			}
//
	//			/*if ( (it->m_bones3 [ 1 ].origin ( ) + it->m_origin + lerp ).dist_to ( pl->abs_origin ( ) ) >= 7.5f )*/ {
	//				memcpy( ret, it->m_bones3, sizeof( matrix3x4_t ) * 128 );
//
	//				for ( auto i = 0; i < 128; i++ ) {
	//					const vec3_t matrix_delta = it->m_bones3 [ i ].origin( ) - it->m_origin;
	//					it->m_bones3 [ i ].set_origin( matrix_delta + lerp );
	//				}
//
	//				memcpy( &data::cham_records [ pl->idx( ) ].m_bones3, ret, sizeof( matrix3x4_t ) * 128 );
	//			}
	//		}
	//	}
	//}

	/*
		@CBRS Great, you won't shoot at bad records!
	*/
	/*if ( pl->simtime( ) <= pl->old_simtime( ) )
		return;*/

	lag_record_t rec;

	if ( rec.store( pl, data::old_origins [ pl->idx( ) ], predicted ) ) {
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

		if ( !predicted ) {
			if ( std::fabsf( pl->angles( ).x ) < 50.0f && pl->layers ( ) && pl->layers ( )[ 1 ].m_weight < 0.1f ) {
				//if ( animations::data::overlays [ pl->idx ( ) ][ 1 ].m_weight < 0.1f && pl->weapon ( )->last_shot_time ( ) > pl->old_simtime ( ) ) {
				data::shot_records [ pl->idx( ) ] = rec;
				data::shot_count [ pl->idx( ) ]++;
				data::shot_records [ pl->idx( ) ].m_priority = 1;
				//data::shot_records [ pl->idx ( ) ].m_simtime = pl->weapon ( )->last_shot_time ( );
			}

			data::all_records [ pl->idx( ) ].push_front( rec );
		}

		data::records [ pl->idx( ) ].push_front( rec );
	}

	if ( !predicted )
		data::old_origins [ pl->idx( ) ] = pl->origin( );

	pop( pl );
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