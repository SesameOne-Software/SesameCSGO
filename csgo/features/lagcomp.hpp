#pragma once
#include "../sdk/sdk.hpp"
#include <deque>
#include <optional>
#include "../globals.hpp"
#include "../animations/anims.hpp"
#include "prediction.hpp"

namespace features {
	namespace lagcomp {
		inline float old_curtime = 0.0f;

		float lerp( );

		struct lag_record_t {
			int m_idx, m_priority;
			flags_t m_flags;
			float m_simtime, m_lby;
			bool m_lc;
			vec3_t m_min, m_max, m_vel, m_origin, m_abs_origin, m_ang;
			animstate_t m_state;
			std::array< animlayer_t,13> m_layers;
			std::array< std::array< matrix3x4_t, 128>, 3> m_aim_bones;
			std::array< matrix3x4_t, 128> m_render_bones;
			std::array< float,24> m_poses;

			bool operator==( const lag_record_t& otr ) const {
				return cs::time2ticks( m_simtime ) == cs::time2ticks(otr.m_simtime);
			}

			bool store ( player_t* pl, const vec3_t& last_origin );
			bool apply ( player_t* pl );

			inline bool valid( ) {
				const auto nci = cs::i::engine->get_net_channel_info( );

				if ( !nci || !g::local )
					return false;

				if ( m_simtime < int ( cs::i::globals->m_curtime - g::cvars::sv_maxunlag->get_float ( ) ) )
					return false;

				const auto correct = std::clamp ( nci->get_latency ( 0 ) + nci->get_latency ( 1 ) + lerp ( ), 0.0f, g::cvars::sv_maxunlag->get_float ( ) );

				return abs ( correct - ( cs::i::globals->m_curtime - m_simtime ) ) <= 0.2f;
			}

			inline void backtrack ( ucmd_t* ucmd ) {
				ucmd->m_tickcount = cs::time2ticks ( m_simtime + lerp ( ) );
			}
		};

		namespace data {
			inline std::array< vec3_t, 65 > old_origins;
			inline std::array< int, 65 > shot_count;
			inline std::array< std::deque< lag_record_t >, 65 > records;
			inline std::array< std::deque< lag_record_t >, 65 > visual_records;
		}

		std::deque< lag_record_t > get_records( player_t* ent );
		std::optional< lag_record_t > get_shot( const std::deque< lag_record_t >& valid_records );
		bool get_render_record ( player_t* ent, std::array<matrix3x4_t, 128>& out );
		bool cache( player_t* pl );
	}
}