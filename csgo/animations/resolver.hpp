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

			inline float middle_tolerance = 0.0227f;
			inline float low_delta_tolerance = 0.0227f;
			inline float correct_tolerance = 0.0227f;
			inline float lean_tolerance = 20.0f;
			inline float velocity_tolerance = 20.0f;

			inline std::array < std::array<animlayer_t, 13>, 65 > latest_layers { {} };
			inline std::array < bool, 65 > new_resolve { false };
			inline std::array < bool, 65 > was_moving { false };
			inline std::array < bool, 65 > prefer_edge { false };
			inline std::array < anims::desync_side_t, 65 > resolved_side { anims::desync_side_t::desync_middle };
			inline std::array < bool, 65 > last_good_weight { false };
			inline std::array < bool, 65 > last_bad_weight { false };
			inline std::array < bool, 65 > resolved_jitter { false };
			inline std::array < int, 65 > jitter_sync { 0 };
			inline std::array < anims::desync_side_t, 65 > resolved_side_run { anims::desync_side_t::desync_middle };

			inline std::array < anims::desync_side_t, 65 > resolved_side_jitter1 { anims::desync_side_t::desync_middle };
			inline std::array < anims::desync_side_t, 65 > resolved_side_jitter2 { anims::desync_side_t::desync_middle };
		}

		struct hit_matrix_rec_t {
			uint32_t m_pl;
			std::array < matrix3x4_t, 128 > m_bones;
			float m_time;
			std::array<float, 4> m_color;

			mdlrender_info_t m_render_info;
			mdlstate_t m_mdl_state;
		};
		
		bool process_blood ( const effect_data_t& effect_data );
		void process_impact_clientside ( event_t* event );
		void process_impact( event_t* event );
		void process_hurt( event_t* event );
		void process_event_buffer( int pl_idx );
		bool bruteforce ( int brute_mode /* 0 = none, 1 = opposite, 2 = close, 3 = fast, 4 = fast opposite*/, int target_side, anim_info_t& rec );
		desync_side_t apply_antiaim ( player_t* player, float speed_2d, float max_speed );
		bool is_spotted ( player_t* src, player_t* dst );
		bool resolve_desync( player_t* ent, anim_info_t& rec, bool shot );
		void create_beams( );
		void render_impacts( );
	}
}