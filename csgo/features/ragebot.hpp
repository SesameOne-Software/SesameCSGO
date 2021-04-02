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
				onshot_only;

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
				safe_point_key, safe_point_key_mode,
				dt_recharge_delay;
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
		anims::anim_info_t& get_lag_rec( int pl );
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

				vec3_t screen;

				for ( auto& point : m_synced_points ) {
					if ( cs::render::world_to_screen ( screen, point ) ) {
						const auto radius = 8.0f;
						const auto x = screen.x;
						const auto y = screen.y;
						const auto verticies = 16;

						struct vtx_t {
							float x, y, z, rhw;
							std::uint32_t color;
						};

						std::vector< vtx_t > circle ( verticies + 2 );

						const auto angle = 0.0f;

						circle [ 0 ].x = static_cast< float > ( x ) - 0.5f;
						circle [ 0 ].y = static_cast< float > ( y ) - 0.5f;
						circle [ 0 ].z = 0;
						circle [ 0 ].rhw = 1;
						circle [ 0 ].color = D3DCOLOR_RGBA ( 255, 0, 0, 255 );

						for ( auto i = 1; i < verticies + 2; i++ ) {
							circle [ i ].x = ( float ) ( x - radius * std::cosf ( std::numbers::pi * ( ( i - 1 ) / ( static_cast<float>( verticies ) / 2.0f ) ) ) ) - 0.5f;
							circle [ i ].y = ( float ) ( y - radius * std::sinf ( std::numbers::pi * ( ( i - 1 ) / ( static_cast<float>( verticies ) / 2.0f ) ) ) ) - 0.5f;
							circle [ i ].z = 0;
							circle [ i ].rhw = 1;
							circle [ i ].color = D3DCOLOR_RGBA ( 255, 0 , 0, 0 );
						}

						for ( auto i = 0; i < verticies + 2; i++ ) {
							circle [ i ].x = x + std::cosf ( angle ) * ( circle [ i ].x - x ) - std::sinf ( angle ) * ( circle [ i ].y - y ) - 0.5f;
							circle [ i ].y = y + std::sinf ( angle ) * ( circle [ i ].x - x ) + std::cosf ( angle ) * ( circle [ i ].y - y ) - 0.5f;
						}

						IDirect3DVertexBuffer9* vb = nullptr;

						cs::i::dev->CreateVertexBuffer ( ( verticies + 2 ) * sizeof ( vtx_t ), D3DUSAGE_WRITEONLY, D3DFVF_XYZRHW | D3DFVF_DIFFUSE, D3DPOOL_DEFAULT, &vb, nullptr );

						void* verticies_ptr;
						vb->Lock ( 0, ( verticies + 2 ) * sizeof ( vtx_t ), ( void** ) &verticies_ptr, 0 );
						std::memcpy ( verticies_ptr, &circle [ 0 ], ( verticies + 2 ) * sizeof ( vtx_t ) );
						vb->Unlock ( );

						cs::i::dev->SetStreamSource ( 0, vb, 0, sizeof ( vtx_t ) );
						cs::i::dev->DrawPrimitive ( D3DPT_TRIANGLEFAN, 0, verticies );

						if ( vb )
							vb->Release ( );
					}
				}
			}
		};

		extern c_scan_points scan_points;

		bool dmg_hitchance( vec3_t ang, player_t* pl, vec3_t point, int rays, int hitbox );
		bool hitchance( vec3_t ang, player_t* pl, vec3_t point, int rays, int hitbox, anims::anim_info_t& rec );
		void slow ( ucmd_t* ucmd, float& old_smove, float& old_fmove );
		void run_meleebot ( ucmd_t* ucmd );
		void run( ucmd_t* ucmd, float& old_smove, float& old_fmove, vec3_t& old_angs );
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

		bool hitscan( player_t* ent, anims::anim_info_t& rec, vec3_t& pos_out, int& hitbox_out, float& best_dmg );
		bool create_points( player_t* ent, anims::anim_info_t& rec, int i, std::deque< vec3_t >& points, multipoint_side_t multipoint_side );
		bool get_hitbox( player_t* ent, anims::anim_info_t& rec, int i, vec3_t& pos_out, float& rad_out, float& zrad_out );
		void idealize_shot( player_t* ent, vec3_t& pos_out, int& hitbox_out, anims::anim_info_t& rec_out, float& best_dmg );
	}
}