#pragma once
#include <deque>
#include <array>
#include <optional>

#include "../sdk/sdk.hpp"

namespace anims {
	inline std::array<animlayer_t, 64> layer3 = { };
	inline std::array<float, 64> last_moving_lby = { };
	inline std::array<float, 64> last_moving_time = { };
	inline std::array<float, 64> last_moving_lby_time = { };
	inline std::array<float, 64> last_lby = { };
	inline std::array<float, 64> next_lby_update_time = { };
	inline std::array<float, 64> last_freestanding = { };
	inline std::array<float, 64> last_freestand_time = { };
	inline std::array<bool, 64> triggered_balance_adjust = { };
	inline std::array<bool, 64> has_real_jitter = { };

	enum ResolveMode {
		None = 0,
		LBY,
		PositiveLBY,
		NegativeLBY,
		LowLBY,
		Freestand,
		LastMovingLBY,
		Backwards,
	};

	inline const char* resolver_mode_names [ ] = {
		"None",
		"LBY",
		"PositiveLBY",
		"NegativeLBY",
		"LowLBY",
		"Freestand",
		"LastMovingLBY",
		"Backwards",
	};

	inline std::array<int, 64> resolver_mode = { };
	inline std::array<float, 64> last_last_moving_lby = { };
	inline std::array<int, 64> lby_updates_within_jitter_range = { };

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

	struct anim_info_t {
		bool m_shot, m_invalid;
		bool m_forward_track;
		bool m_resolved;
		bool m_has_vel;
		bool m_shifted;
		bool m_lby_update;
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
		std::array<animlayer_t, 13> m_anim_layers;
		std::array<float, 24> m_poses;
		vec3_t m_abs_angles;
		animstate_t m_anim_state;
		std::array< matrix3x4_t, 128 > m_aim_bones;

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

	void resolve_player ( player_t* player, bool& lby_updated_out );
	void recalc_poses ( std::array<float, 24>& poses, float ladder_yaw, float move_yaw, float eye_yaw, float feet_yaw );

	void on_net_update_end ( int idx );
	void on_render_start ( int idx );

	void manage_fake ( );

	bool build_bones( player_t* target , matrix3x4_t* mat , int mask , vec3_t rotation , vec3_t origin , float time , std::array<float , 24>& poses );
	
	inline std::deque<anim_info_t*> get_lagcomp_records( player_t* ent ) {
		if ( !ent->valid( ) || !ent->idx() || ent->idx() > cs::i::globals->m_max_clients || lagcomp_track [ ent->idx( ) ].empty( ) )
			return {};

		//return { lagcomp_track [ ent->idx ( ) ].front ( ) };
		std::deque<anim_info_t*> records {};
		
		for ( auto& rec : lagcomp_track [ ent->idx( ) ] )
			if ( rec->valid( ) && !rec->m_forward_track )
				records.push_back( rec );
		
		return records;
	}

	inline std::optional<anim_info_t*> get_simulated_record( player_t* ent ) {
		if ( !ent->valid( ) || !ent->idx( ) || ent->idx( ) > cs::i::globals->m_max_clients || lagcomp_track [ ent->idx( ) ].empty( ) )
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
	void update_all_anims( player_t* ent , vec3_t& angles, anim_info_t& to, std::array<animlayer_t, 13>& cur_layers );
	bool fix_velocity ( player_t* ent, vec3_t& vel, const std::array<animlayer_t, 13>& animlayers, const vec3_t& origin );
	void update_from ( player_t* ent, anim_info_t& from, anim_info_t& to, std::array<animlayer_t, 13>& cur_layers );
	void apply_anims ( player_t* ent );

	void pre_fsn ( int stage );
	void fsn ( int stage );
}