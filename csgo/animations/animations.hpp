#pragma once
#include <sdk.hpp>

namespace animations {
	namespace rebuilt {
		namespace poses {
			float jump_fall( player_t* pl, float air_time );
			float body_pitch( player_t* pl, float pitch );
			float body_yaw( player_t* pl, float yaw );
		}
	}

	namespace fake {
		std::array< matrix3x4_t, 128 >& matrix( );
		void simulate( );
	}

	struct bone_setup_args {
		struct animating* m_anim;
		matrix3x4_t* m_mat;
		int m_bone_count, m_mask;
		float m_seed;
	};

	void force_animation_skip( struct animating* a, bool skip_anim_frame );
	void fix_matrix_construction( struct animating* a );
	bool build_matrix( player_t* pl );
	void fix_local( );
	void fix_pl( player_t* pl );
	void run( int stage );
}