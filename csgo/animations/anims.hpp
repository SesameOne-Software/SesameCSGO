#pragma once
#include <deque>
#include <array>
#include <optional>

#include "../sdk/sdk.hpp"

namespace anims {
	/* https://github.com/perilouswithadollarsign/cstrike15_src/blob/f82112a2388b841d72cb62ca48ab1846dfcc11c8/game/client/cdll_bounded_cvars.cpp#L112 */
	inline float lerp_time ( ) {
		int ud_rate = g::cvars::cl_updaterate->get_int();

		if ( g::cvars::sv_minupdaterate && g::cvars::sv_maxupdaterate )
			ud_rate = g::cvars::sv_maxupdaterate->get_int ( );

		auto ratio = g::cvars::cl_interp_ratio->get_float ( );

		if ( !ratio )
			ratio = 1.0f;

		if ( g::cvars::sv_client_min_interp_ratio && g::cvars::sv_client_max_interp_ratio && g::cvars::sv_client_min_interp_ratio->get_float ( ) != 1.0f )
			ratio = std::clamp ( ratio, g::cvars::sv_client_min_interp_ratio->get_float ( ), g::cvars::sv_client_max_interp_ratio->get_float ( ) );

		return std::max<float> ( g::cvars::cl_interp->get_float(), ( ratio / ud_rate ) );
	}

	enum pose_param_t : int {
		strafe_yaw = 0 ,
		stand ,
		lean_yaw ,
		speed ,
		ladder_yaw ,
		ladder_speed ,
		jump_fall ,
		move_yaw ,
		move_blend_crouch ,
		move_blend_walk ,
		move_blend_run ,
		body_yaw ,
		body_pitch ,
		aim_blend_stand_idle ,
		aim_blend_stand_walk ,
		aim_blend_stand_run ,
		aim_blend_courch_idle ,
		aim_blend_crouch_walk ,
		death_yaw
	};

	enum desync_side_t : int {
		desync_left_max = 0, 
		desync_left_half,
		desync_middle,
		desync_right_half,
		desync_right_max,
		desync_max
	};

	struct anim_info_t {
		bool m_shot, m_invalid;
		bool m_forward_track;
		bool m_resolved;
		bool m_has_vel;
		bool m_shifted;
		vec3_t m_angles;
		vec3_t m_origin;
		vec3_t m_mins;
		vec3_t m_maxs;
		float m_lby;
		float m_simtime;
		float m_old_simtime;
		float m_duck_amount;
		int m_choked_commands;
		flags_t m_flags;
		vec3_t m_vel;
		std::array<std::array<animlayer_t , 13> , desync_side_t::desync_max + 1> m_anim_layers;
		std::array<std::array<float , 24> , desync_side_t::desync_max + 1> m_poses;
		std::array<vec3_t , desync_side_t::desync_max + 1> m_abs_angles;
		std::array<animstate_t , desync_side_t::desync_max + 1> m_anim_state;
		std::array<std::array< matrix3x4_t , 128 > , desync_side_t::desync_max + 1> m_aim_bones;
		desync_side_t m_side;

		bool valid ( );

		anim_info_t ( ) {

		}

		anim_info_t ( player_t* ent );
	};

	inline std::array<std::array< matrix3x4_t, 128 >, 65> usable_bones {};
	inline vec3_t usable_origin_real_local;
	inline std::array<vec3_t, 65> usable_origin {};
	inline std::array< int, 65> shot_count { 0 };
	inline std::array< std::deque< anim_info_t >, 65> anim_info { {} };
	inline std::array< std::deque< anim_info_t* >, 65> lagcomp_track { {} };
	inline std::array< matrix3x4_t, 128 > real_matrix { {} };
	inline std::array< matrix3x4_t, 128 > fake_matrix { {} };
	inline animstate_t last_local_animstate { };

	void recalc_poses ( std::array<float, 24>& poses, float ladder_yaw, float move_yaw, float eye_yaw, float feet_yaw );

	void on_net_update_end ( int idx );
	void on_render_start ( int idx );

	void manage_fake ( );

	bool build_bones( player_t* target , matrix3x4_t* mat , int mask , vec3_t rotation , vec3_t origin , float time , std::array<float , 24>& poses );
	
	inline std::deque<anim_info_t*> get_lagcomp_records( player_t* ent ) {
		if ( !ent->valid( ) || !ent->idx() || ent->idx() > cs::i::globals->m_max_clients || lagcomp_track [ ent->idx( ) ].empty( ) || !lagcomp_track [ ent->idx ( ) ].front ( )->valid ( ) )
			return {};

		//return { lagcomp_track [ ent->idx ( ) ].front ( ) };
		std::deque<anim_info_t*> records {};
		
		for ( auto& rec : lagcomp_track [ ent->idx( ) ] )
			if ( rec->valid( ) )
				records.push_back( rec );
		
		return records;
	}

	inline std::optional<anim_info_t*> get_simulated_record( player_t* ent ) {
		if ( !ent->valid( ) || !ent->idx( ) || ent->idx( ) > cs::i::globals->m_max_clients || lagcomp_track [ ent->idx( ) ].empty( ) )
			return std::nullopt;

		return std::nullopt;

		/* change later, this is a placeholder */
		return lagcomp_track [ ent->idx( ) ].front( );
	}
	
	inline std::optional<anim_info_t*> get_onshot( const std::deque<anim_info_t*>& recs ) {
		if ( recs.empty() )
			return std::nullopt;

		for ( auto& rec : recs )
			if ( rec->m_shot )
				return rec;

		return std::nullopt;
	}

	inline int yaw_mode = 0;
	
	bool get_lagcomp_bones( player_t* ent , std::array<matrix3x4_t , 128>& out );

	void reset_data ( int idx );
	void copy_client_layers ( player_t* ent, std::array<animlayer_t, 13>& to, std::array<animlayer_t, 13>& from );
	void update_anims ( player_t* ent, vec3_t& angles, bool force_feet_yaw = false );
	void update_all_anims( player_t* ent , vec3_t& angles, anim_info_t& to, std::array<animlayer_t, 13>& cur_layers, bool should_desync, bool build_matrix );
	bool fix_velocity ( player_t* ent, vec3_t& vel, const std::array<animlayer_t, 13>& animlayers, const vec3_t& origin );
	void update_from ( player_t* ent, anim_info_t& from, anim_info_t& to, std::array<animlayer_t, 13>& cur_layers );
	void apply_anims ( player_t* ent );

	void pre_fsn ( int stage );
	void fsn ( int stage );
}