#pragma once
#include <deque>
#include <array>
#include "../sdk/sdk.hpp"

namespace anims {
	namespace local {
		inline float feet_yaw = 0.0f;
		inline std::array<float, 24> poses { 0.0f };
		inline std::array<animlayer_t, 13> anim_layers { {} };
		inline std::array<matrix3x4_t, 128> fake_matrix { };
		inline std::array<matrix3x4_t, 128> real_matrix { };
		inline bool simulating_fake = false;
	}

	namespace players {
		inline std::array<std::array<std::array<float, 24>, 3>, 65> poses { { {0.0f} } };
		inline std::array<std::array<std::array<matrix3x4_t, 128>, 3>, 65> matricies { { {{}} } };
		inline std::array<std::array<float, 3>, 65> resolved_feet_yaws { { {0.0f} } };
		inline std::array<std::array<animlayer_t, 13>, 65> anim_layers { { {} } };

		inline std::array<vec3_t, 65> accelerations { {} };
		inline std::array<vec3_t, 65> last_velocities { {} };
		inline std::array<vec3_t, 65> last_origins { {} };
		inline std::array<flags_t, 65> flags { {} };
		inline std::array<flags_t, 65> last_flags { {} };
		inline std::array<float, 65> anim_times { 0.0f };
		inline std::array<float, 65> update_time { 0.0f };
		inline std::array<int, 65> updates_since_dormant { 0 };
		inline std::array<int, 65> choked_commands { 0 };
	}

	bool build_bones ( player_t* target, matrix3x4_t* mat, int mask, vec3_t rotation, vec3_t origin, float time, std::array<float, 24>& poses );

	void manage_fake ( );

	void predict_movement ( player_t* ent, flags_t& old_flags, vec3_t& abs_origin, vec3_t& abs_vel, vec3_t& abs_accel );
	void pre_update_callback ( animstate_t* anim_state );
	void post_update_callback ( animstate_t* anim_state );

	float angle_diff ( float dst, float src );
	void calc_poses ( player_t* ent, std::array<float, 24>& poses, float feet_yaw );

	void init ( );

	void update ( int stage );
}