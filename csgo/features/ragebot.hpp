#pragma once
#include <mutex>
#include <numbers>

#include "../sdk/sdk.hpp"
#include "../animations/anims.hpp"
#include "../renderer/render.hpp"

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
				fix_fakelag,
				resolve_desync,
				headshot_only,
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
				triggerbot,
				dt_enabled,
				onshot_only,
				dt_smooth_recharge;

			double min_dmg,
				min_dmg_override,
				dmg_accuracy,
				hit_chance,
				dt_hit_chance,
				max_dt_ticks,
				baim_after_misses,
				head_pointscale,
				body_pointscale,
				baim_if_resolver_confidence_less_than;

			int inherit_from,
				dt_key, dt_key_mode,
				triggerbot_key, triggerbot_key_mode,
				safe_point_key, safe_point_key_mode,
				dt_recharge_delay;
		};

		extern weapon_config_t active_config;

		struct misses_t {
			int spread = 0;
			int pred_error = 0;
			int bad_resolve = 0;
			int occlusion = 0;
		};

		struct aim_target_t {
			player_t* m_ent;
			float m_fov;
			float m_dist;
			int m_health;
		};

		struct shot_t {
			uint32_t idx;
			anims::anim_info_t rec;
			int backtrack;
			hitbox_t hitbox;
			vec3_t src, dst;
			float hitchance;
			float body_yaw;
			int dmg;
			float vel_modifier;

			vec3_t processed_impact_pos;
			int processed_hitgroup;
			int processed_dmg, processed_impact_dmg;
			bool processed_impact, processed_hurt;
			int processed_tick;
		};

		inline std::vector<shot_t> shots {};

		void get_weapon_config( weapon_config_t& const config );
		int& get_target_idx( );
		misses_t& get_misses( int pl );
		int& get_hits( int pl );

		inline void get_shots ( int pl, std::vector<const features::ragebot::shot_t*>& shots_out ) {
			for ( auto& shot : shots )
				if ( shot.idx == pl )
					shots_out.push_back ( &shot );
		}

		inline features::ragebot::shot_t* get_unprocessed_shot ( ) {
			for ( auto& shot : shots )
				if ( !shot.processed_tick || g::server_tick == shot.processed_tick )
					return &shot;

			return nullptr;
		}

		class c_scan_points {
			std::deque< vec3_t > m_points;
			std::deque< vec3_t > m_synced_points;
			std::mutex m_mutex;

		public:
			void emplace( const vec3_t& point ) {
				m_points.push_back( point );
			}

			void sync( ) {
				std::lock_guard< std::mutex > guard( m_mutex );
				m_synced_points = m_points;

				if ( !m_points.empty( ) )
					m_points.clear( );
			}

			void clear( ) {
				if ( !m_points.empty( ) )
					m_points.clear( );
			}

			void draw( ) {
				std::lock_guard< std::mutex > guard( m_mutex );

				for ( auto& point : m_synced_points ) {
					vec3_t screen;

					if ( cs::render::world_to_screen ( screen, point ) ) {
						render::circle ( screen.x, screen.y, 1.5f, 6, rgba ( 255, 0, 0, 255 ), false );
					}
				}
			}
		};

		extern c_scan_points scan_points;

		bool hitchance( vec3_t ang, player_t* pl, vec3_t point, int rays, hitbox_t hitbox, anims::anim_info_t& rec, float& hc_out );
		void slow ( ucmd_t* ucmd );
		void run_meleebot ( ucmd_t* ucmd );
		void run( ucmd_t* ucmd, vec3_t& old_angs );
		void tickbase_controller( ucmd_t* ucmd );
		void select_targets( std::deque < aim_target_t >& targets_out );

		/* hitscan */
		enum class multipoint_side_t : uint32_t {
			none = 0,
			left,
			right
		};

		enum class multipoint_mode_t : uint32_t {
			none = 0,
			center = ( 1 << 1 ),
			left = ( 1 << 2 ),
			right = ( 1 << 3 ),
			bottom = ( 1 << 4 ),
			top = ( 1 << 5 )
		};

		ENUM_BITMASK ( multipoint_mode_t );

		inline int extrap_amount = 4;

		float skeet_accelerate_rebuilt ( ucmd_t* cmd, player_t* player, const vec3_t& wish_dir, const vec3_t& wish_speed, bool& ducking );
		void skeet_slow ( ucmd_t* cmd, float wanted_speed, vec3_t& old_angs );

		float get_scaled_min_dmg ( player_t* ent );
		bool hitscan( player_t* ent, anims::anim_info_t& rec, vec3_t& pos_out, hitbox_t& hitbox_out, float& best_dmg );
		bool create_points( player_t* ent, anims::anim_info_t& rec, hitbox_t i, std::deque< vec3_t >& points, multipoint_side_t multipoint_side );
		bool get_hitbox( player_t* ent, anims::anim_info_t& rec, hitbox_t i, vec3_t& pos_out, float& rad_out, float& zrad_out );
		void idealize_shot( player_t* ent, vec3_t& pos_out, hitbox_t& hitbox_out, anims::anim_info_t& rec_out, float& best_dmg );
	}
}