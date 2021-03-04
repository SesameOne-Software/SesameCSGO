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

	inline float server_time = 0.0f;

	struct anim_info_t {
		bool m_shot, m_predicted;
		vec3_t m_angles;
		vec3_t m_origin;
		vec3_t m_mins;
		vec3_t m_maxs;
		float m_lby;
		float m_simtime;
		float m_old_simtime;
		int m_choked_commands;
		flags_t m_flags;
		std::array<animlayer_t, 13> m_anim_layers;
		std::array<float, 24> m_poses;
		vec3_t m_vel;
		vec3_t m_abs_angles;
		animstate_t m_anim_state;
		std::array< matrix3x4_t, 128 > m_aim_bones;

		inline bool valid ( ) {
			const auto nci = cs::i::engine->get_net_channel_info ( );

			if ( !nci || !g::local )
				return false;

			const auto lerp = lerp_time ( );
			const auto correct = std::clamp ( nci->get_latency ( 0 ) + nci->get_latency ( 1 ) + lerp, 0.0f, g::cvars::sv_maxunlag->get_float ( ) );

			return abs ( correct - ( cs::ticks2time ( g::local->tick_base ( ) ) - m_simtime ) ) <= 0.2f;
		}

		anim_info_t ( ) {

		}

		anim_info_t ( player_t* ent, float feet_yaw ) {
			m_shot = ent->weapon ( ) && abs( ent->angles ( ).x ) < 70.0f;
			m_predicted = false;
			m_angles = ent->angles ( );
			m_origin = ent->origin ( );
			m_mins = ent->mins ( );
			m_maxs = ent->maxs ( );
			m_lby = ent->lby ( );
			m_simtime = ent->simtime ( );
			m_old_simtime = ent->old_simtime ( );
			m_choked_commands = std::clamp ( cs::time2ticks ( ent->simtime ( ) - ent->old_simtime ( ) ) - 1, 0, g::cvars::sv_maxusrcmdprocessticks->get_int() );
			m_flags = ent->flags ( );
			memcpy ( m_anim_layers.data ( ), ent->layers ( ), sizeof ( m_anim_layers ) );
			m_poses = ent->poses ( );
			m_vel = ent->vel ( );
			m_abs_angles = vec3_t( 0.0f, feet_yaw, 0.0f );
			m_anim_state = *ent->animstate ( );
		}
	};

	inline std::array< int, 65> shot_count { 0 };
	inline std::array< std::deque< anim_info_t >, 65> anim_info { {} };
	inline std::array< std::deque< anim_info_t >, 65> predicted_anim_info { {} };
	inline std::array< float, 65> last_update_time { 0.0f };
	inline std::array< bool, 65> has_been_predicted { false };
	inline std::array< matrix3x4_t, 128 > real_matrix { {} };
	inline std::array< matrix3x4_t, 128 > fake_matrix { {} };

	void on_net_update_end ( int idx );
	void on_render_start ( int idx );

	void manage_fake ( );

	bool build_bones( player_t* target , matrix3x4_t* mat , int mask , vec3_t rotation , vec3_t origin , float time , std::array<float , 24>& poses );
	void reset_data ( int idx );
	void update_anims ( player_t* ent, vec3_t& angles, bool resolve, std::array< matrix3x4_t, 128 >* bones_out = nullptr, bool update_anim_layers = false );
	void fix_velocity ( player_t* ent, vec3_t& vel );
	void process_networked_anims ( player_t* ent );
	void process_predicted_anims ( player_t* ent, bool resolve );
	void apply_anims ( player_t* ent );
	bool get_lagcomp_bones ( player_t* ent, std::array<matrix3x4_t, 128>& out );

	inline std::deque< anim_info_t > get_lagcomp_records ( player_t* ent ) {
		if ( !g::local || !ent->valid ( ) || anim_info [ ent->idx ( ) ].empty ( ) )
			return { };

		std::deque< anim_info_t > ret { };

		for ( auto& rec : anim_info [ ent->idx ( ) ] )
			if ( rec.valid() )
				ret.push_back ( rec );

		return ret;
	}

	inline std::optional<anim_info_t> get_simulated_record ( player_t* ent ) {
		return std::nullopt;
		if ( !g::local || !ent->valid ( ) || predicted_anim_info [ ent->idx ( ) ].empty ( ) || anim_info [ ent->idx ( ) ].empty ( ) )
			return std::nullopt;

		for ( auto& rec : predicted_anim_info [ ent->idx ( ) ] )
			if ( rec.valid ( ) )
				return rec;

		return std::nullopt;
	}

	inline std::optional<anim_info_t> get_onshot ( const std::deque< anim_info_t >& recs ) {
		if ( !g::local || recs.empty ( ) )
			return std::nullopt;

		for ( auto& rec : recs )
			if ( rec.m_shot )
				return rec;

		return std::nullopt;
	}

	void pre_fsn ( int stage );
	void fsn ( int stage );
}