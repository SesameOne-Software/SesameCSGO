#pragma once
#include <sdk.hpp>
#include <deque>
#include "../globals.hpp"
#include "../animations/animations.hpp"
#include "prediction.hpp"

namespace features {
	namespace lagcomp {
		float lerp( );

		struct lag_record_t {
			player_t* m_pl;
			int m_tick, m_flags, m_priority;
			float m_simtime;
			bool m_lc, m_needs_matrix_construction, m_extrapolated;
			vec3_t m_min, m_max, m_vel, m_origin, m_ang;
			animlayer_t m_layers [ 15 ];
			animstate_t m_state;
			matrix3x4_t m_bones [ 128 ];
			float m_poses [ 24 ];

			bool operator==( const lag_record_t& otr ) {
				return m_tick == otr.m_tick;
			}

			bool store( player_t* pl, const vec3_t& last_origin, bool simulated = false ) {
				if ( simulated ) {
					m_pl = pl;
					m_needs_matrix_construction = false;
					m_tick = csgo::time2ticks( prediction::predicted_curtime );
					m_simtime = animations::data::simulated::animtimes [ pl->idx ( ) ];
					m_flags = animations::data::simulated::flags [ pl->idx ( ) ];
					m_ang = pl->angles ( );
					m_origin = animations::data::simulated::origins [ pl->idx ( ) ];
					m_min = pl->mins ( );
					m_max = pl->maxs ( );
					m_vel = animations::data::simulated::velocities [ pl->idx ( ) ];
					m_lc = true;
					
					std::memcpy ( &m_state, &animations::data::simulated::animstates [ pl->idx ( ) ], sizeof ( animstate_t ) );
					std::memcpy ( m_layers, animations::data::simulated::overlays [ pl->idx ( ) ], sizeof ( animlayer_t [ 15 ] ) );
					std::memcpy ( m_bones, animations::data::simulated::bones [ pl->idx ( ) ], sizeof ( matrix3x4_t [ 128 ] ) );
					std::memcpy ( m_poses, animations::data::simulated::poses [ pl->idx ( ) ], sizeof ( float [ 24 ] ) );

					return true;
				}

				m_pl = pl;

				if ( !pl->layers( ) || !pl->bone_cache( ) || !pl->animstate( ) || !g::local )
					return false;

				m_priority = 0;
				m_needs_matrix_construction = false;
				m_tick = csgo::time2ticks ( prediction::predicted_curtime );
				m_simtime = pl->simtime( );
				m_flags = pl->flags( );
				m_ang = pl->angles( );
				m_origin = pl->origin( );
				m_min = pl->mins( );
				m_max = pl->maxs( );
				m_vel = pl->vel( );
				m_lc = pl->origin( ).dist_to_sqr( last_origin ) <= 4096.0f;

				std::memcpy( m_layers, pl->layers( ), sizeof animlayer_t * 15 );
				std::memcpy ( m_bones, pl->bone_cache ( ), sizeof matrix3x4_t * pl->bone_count( ) );
				std::memcpy ( &m_state, pl->animstate( ), sizeof ( animstate_t ) );
				std::memcpy( m_poses, &pl->poses( ), sizeof( float ) * 24 );

				return true;
			}

			bool valid( ) {
				const auto nci = csgo::i::engine->get_net_channel_info ( );

				if ( !nci || !g::local )
					return false;
				
				const auto correct = std::clamp( nci->get_latency ( 0 ) + nci->get_latency ( 1 ) + lerp ( ), 0.0f, 0.2f );
				const auto dt = correct - ( prediction::predicted_curtime - m_simtime );

				return std::abs ( dt ) < 0.2f;
			}

			void backtrack( ucmd_t* ucmd ) {
				ucmd->m_tickcount = csgo::time2ticks ( m_simtime + lerp ( ) );
			}

			void extrapolate( ) {
				auto dst = m_origin + m_vel * csgo::i::globals->m_ipt;

				trace_t tr;
				csgo::util_tracehull( m_origin + vec3_t( 0.0f, 0.0f, 2.0f ), dst, m_min, m_max, 0x201400B, m_pl, &tr );

				m_flags &= ~1;
				m_origin = dst;

				if ( tr.did_hit( ) && tr.m_plane.m_normal.z > 0.7f ) {
					m_origin = tr.m_endpos;
					m_flags |= 1;
				}
			}
		};

		namespace data {
			extern std::array< vec3_t, 65 > old_origins;
			extern std::array< float, 65 > old_shots;
			extern std::array< int, 65 > shot_count;
			extern std::array< lag_record_t, 65 > interpolated_oldest;
			extern std::array< std::deque< lag_record_t >, 65 > records;
			extern std::array< std::deque< lag_record_t >, 65 > all_records;
			extern std::array< lag_record_t, 65 > shot_records;
			extern std::array< lag_record_t, 65 > cham_records;
			extern std::array< std::deque< lag_record_t >, 65 > extrapolated_records;
		}

		const std::pair< std::deque< lag_record_t >&, bool > get ( player_t* pl );
		const std::pair< std::deque< lag_record_t >&, bool > get_all( player_t* pl );
		const std::pair< lag_record_t&, bool > get_extrapolated( player_t* pl );
		const std::pair< lag_record_t&, bool > get_shot( player_t* pl );
		void pop( player_t* pl );
		bool extrapolate_record( player_t* pl, lag_record_t& rec, bool shot = false );
		void cache_shot( event_t* event );
		void cache( player_t* pl );
		bool breaking_lc( player_t* pl );
		bool has_onshot( player_t* pl );
	}
}