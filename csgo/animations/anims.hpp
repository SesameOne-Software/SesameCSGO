#pragma once
#include <deque>
#include <array>
#include <optional>
#include "../sdk/sdk.hpp"

namespace anims {
	inline float lerp_time ( ) {
		auto ud_rate = static_cast< float >( g::cvars::cl_updaterate->get_int ( ) );

		if ( g::cvars::sv_minupdaterate && g::cvars::sv_maxupdaterate )
			ud_rate = static_cast< float >( g::cvars::sv_maxupdaterate->get_int ( ) );

		auto ratio = g::cvars::cl_interp_ratio->get_float ( );

		if ( ratio == 0 )
			ratio = 1.0f;

		if ( g::cvars::sv_client_min_interp_ratio && g::cvars::sv_client_max_interp_ratio && g::cvars::sv_client_min_interp_ratio->get_float ( ) != 1 )
			ratio = std::clamp ( ratio, g::cvars::sv_client_min_interp_ratio->get_float ( ), g::cvars::sv_client_max_interp_ratio->get_float ( ) );

		return std::max<float> ( g::cvars::cl_interp->get_float ( ), ratio / ud_rate );
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

		inline bool valid ( ) {
			const auto nci = cs::i::engine->get_net_channel_info ( );

			if ( !nci || !g::local || m_invalid /*|| m_simtime < static_cast<int>(cs::ticks2time( g::local->tick_base( ) ) - g::cvars::sv_maxunlag->get_float( ) )*/ )
				return false;

			const auto lerp = lerp_time ( );
			const auto correct = std::clamp ( nci->get_latency ( 0 ) + nci->get_latency ( 1 ) + lerp, 0.0f, g::cvars::sv_maxunlag->get_float ( ) );

			return abs ( correct - ( cs::ticks2time ( g::local->tick_base ( ) ) - m_simtime ) ) <= 0.2f;
		}

		anim_info_t ( ) {

		}

		anim_info_t ( player_t* ent ) {
			m_invalid = false;
			m_shot = ent->weapon() && ent->weapon( )->last_shot_time() > ent->old_simtime() && ent->weapon( )->last_shot_time( ) <= ent->simtime();
			m_angles = ent->angles ( );
			m_origin = ent->origin ( );
			m_mins = ent->mins ( );
			m_maxs = ent->maxs ( );
			m_lby = ent->lby ( );
			m_simtime = ent->simtime ( );
			m_old_simtime = ent->old_simtime ( );
			m_duck_amount = ent->crouch_amount( );
			m_choked_commands = std::clamp ( cs::time2ticks ( ent->simtime ( ) - ent->old_simtime ( ) ) - 1, 0, g::cvars::sv_maxusrcmdprocessticks->get_int() );
			m_flags = ent->flags ( );
			m_vel = ent->vel( );

			for ( auto& anim_layers : m_anim_layers )
				memcpy( anim_layers.data( ) , ent->layers( ) , sizeof( anim_layers ) );

			for (auto& cur_pose : m_poses )
				cur_pose = ent->poses( );

			for ( auto& abs_angles : m_abs_angles )
				abs_angles = ent->abs_angles( );

			for ( auto& anim_state : m_anim_state )
				anim_state = *ent->animstate( );

			for ( auto& aim_bones : m_aim_bones )
				memcpy( aim_bones.data( ) , ent->bone_cache( ) , ent->bone_count( ) * sizeof( matrix3x4_t ) );

			m_side = desync_middle;
		}
	};

	inline std::array< int, 65> shot_count { 0 };
	inline std::array< std::deque< anim_info_t >, 65> anim_info { {} };
	inline std::array< matrix3x4_t, 128 > real_matrix { {} };
	inline std::array< matrix3x4_t, 128 > fake_matrix { {} };
	inline flags_t createmove_flags;

	void on_net_update_end ( int idx );
	void on_render_start ( int idx );

	void manage_fake ( );

	bool build_bones( player_t* target , matrix3x4_t* mat , int mask , vec3_t rotation , vec3_t origin , float time , std::array<float , 24>& poses );
	
	inline std::deque<anim_info_t> get_lagcomp_records( player_t* ent ) {
		if ( !ent->valid( ) || !ent->idx() || ent->idx() > cs::i::globals->m_max_clients || anim_info[ ent->idx( ) ].empty( ) )
			return {};

		std::deque<anim_info_t> records {};

		for ( auto& rec : anim_info[ ent->idx( ) ] )
			if ( rec.valid( ) )
				records.push_back( rec );

		return records;
	}

	inline std::optional<anim_info_t> get_simulated_record( player_t* ent ) {
		if ( !ent->valid( ) || !ent->idx( ) || ent->idx( ) > cs::i::globals->m_max_clients || anim_info[ ent->idx( ) ].empty( ) )
			return std::nullopt;

		return std::nullopt;

		/* change later, this is a placeholder */
		return anim_info[ ent->idx( ) ].front( );
	}
	
	inline std::optional<anim_info_t> get_onshot( const std::deque<anim_info_t>& recs ) {
		if ( recs.empty() )
			return std::nullopt;

		for ( auto& rec : recs )
			if ( rec.m_shot )
				return rec;

		return std::nullopt;
	}

	bool get_lagcomp_bones( player_t* ent , std::array<matrix3x4_t , 128>& out );

	float angle_diff( float dst , float src );
	void update_local_poses( player_t* ent );
	void calc_poses( player_t* ent , std::array<float , 24>& poses , float feet_yaw );
	void simulate_movement( player_t* ent , flags_t& flags , vec3_t& origin , vec3_t& vel, flags_t& old_flags );

	void reset_data ( int idx );
	void update_anims ( player_t* ent, vec3_t& angles );
	void update_all_anims( player_t* ent , vec3_t& angles, anim_info_t& to, bool build_matrix );
	void fix_velocity ( player_t* ent, vec3_t& vel );
	void update_from ( player_t* ent, const anim_info_t& from, anim_info_t& to );
	void apply_anims ( player_t* ent );

	void pre_fsn ( int stage );
	void fsn ( int stage );
}