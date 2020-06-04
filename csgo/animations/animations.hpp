#pragma once
#include <sdk.hpp>

namespace animations {
	namespace data {
		namespace simulated {
			extern std::array< float, 65 > animtimes;
			extern std::array< int, 65 > flags;
			extern std::array< vec3_t, 65 > origins;
			extern std::array< vec3_t, 65 > velocities;
			extern std::array< animstate_t, 65 > animstates;
			extern std::array< matrix3x4_t [ 128 ], 65 > bones;
			extern std::array< float [ 24 ], 65 > poses;
			extern std::array< animlayer_t [ 15 ], 65 > overlays;
		}

		extern std::array< animstate_t, 65 > fake_states;
		extern std::array< vec3_t, 65 > origin;
		extern std::array< vec3_t, 65 > old_origin;
		extern std::array< std::array< matrix3x4_t, 128 >, 65 > bones;
		extern std::array< std::array< float, 24 >, 65 > poses;
		extern std::array< int, 65 > last_animation_frame;
		extern std::array< int, 65 > old_tick;
		extern std::array< float, 65 > old_abs;
		extern std::array< std::array< animlayer_t, 15 >, 65 > overlays;
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
		extern animstate_t fake_state;
		std::array< matrix3x4_t, 128 >& matrix( );
		int simulate( );
	}

	void estimate_vel ( player_t* pl, vec3_t& out );
	bool build_matrix( player_t* pl, matrix3x4_t* out, int max_bones, int mask, float seed );
	int fix_local( );
	void simulate_movement( player_t* pl, vec3_t& origin );
	void simulate_command( player_t* pl );
	int fix_pl( player_t* pl );
	void simulate ( player_t* pl, bool updated = false );
	int run( int stage );
}