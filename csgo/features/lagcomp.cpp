#include "lagcomp.hpp"
#include <deque>
#include "autowall.hpp"
#include "../menu/menu.hpp"
#include "../globals.hpp"
#include "prediction.hpp"
#include "../animations/resolver.hpp"
#include "ragebot.hpp"
#include "../animations/anims.hpp"

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

bool features::lagcomp::get_render_record ( player_t* ent, std::array<matrix3x4_t, 128>& out ) {
	const auto nci = cs::i::engine->get_net_channel_info ( );

	auto idx = ent->idx ( );

	if ( !idx || idx > cs::i::globals->m_max_clients )
		return false;

	auto& data = data::visual_records [ idx ];

	if ( data.empty ( ) )
		return false;

	const auto misses = ragebot::get_misses ( idx ).bad_resolve;

	for ( int i = static_cast<int>(data.size ( )) - 1; i >= 0; i-- ) {
		auto& it = data[i];

		if ( it.valid() ) {
			if ( it.m_origin.dist_to ( ent->origin ( ) ) < 1.0f )
				return false;

			bool end = ( i - 1 ) <= 0;
			vec3_t next = end ? ent->origin ( ) : data [ i - 1 ].m_origin;
			float time_next = end ? ent->simtime ( ) : data [ i - 1 ].m_simtime;

			float correct = nci->get_avg_latency ( 0 )+ nci->get_avg_latency ( 1 ) + lerp ( );
			float time_delta = time_next - it.m_simtime;
			float add = end ? 0.2f : time_delta;
			float deadtime = it.m_simtime + correct + add;

			float curtime = cs::ticks2time ( g::local->tick_base ( ) );
			float delta = deadtime - curtime;

			float mul = 1.0f / add;

			vec3_t lerp = next + ( it.m_origin - next ) * std::clamp ( delta * mul, 0.0f, 1.0f );

			static std::array<matrix3x4_t, 128> ret {};
			ret = it.m_render_bones;

			for ( auto& iter : ret )
				iter.set_origin ( iter.origin ( ) - it.m_origin + lerp );

			out = ret;

			return true;
		}
	}

	return false;
}

bool features::lagcomp::lag_record_t::store( player_t* pl, const vec3_t& last_origin ) {
	if ( !pl->bone_cache( ) || !pl->animstate( ) || !g::local )
		return false;

	m_idx = pl->idx ( );
	m_priority = 0;
	m_simtime = pl->simtime( );
	m_flags = pl->flags( );
	m_ang = pl->angles( );
	m_abs_origin = pl->abs_origin ( );
	m_origin = pl->origin( );
	m_min = pl->mins( );
	m_max = pl->maxs( );
	m_vel = pl->vel( );
	m_lc = pl->origin( ).dist_to_sqr( last_origin ) <= 4096.0f;
	m_lby = pl->lby( );
	m_state = *pl->animstate ( );
	m_layers = pl->layers ( );
	m_poses = pl->poses ( );
	m_aim_bones = anims::aim_matricies [ pl->idx ( ) ];
	memcpy ( m_render_bones.data ( ), pl->bone_cache ( ), sizeof ( matrix3x4_t ) * pl->bone_count ( ) );

	return true;
}

bool features::lagcomp::lag_record_t::apply ( player_t* pl ) {
	if ( !pl->bone_cache ( ) || !pl->animstate ( ) || !g::local )
		return false;

	pl->flags ( ) = m_flags;
	pl->angles ( ) = m_ang;
	pl->set_abs_origin ( m_abs_origin );
	pl->origin ( ) = m_origin;
	pl->mins ( ) = m_min;
	pl->maxs ( ) = m_max;
	pl->vel ( ) = m_vel;
	pl->lby ( ) = m_lby;
	*pl->animstate ( ) = m_state;
	pl->layers ( ) = m_layers;
	pl->poses ( ) = m_poses;

	return true;
}

std::deque< features::lagcomp::lag_record_t > features::lagcomp::get_records ( player_t* ent ) {
	std::deque< features::lagcomp::lag_record_t > ret {};

	for ( auto& rec : data::records [ ent->idx ( ) ] )
		if ( rec.valid ( ) )
			ret.push_back ( rec );

	return ret;
}

std::optional< features::lagcomp::lag_record_t > features::lagcomp::get_shot ( const std::deque< features::lagcomp::lag_record_t >& valid_records ) {
	for ( auto& rec : valid_records )
		if ( rec.m_priority == 1 )
			return rec;

	return std::nullopt;
}

bool features::lagcomp::cache( player_t* pl ) {
	if ( !pl->valid( ) || !pl->weapon( ) || pl->team( ) == g::local->team( ) )
		return false;
	
	const auto idx = pl->idx ( );

	if( anims::anim_info [ idx ].empty ( ) )
		return false;

	static lag_record_t rec {};

	if ( rec.store( pl, data::old_origins [ idx ] ) ) {
		/* mark as onshot */
		if ( anims::anim_info [ idx ].front ( ).m_shot ) {
			rec.m_priority = 1;
			data::shot_count [ idx ]++;
		}

		data::records [ idx ].push_front ( rec );
		data::visual_records [ idx ].push_front ( rec );
	}
	
	data::old_origins [ idx ] = pl->origin( );

	while ( !data::visual_records [ idx ].empty ( ) && abs ( cs::ticks2time ( g::local->tick_base ( ) ) - data::visual_records [ idx ].back ( ).m_simtime ) > 0.5f )
		data::visual_records [ idx ].pop_back ( );

	/* pop off dead records */
	const int dead_time = pl->simtime ( ) - g::cvars::sv_maxunlag->get_float ( );

	while ( data::records [ idx ].size ( ) > 1 ) {
		if ( data::records [ idx ].back ( ).m_simtime >= dead_time )
			break;

		data::records [ idx ].pop_back ( );
	}
}