#pragma once
#include <deque>
#include <array>
#include "../sdk/sdk.hpp"

namespace anims {
	struct anim_info_t {
		bool m_shot;
		vec3_t m_angles;
		float m_lby;
		float m_simtime;
		float m_old_simtime;
		int m_choked_commands;
		std::array<animlayer_t, 13> m_anim_layers;
		std::array<float, 24> m_poses;
		vec3_t m_vel;
		vec3_t m_abs_angles;
		animstate_t m_anim_state;

		anim_info_t ( player_t* ent ) {
			m_shot = ent->weapon ( ) && ( ent->weapon ( )->last_shot_time ( ) > ent->old_simtime ( ) && ent->weapon ( )->last_shot_time ( ) <= ent->simtime ( ) );
			m_angles = ent->angles ( );
			m_lby = ent->lby ( );
			m_simtime = ent->simtime ( );
			m_old_simtime = ent->old_simtime ( );
			m_choked_commands = std::clamp ( cs::time2ticks ( ent->simtime ( ) - ent->old_simtime ( ) ) - 1, 0, g::cvars::sv_maxusrcmdprocessticks->get_int() );
			memcpy ( m_anim_layers.data ( ), ent->layers ( ), sizeof ( m_anim_layers ) );
			m_poses = ent->poses ( );
			m_vel = ent->vel ( );
			m_abs_angles = vec3_t( 0.0f, ent->animstate ( )->m_abs_yaw, 0.0f );
			m_anim_state = *ent->animstate ( );
		}
	};

	inline std::array< std::deque< anim_info_t >, 65> anim_info { {} };
	inline std::array< std::array<std::array< matrix3x4_t, 128 >,3>, 65> aim_matricies { {} };
	inline std::array< matrix3x4_t, 128 > real_matrix { {} };
	inline std::array< matrix3x4_t, 128 > fake_matrix { {} };

	void on_net_update_end ( int idx );
	void on_render_start ( int idx );

	void manage_fake ( );

	void reset_data ( int idx );
	void update_anims ( player_t* ent, vec3_t& angles );
	void apply_anims ( player_t* ent );

	void pre_fsn ( int stage );
	void fsn ( int stage );
}