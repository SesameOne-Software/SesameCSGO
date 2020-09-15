#pragma once
#include <mutex>
#include "../sdk/sdk.hpp"
#include "lagcomp.hpp"
#include "../renderer/d3d9.hpp"

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
				dt_enabled;

			double min_dmg,
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
				safe_point_key, safe_point_key_mode;
		};

		extern weapon_config_t active_config;

		struct misses_t {
			int spread = 0;
			int bad_resolve = 0;
			int occlusion = 0;
		};

		struct aim_target_t {
			player_t* m_ent;
			float m_fov;
			float m_dist;
			int m_health;
		};

		void get_weapon_config( weapon_config_t& const config );
		lagcomp::lag_record_t& get_lag_rec( int pl );
		int& get_target_idx( );
		player_t*& get_target( );
		misses_t& get_misses( int pl );
		vec3_t& get_target_pos( int pl );
		vec3_t& get_shot_pos( int pl );
		int& get_hits( int pl );
		int& get_shots( int pl );
		int& get_hitbox( int pl );

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

				render::rectangle( 100, 100, 10, 10, D3DCOLOR_RGBA( 255, 0, 0, 255 ) );

				vec3_t screen;

				for ( auto& point : m_synced_points ) {
					if ( csgo::render::world_to_screen( screen, point ) )
						render::rectangle( screen.x, screen.y, 2, 2, D3DCOLOR_RGBA( 255, 0, 0, 255 ) );
				}
			}
		};

		extern c_scan_points scan_points;

		bool dmg_hitchance( vec3_t ang, player_t* pl, vec3_t point, int rays, int hitbox );
		bool hitchance( vec3_t ang, player_t* pl, vec3_t point, int rays, int hitbox, lagcomp::lag_record_t& rec );
		void slow ( ucmd_t* ucmd, float& old_smove, float& old_fmove );
		void run( ucmd_t* ucmd, float& old_smove, float& old_fmove, vec3_t& old_angs );
		void tickbase_controller( ucmd_t* ucmd );
		bool can_shoot( );
		void select_targets( std::deque < aim_target_t >& targets_out );

		/* hitscan */
		enum multipoint_side_t {
			mp_side_none = 0,
			mp_side_left,
			mp_side_right
		};

		enum multipoint_mode_t {
			mp_none = 0,
			mp_center = ( 1 << 1 ),
			mp_left = ( 1 << 2 ),
			mp_right = ( 1 << 3 ),
			mp_bottom = ( 1 << 4 ),
			mp_top = ( 1 << 5 )
		};

		bool hitscan( lagcomp::lag_record_t& rec, vec3_t& pos_out, int& hitbox_out, float& best_dmg );
		bool create_points( lagcomp::lag_record_t& rec, int i, std::deque< vec3_t >& points, multipoint_side_t multipoint_side );
		bool get_hitbox( lagcomp::lag_record_t& rec, int i, vec3_t& pos_out, float& rad_out, float& zrad_out );
		void idealize_shot( player_t* ent, vec3_t& pos_out, int& hitbox_out, lagcomp::lag_record_t& rec_out, float& best_dmg );
	}
}