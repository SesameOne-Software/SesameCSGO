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
			float m_simtime, m_lby, m_abs_yaw, m_unresolved_abs;
			bool m_lc, m_needs_matrix_construction, m_extrapolated;
			vec3_t m_min, m_max, m_vel, m_origin, m_ang;
			animlayer_t m_layers [ 15 ];
			animstate_t m_state;
			matrix3x4_t m_bones1 [ 128 ];
			matrix3x4_t m_bones2 [ 128 ];
			matrix3x4_t m_bones3 [ 128 ];
			float m_poses [ 24 ];
			int m_failed_resolves;

			bool operator==( const lag_record_t& otr ) {
				return m_tick == otr.m_tick;
			}

			bool store ( player_t* pl, const vec3_t& last_origin, bool simulated = false );

			bool valid( bool use_tick = false ) {
				const auto nci = csgo::i::engine->get_net_channel_info ( );

				if ( !nci || !g::local )
					return false;
				
				const auto correct = std::clamp( nci->get_latency ( 0 ) + nci->get_latency ( 1 ) + lerp ( ), 0.0f, 0.2f );
				const auto dt = correct - ( prediction::predicted_curtime - ( use_tick ? csgo::ticks2time ( m_tick ) : m_simtime ) );

				return std::abs ( dt ) < 0.2f;
			}

			void backtrack( ucmd_t* ucmd ) {
			//	if ( !m_extrapolated )
					ucmd->m_tickcount = csgo::time2ticks ( m_simtime + lerp ( ) );
				//else
				//	ucmd->m_tickcount = csgo::time2ticks ( prediction::predicted_curtime + lerp ( ) );
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
		//bool extrapolate_record( player_t* pl, lag_record_t& rec, bool shot = false );
		void cache_shot( event_t* event );
		void cache( player_t* pl );
		bool breaking_lc( player_t* pl );
		bool has_onshot( player_t* pl );
	}
}