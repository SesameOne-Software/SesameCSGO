#pragma once
#include "../sdk/sdk.hpp"

#include "anims.hpp"

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
			inline bool clientside_shot = false;

			inline float lean_tolerance = 3.0f;
			inline float velocity_tolerance = 0.909f;

			inline std::array < std::array<animlayer_t, 13>, 65 > latest_layers { {} };
			inline std::array < bool, 65 > new_resolve { false };
			inline std::array < bool, 65 > was_moving { false };
			inline std::array < bool, 65 > prefer_edge { false };
			inline std::array < anims::desync_side_t, 65 > resolved_side { anims::desync_side_t::desync_middle };
		}

		struct hit_matrix_rec_t {
			uint32_t m_pl;
			std::array < matrix3x4_t, 128 > m_bones;
			float m_time;
			uint32_t m_clr;
		};
		
		void process_impact_clientside ( event_t* event );
		void process_impact( event_t* event );
		void process_hurt( event_t* event );
		void process_event_buffer( int pl_idx );
		bool resolve_desync( player_t* ent, anim_info_t& rec );
		void create_beams( );
		void render_impacts( );
	}
}