#pragma once
#include "../sdk/sdk.hpp"

#include "anims.hpp"

namespace anims {
	namespace resolver {
		namespace rdata {
			inline std::array < std::array<animlayer_t, 13>, 65 > latest_layers { {} };
			inline std::array < bool, 65 > new_resolve { false };
			inline std::array < bool, 65 > was_moving { false };
			inline std::array < bool, 65 > prefer_edge { false };
			
			inline std::array < bool, 65 > last_good_weight { false };
			inline std::array < bool, 65 > last_bad_weight { false };
			inline std::array < bool, 65 > resolved_jitter { false };
			inline std::array < int, 65 > jitter_sync { 0 };
		}

		struct hit_matrix_rec_t {
			uint32_t m_pl;
			std::array < matrix3x4_t, 128 > m_bones;
			float m_time;
			std::array<float, 4> m_color;

			mdlrender_info_t m_render_info;
			mdlstate_t m_mdl_state;
		};
		
		struct impact_rec_t {
			vec3_t m_src, m_dst;
			std::string m_msg;
			float m_time;
			bool m_hurt;
			uint32_t m_clr;
			bool m_beam_created;
		};

		inline std::deque< anims::resolver::hit_matrix_rec_t > hit_matrix_rec { };
		inline std::deque< impact_rec_t > impact_recs { };

		inline float weight_tolerance = 1.0f;
		
		bool process_blood ( const effect_data_t& effect_data );
		void process_impact( event_t* event );
		void process_hurt( event_t* event );
		void process_event_buffer( );
		bool is_spotted ( player_t* src, player_t* dst );
		void create_beams( );
		void render_impacts( );
	}
}