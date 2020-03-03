#pragma once
#include <sdk.hpp>

namespace animations {
	namespace data {
		static std::array< vec3_t, 65 > origin { vec3_t( FLT_MAX, FLT_MAX, FLT_MAX ) };
		static std::array< vec3_t, 65 > old_origin { vec3_t( FLT_MAX, FLT_MAX, FLT_MAX ) };
		static std::array< std::array< matrix3x4_t, 128 >, 65 > bones { };
	}

	namespace rebuilt {
		namespace poses {
			float jump_fall( player_t* pl, float air_time );
			float body_pitch( player_t* pl, float pitch );
			float body_yaw( player_t* pl, float yaw );
			float lean_yaw( player_t* pl, float yaw );

			void calculate( player_t* pl );
		}
	}

	namespace fake {
		std::array< matrix3x4_t, 128 >& matrix( );
		int simulate( );
	}

	bool build_matrix( player_t* pl, matrix3x4_t* out, int max_bones, int mask, float seed );
	int fix_local( );
	void simulate_command( player_t* pl );
	int fix_pl( player_t* pl );
	int run( int stage );
}