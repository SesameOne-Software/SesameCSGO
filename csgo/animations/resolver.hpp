#pragma once
#include "../sdk/sdk.hpp"

namespace anims {
	namespace resolver {
		namespace rdata {
			inline std::array< float, 65 > impact_dmg { 0.0f };
			inline std::array< vec3_t, 65 > impacts { vec3_t ( 0.0f, 0.0f, 0.0f ) };
			inline vec3_t non_player_impacts = vec3_t ( 0.0f, 0.0f, 0.0f );
			inline std::array< int, 65 > player_dmg { 0 };
			inline std::array< bool, 65 > player_hurt { false };
			inline std::array< bool, 65 > wrong_hitbox { false };
			inline std::array< bool, 65 > queued_hit { false };
			inline std::array< int, 65 > last_shots { 0 };
		}

		struct hit_matrix_rec_t {
			uint32_t m_pl;
			std::array < matrix3x4_t, 128 > m_bones;
			float m_time;
			uint32_t m_clr;
		};

		void process_impact( event_t* event );
		void process_hurt( event_t* event );
		void process_event_buffer( int pl_idx );
		float resolve_yaw( player_t* pl );
		void create_beams( );
		void render_impacts( );
	}
}