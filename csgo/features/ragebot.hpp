#pragma once
#include <sdk.hpp>
#include "lagcomp.hpp"

namespace features {
	namespace ragebot {
		struct misses_t {
			int spread = 0;
			int bad_resolve = 0;
			int occlusion = 0;
		};

		static bool revolver_firing = false;
		static bool revolver_cocking = false;

		lagcomp::lag_record_t& get_lag_rec ( int pl );
		int& get_target_idx ( );
		player_t*& get_target ( );
		misses_t& get_misses( int pl );
		vec3_t& get_target_pos( int pl );
		vec3_t& get_shot_pos( int pl );
		int& get_hits( int pl );
		int& get_shots ( int pl );
		int& get_hitbox( int pl );

		void hitscan( player_t* pl, vec3_t& point, float& dmg, lagcomp::lag_record_t& rec_out, int& hitbox_out );
		void run( ucmd_t* ucmd, float& old_smove, float& old_fmove, vec3_t& old_angs );
		void tickbase_controller( ucmd_t* ucmd );
	}
}