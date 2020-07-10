#pragma once
#include <sdk.hpp>
#include "lagcomp.hpp"

namespace features {
	namespace ragebot {
		struct weapon_config_t {
			bool main_switch,
				choke_on_shot,
				silent,
				auto_shoot,
				auto_scope,
				auto_slow,
				auto_revolver,
				knife_bot,
				zeus_bot,
				dt_teleport,
				baim_lethal,
				baim_air,
				scan_head,
				scan_neck,
				scan_chest,
				scan_pelvis,
				scan_arms,
				scan_legs,
				scan_feet,
				safe_point,
				legit_mode,
				triggerbot;

			double min_dmg,
				hit_chance,
				dt_hit_chance,
				max_dt_ticks,
				baim_after_misses,
				head_pointscale,
				body_pointscale,
				baim_if_resolver_confidence_less_than;

			int inherit_from,
				dt_key,
				optimization,
				triggerbot_key;
		};
		
		extern weapon_config_t active_config;

		struct misses_t {
			int spread = 0;
			int bad_resolve = 0;
			int occlusion = 0;
		};

		static bool revolver_firing = false;
		static bool revolver_cocking = false;

		extern std::array< float, 65 > hitchances;

		void get_weapon_config ( weapon_config_t& const config );
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