#pragma once
#include "../sdk/sdk.hpp"

namespace features {
	namespace autopeek {
		struct peek_data_t {
			vec3_t m_pos;
			float m_time;
			bool m_retrack, m_recorded;
			float m_fade;
		};

		inline peek_data_t peek {};

		void run ( ucmd_t* ucmd, vec3_t& move_ang );
		void draw ( );
	}
}