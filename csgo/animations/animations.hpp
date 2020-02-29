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
		int simulate( );
	}

	int force_animation_skip( uintptr_t a, bool skip_anim_frame );
	int fix_matrix_construction( uintptr_t a );
	bool build_matrix( player_t* pl );
	int fix_local( );
	int fix_pl( player_t* pl );
	int run( int stage );
}