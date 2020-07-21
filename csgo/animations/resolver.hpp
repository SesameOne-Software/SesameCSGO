#pragma once
#include <sdk.hpp>

namespace animations {
	namespace resolver {
		namespace rdata {
			static std::array< float, 65 > impact_dmg { 0.0f };
			static std::array< vec3_t, 65 > impacts { vec3_t( 0.0f, 0.0f, 0.0f ) };
			static std::array< int, 65 > player_dmg { 0 };
			static std::array< bool, 65 > player_hurt { false };
			static std::array< bool, 65 > queued_hit { false };
			static std::array< bool, 65 > has_jitter { false };
			static std::array< bool, 65 > flip_jitter { false };
			static std::array< int, 65 > last_shots { 0 };
			static std::array< int, 65 > old_tc { 0 };
			static std::array< float, 65 > spawn_times { 0.0f };
			static std::array< int, 65 > forced_side { 0 };
			static std::array< int, 65 > tried_side { 0 };
			static std::array< float, 65 > l_dmg { 0.0f };
			static std::array< float, 65 > r_dmg { 0.0f };
			static std::array< float, 65 > last_vel_rate { 0.0f };
			static std::array< float, 65 > last_vel_check { 0.0f };
			static std::array< float, 65 > last_abs_delta { 0.0f };
		}

		namespace flags {
			extern std::array< bool, 65 > has_slow_walk;
			extern std::array< bool, 65 > has_micro_movements;
			extern std::array< bool, 65 > has_desync;
			extern std::array< bool, 65 > has_jitter;

			extern std::array< bool, 65 > force_pitch_flick_yaw;
			extern std::array< float, 65 > eye_delta;
			extern std::array< float, 65 > last_pitch;
			extern std::array< animlayer_t[ 15 ], 65 > anim_layers;

			static bool has ( player_t* pl, const std::string_view& flag ) {
				if ( flag == _ ( "slow walk" ) )
					return has_slow_walk [ pl->idx ( ) ];
				else if ( flag == _ ( "micro movements" ) )
					return has_micro_movements [ pl->idx ( ) ];
				else if ( flag == _ ( "desync" ) )
					return has_desync [ pl->idx ( ) ];
				else if ( flag == _ ( "jitter" ) )
					return has_jitter [ pl->idx ( ) ];

				return false;
			}
		}

		struct hit_matrix_rec_t {
			uint32_t m_pl;
			std::array < matrix3x4_t, 128 > m_bones;
			float m_time;
			uint32_t m_clr;
		};

		float get_confidence ( int pl_idx );
		float get_dmg ( player_t* pl, int side );
		bool jittering( player_t* pl );
		void process_blood ( const effect_data_t& effect_data );
		void process_impact( event_t* event );
		void process_hurt( event_t* event );
		void process_event_buffer( int pl_idx );
		void resolve_edge ( player_t* pl, float& yaw );
		void resolve_smart ( player_t* pl, float& yaw );
		void resolve_smart_v2 ( player_t* pl, float& yaw );
		void resolve( player_t* pl, float& yaw1, float& yaw2, float& yaw3 );
		void render_impacts( );
		void update( ucmd_t* ucmd );
	}
}