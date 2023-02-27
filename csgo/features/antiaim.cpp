#include "antiaim.hpp"
#include "../menu/menu.hpp"
#include "../globals.hpp"
#include "prediction.hpp"
#include "ragebot.hpp"
#include "autowall.hpp"
#include "../menu/options.hpp"

#include "../animations/rebuilt.hpp"
#include "../sdk/netvar.hpp"

#include "esp.hpp"

#include "exploits.hpp"

bool features::antiaim::antiaiming = false;

namespace lby {
	float spawn_time = 0.0f;
	float last_breaker_time = 0.0f;
	bool tick_before_update = false;
	bool in_update = false;
	bool broken = false;
	bool balance_update = false;
}

namespace aa {
	float last_flip_angle = 0.0f;
	float last_flip_time = 0.0f;
	bool extended_last_tick = false;
	bool was_on_ground = true;
	bool flip = false;
	bool was_fd = false;
	bool move_flip = false;
	bool jitter_flip = false;

	int old_lag_air = 0;
	int old_lag_move = 0;
	int old_lag_slow_walk = 0;
	int old_lag_stand = 0;
}

void features::antiaim::simulate_lby ( ) {
	lby::in_update = false;

	if ( !g::local->valid ( ) ) {
		return;
	}

	const auto state = g::local->animstate ( );

	if ( !state )
		return;

	if ( g::local->spawn_time ( ) != lby::spawn_time ) {
		lby::spawn_time = g::local->spawn_time ( );
		lby::last_breaker_time = lby::spawn_time;
	}

	if ( g::local->vel ( ).length_2d ( ) > 0.1f )
		lby::last_breaker_time = cs::i::globals->m_curtime + 0.22f;
	else if ( cs::i::globals->m_curtime >= lby::last_breaker_time ) {
		lby::in_update = true;
		lby::last_breaker_time = cs::i::globals->m_curtime + 1.1f;
	}
}

player_t* looking_at ( bool& hittable ) {
	hittable = false;

	player_t* at_target = nullptr;
	player_t* closest_to_crosshair = nullptr;

	std::deque<player_t*> attackers {};

	auto best_target_fov = std::numeric_limits<float>::max ( );
	auto best_fov = std::numeric_limits<float>::max ( );

	vec3_t angs;
	cs::i::engine->get_viewangles ( angs );
	cs::clamp ( angs );

	for ( auto i = 0; i < cs::i::globals->m_max_clients; i++ ) {
		const auto ent = cs::i::ent_list->get<player_t*> ( i );

		if ( ent && ent->is_player ( ) && ent->alive ( ) && !ent->immune ( ) && g::local->is_enemy(ent) ) {
			auto calc_alpha = [ & ] ( float time, float fade_time, bool add = false ) {
				auto dormant_time = std::max< float > ( 9.0f/*esp_fade_time*/, 0.1f );
				return ( std::clamp< float > ( dormant_time - ( std::clamp< float > ( add ? ( dormant_time - std::clamp< float > ( std::fabsf ( ( g::local ? cs::ticks2time ( g::local->tick_base ( ) ) : cs::i::globals->m_curtime ) - time ), 0.0f, dormant_time ) ) : std::fabsf ( cs::i::globals->m_curtime - time ), std::max< float > ( dormant_time - fade_time, 0.0f ), dormant_time ) ), 0.0f, fade_time ) / fade_time );
			};

			int box_alpha = 0;
			if ( !features::esp::esp_data [ i ].m_dormant )
				box_alpha = calc_alpha ( features::esp::esp_data [ i ].m_first_seen, 0.6f, true );
			else
				box_alpha = calc_alpha ( features::esp::esp_data [ i ].m_last_seen, 2.0f );

			if ( box_alpha > 0 ) {
				auto angle_to = cs::calc_angle ( g::local->origin ( ), features::esp::esp_data [ i ].m_pos );
				cs::clamp ( angle_to );
				auto fov = cs::calc_fov ( angle_to, angs );

				if ( fov < best_fov ) {
					closest_to_crosshair = ent;
					best_fov = fov;
				}

				const auto pred_ent_pos = features::esp::esp_data [ i ].m_pos + ent->view_offset ( ) + ent->vel ( ) * std::clamp ( ent->simtime ( ) - ent->old_simtime ( ), 0.0f, cs::ticks2time ( 16 ) );
				const auto cross = cs::angle_vec ( cs::calc_angle ( g::local->eyes ( ), pred_ent_pos ) ).cross_product ( vec3_t ( 0.0f, 0.0f, 1.0f ) );
				const auto src = g::local->eyes ( ) + features::prediction::vel * cs::ticks2time ( 2 );

				const auto dmg_l = autowall::dmg ( g::local, ent, src, pred_ent_pos + cross * 35.0f, hitbox_t::head );
				const auto dmg_r = autowall::dmg ( g::local, ent, src, pred_ent_pos - cross * 35.0f, hitbox_t::head );

				if ( dmg_l > 0.0f || dmg_r > 0.0f ) {
					if ( fov < best_target_fov ) {
						at_target = ent;
						best_target_fov = fov;
					}

					if ( !ent->dormant ( ) )
						hittable = true;

					break;
				}
			}
		}
	}

	return at_target ? at_target : closest_to_crosshair;
}

int find_freestand_side ( player_t* pl, float range ) {
	const auto cross = cs::angle_vec ( cs::calc_angle ( g::local->origin ( ) + vec3_t ( 0.0f, 0.0f, 64.0f ), pl->origin ( ) + vec3_t ( 0.0f, 0.0f, 64.0f ) ) ).cross_product ( vec3_t ( 0.0f, 0.0f, 1.0f ) );

	const auto src = g::local->origin ( ) + vec3_t ( 0.0f, 0.0f, 64.0f );
	const auto dst = pl->origin ( ) + pl->vel ( ) * ( cs::i::globals->m_curtime - pl->simtime ( ) ) + vec3_t ( 0.0f, 0.0f, 64.0f );

	const auto l_dmg1 = autowall::dmg ( g::local, pl, src + cross * range, dst + cross * range, hitbox_t::head );
	const auto r_dmg1 = autowall::dmg ( g::local, pl, src - cross * range, dst - cross * range, hitbox_t::head );

	const auto l_dmg2 = autowall::dmg ( g::local, pl, src + cross * ( range * 0.5f ), dst + cross * ( range * 0.5f ), hitbox_t::head );
	const auto r_dmg2 = autowall::dmg ( g::local, pl, src - cross * ( range * 0.5f ), dst - cross * ( range * 0.5f ), hitbox_t::head );

	const auto l_dmg = std::max ( l_dmg1, l_dmg2 );
	const auto r_dmg = std::max ( r_dmg1, r_dmg2 );

	bool hit_wall = false;

	for ( float i = 0.0f; i < cs::pi * 2.0f; i += cs::pi * 0.25f ) {
		vec3_t location ( src.x + cos ( i ) * 64.0f, src.y + sin ( i ) * 64.0f, src.z );

		ray_t ray;
		trace_t tr;
		trace_filter_t filter;

		filter.m_skip = g::local;
		ray.init ( src, location );

		cs::i::trace->trace_ray ( ray, mask_shot & ~contents_monster & ~contents_hitbox, &filter, &tr );

		if ( tr.m_fraction != 1.0f ) {
			hit_wall = true;
			break;
		}
	}

	if ( l_dmg * r_dmg != 0.0f || ( l_dmg == 0.0f && r_dmg == 0.0f ) || !hit_wall )
		return -1;

	return ( l_dmg > r_dmg ) ? 0 : 1;
}

void features::antiaim::slow_walk ( ucmd_t* cmd, vec3_t& old_angs ) {
	static auto& slowwalk_key = options::vars [ _ ( "antiaim.slow_walk_key" ) ].val.i;
	static auto& slowwalk_key_mode = options::vars [ _ ( "antiaim.slow_walk_key_mode" ) ].val.i;
	static auto& slow_walk_speed = options::vars [ _ ( "antiaim.slow_walk_speed" ) ].val.f;
	static auto& fakewalk = options::vars [ _ ( "antiaim.fakewalk" ) ].val.b;

	auto approach_speed = [ & ] ( float target_speed ) {
		const auto vec_move = vec3_t ( cmd->m_fmove, cmd->m_smove, cmd->m_umove );

		if ( !target_speed ) {
			if ( prediction::vel.length_2d ( ) <= 13.0f ) {
				cmd->m_fmove = cmd->m_smove = 0.0f;
			}
			else {
				auto as_ang = cs::vec_angle ( vec_move );
				as_ang.y = cs::normalize ( as_ang.y + 180.0f );
				const auto inverted_move = cs::angle_vec ( as_ang );

				cmd->m_fmove = inverted_move.x * g::cvars::cl_forwardspeed->get_float ( );
				cmd->m_smove = inverted_move.y * g::cvars::cl_forwardspeed->get_float ( );
			}
		}
		else if ( abs ( cmd->m_fmove ) > 3.0f || abs ( cmd->m_smove ) > 3.0f ) {
			const auto magnitude = vec_move.length_2d ( );

			cmd->m_fmove = ( cmd->m_fmove / magnitude ) * target_speed;
			cmd->m_smove = ( cmd->m_smove / magnitude ) * target_speed;
		}

		cmd->m_buttons &= ~buttons_t::walk;
		//cmd->m_buttons |= buttons_t::speed;
	};

	auto max_speed = 260.0f;
	const auto weapon = g::local->weapon ( );

	if ( weapon && weapon->data ( ) )
		max_speed = g::local->scoped ( ) ? weapon->data ( )->m_max_speed_alt : weapon->data ( )->m_max_speed;

	auto abs_max_speed = max_speed;

	if ( !!( g::local->flags ( ) & flags_t::ducking ) )
		abs_max_speed *= 0.34f;
	else if ( g::local->is_walking() )
		abs_max_speed *= 0.52f;

	//max_speed = std::min ( max_speed, deployable_limited_max_speed ( g::local ) );

	if ( utils::keybind_active ( slowwalk_key, slowwalk_key_mode ) && g::local->weapon ( ) && g::local->weapon ( )->data ( ) ) {
		if ( fakewalk && !cs::is_valve_server ( ) ) {
			if ( cs::i::client_state->choked ( ) > 7 )
				approach_speed ( 0.0f );
		}
		else {
			auto target_speed = max_speed * 0.33f * ( slow_walk_speed / 100.0f );

			/* tiny slowwalk */
			if ( abs ( cmd->m_fmove ) > 3.3f || abs ( cmd->m_smove ) > 3.3f )
				approach_speed ( abs( target_speed - fmodf ( cs::i::globals->m_curtime * ( target_speed * 0.42f * 4.0f ), target_speed * 0.42f ) ) );
		}
	}
	else if ( !!( g::local->flags ( ) & flags_t::on_ground ) && ( abs ( cmd->m_fmove ) > 3.3f || abs ( cmd->m_smove ) > 3.3f ) ) {
		//approach_speed ( abs( abs_max_speed - fmodf ( cs::i::globals->m_curtime * ( abs_max_speed * 0.25f * 4.0f ), abs_max_speed * 0.25f ) ));

		float v14 = ( ( cmd->m_cmdnum % 10000 ) * cs::ticks2time ( 1 ) ) * 5.0f;
		float v10 = abs ( ( v14 - floor ( v14 ) ) - 0.5f );

		approach_speed ( ( abs_max_speed - 1.0f ) - ( ( v10 + v10 ) * 10.0f ) );
	}
}

int ducked_ticks = 0;

void features::antiaim::fakelag ( ucmd_t* ucmd, player_t* target ) {
	static auto& fakelag_enabled = options::vars [ _ ( "antiaim.fakelag" ) ].val.b;
	static auto& fakelag_limit = options::vars [ _ ( "antiaim.fakelag_limit" ) ].val.i;
	static auto& fakelag_jitter = options::vars [ _ ( "antiaim.fakelag_jitter" ) ].val.i;
	static auto& fakelag_trigger_limit = options::vars [ _ ( "antiaim.fakelag_trigger_limit" ) ].val.i;
	static auto& fakelag_triggers = options::vars [ _ ( "antiaim.fakelag_triggers" ) ].val.l;

	static auto& fd_enabled = options::vars [ _ ( "antiaim.fakeduck" ) ].val.b;
	static auto& fd_key = options::vars [ _ ( "antiaim.fakeduck_key" ) ].val.i;
	static auto& fd_key_mode = options::vars [ _ ( "antiaim.fakeduck_key_mode" ) ].val.i;

	static auto& slowwalk_key = options::vars [ _ ( "antiaim.slow_walk_key" ) ].val.i;
	static auto& slowwalk_key_mode = options::vars [ _ ( "antiaim.slow_walk_key_mode" ) ].val.i;
	static auto& slow_walk_speed = options::vars [ _ ( "antiaim.slow_walk_speed" ) ].val.f;
	static auto& fakewalk = options::vars [ _ ( "antiaim.fakewalk" ) ].val.b;

	const auto choke_limit = cs::is_valve_server ( ) ? 8 : g::cvars::sv_maxusrcmdprocessticks->get_int ( );

	auto final_shift_amount_max = std::clamp<int> ( static_cast< int >( features::ragebot::active_config.max_dt_ticks ), 0, g::cvars::sv_maxusrcmdprocessticks->get_int ( ) - 1 );

	if ( !features::ragebot::active_config.dt_enabled || !utils::keybind_active ( features::ragebot::active_config.dt_key, features::ragebot::active_config.dt_key_mode ) )
		final_shift_amount_max = 0;

	auto max_lag = std::clamp< int > ( choke_limit, 1, choke_limit - final_shift_amount_max );

	if ( fd_enabled && utils::keybind_active ( fd_key, fd_key_mode ) )
		max_lag = cs::is_valve_server ( ) ? 8 : 16;
	else if ( fakewalk && utils::keybind_active ( slowwalk_key, slowwalk_key_mode ) && !cs::is_valve_server ( ) )
		max_lag = 14;

	static bool last_on_ground = true;
	static int next_choke = fakelag_limit;
	static weapon_t* last_weapon = nullptr;
	static vec3_t last_vel = vec3_t ( 0.0f, 0.0f, 0.0f );

	if ( !fakelag_enabled || !g::local || !g::local->alive ( ) ) {
		g::send_packet = cs::i::client_state->choked ( ) >= ( ( !g::local || !g::local->alive ( ) ) ? 0 : 1 );

		next_choke = fakelag_limit;
		last_on_ground = true;
		last_weapon = nullptr;
		last_vel = vec3_t ( 0.0f, 0.0f, 0.0f );
		g::just_shot = false;

		return;
	}

	const auto is_reloading = g::local->weapon ( ) && *reinterpret_cast< bool* >( reinterpret_cast< uintptr_t >( &g::local->weapon ( )->next_primary_attack ( ) ) + 0x6D );

	/* select new choke amount every choke cycle */
	if ( !cs::i::client_state->choked ( ) )
		next_choke = std::clamp ( fakelag_limit - static_cast< int >( ( static_cast< float >( fakelag_jitter ) / 100.0f * static_cast< float >( fakelag_limit ) ) * ( static_cast< float >( rand ( ) ) / static_cast< float >( RAND_MAX ) ) ), 1, max_lag );

	/* fakelag triggers */
	const auto trigger_lag_clamped = std::clamp ( fakelag_trigger_limit, 1, max_lag );

	if ( fakelag_triggers [ 0 /* in air */ ]
		&& !( g::local->flags ( ) & flags_t::on_ground ) )
		next_choke = trigger_lag_clamped;

	/* on peek */
	if ( fakelag_triggers [ 1 /* on peek */ ] && target ) {
		const auto pred_ent_pos = target->origin ( ) + target->view_offset ( ) + target->vel ( ) * std::clamp ( target->simtime ( ) - target->old_simtime ( ), cs::ticks2time ( 1 ), cs::ticks2time ( 16 ) );
		const auto cross = cs::angle_vec ( cs::calc_angle ( g::local->eyes ( ), pred_ent_pos ) ).cross_product ( vec3_t ( 0.0f, 0.0f, 1.0f ) );
		const auto src = g::local->eyes ( ) + prediction::vel * cs::ticks2time ( 1 );

		const auto dmg_l = autowall::dmg ( g::local, target, src + cross * 12.0f, pred_ent_pos, hitbox_t::head );
		const auto dmg_r = autowall::dmg ( g::local, target, src - cross * 12.0f, pred_ent_pos, hitbox_t::head );

		if ( dmg_l > 0.0f || dmg_r > 0.0f )
			next_choke = trigger_lag_clamped;
	}

	if ( fakelag_triggers [ 2 /* on shot */ ]
		&& g::just_shot )
		next_choke = trigger_lag_clamped;

	g::just_shot = false;

	if ( fakelag_triggers [ 3 /* on land */ ]
		&& !last_on_ground && !!( g::local->flags ( ) & flags_t::on_ground ) )
		next_choke = trigger_lag_clamped;

	last_on_ground = !!( g::local->flags ( ) & flags_t::on_ground );

	if ( fakelag_triggers [ 4 /* while reloading */ ]
		&& is_reloading )
		next_choke = trigger_lag_clamped;

	if ( fakelag_triggers [ 5 /* on weapon switch */ ]
		&& last_weapon != g::local->weapon ( ) )
		next_choke = trigger_lag_clamped;

	last_weapon = g::local->weapon ( );

	if ( fakelag_triggers [ 6 /* on velocity change */ ]
		&& !!( g::local->flags ( ) & flags_t::on_ground )
		&& prediction::vel.length_2d ( ) > 5.0f
		&& abs ( cs::normalize ( cs::vec_angle ( prediction::vel ).y - cs::vec_angle ( last_vel ).y ) ) > 10.0f )
		next_choke = trigger_lag_clamped;

	last_vel = prediction::vel;

	if ( fakelag_triggers [ 7 /* break lagcomp */ ]
		&& prediction::vel.length_2d ( ) > 5.0f
		&& trigger_lag_clamped >= static_cast< int >( ceil ( 64.0f / ( prediction::vel.length_2d ( ) * cs::ticks2time ( 1 ) ) ) ) )
		next_choke = trigger_lag_clamped;

	g::send_packet = cs::i::client_state->choked ( ) >= next_choke;
}

void features::antiaim::run ( ucmd_t* ucmd, bool was_shooting, vec3_t& old_angs ) {
	/* toggle */
	static auto& air = options::vars [ _ ( "antiaim.air.enabled" ) ].val.b;
	static auto& move = options::vars [ _ ( "antiaim.moving.enabled" ) ].val.b;
	static auto& stand = options::vars [ _ ( "antiaim.standing.enabled" ) ].val.b;
	static auto& slow_walk = options::vars [ _ ( "antiaim.slow_walk.enabled" ) ].val.b;

	/* desync */
	static auto& desync_air = options::vars [ _ ( "antiaim.air.desync" ) ].val.b;
	static auto& desync_move = options::vars [ _ ( "antiaim.moving.desync" ) ].val.b;
	static auto& desync_stand = options::vars [ _ ( "antiaim.standing.desync" ) ].val.b;
	static auto& desync_slow_walk = options::vars [ _ ( "antiaim.slow_walk.desync" ) ].val.b;

	/* desync side */
	static auto& desync_side_air = options::vars [ _ ( "antiaim.air.invert_initial_side" ) ].val.b;
	static auto& desync_side_move = options::vars [ _ ( "antiaim.moving.invert_initial_side" ) ].val.b;
	static auto& desync_side_stand = options::vars [ _ ( "antiaim.standing.invert_initial_side" ) ].val.b;
	static auto& desync_side_slow_walk = options::vars [ _ ( "antiaim.slow_walk.invert_initial_side" ) ].val.b;

	/* fake lag */
	static auto& lag_air = options::vars [ _ ( "antiaim.air.fakelag_factor" ) ].val.i;
	static auto& lag_move = options::vars [ _ ( "antiaim.moving.fakelag_factor" ) ].val.i;
	static auto& lag_stand = options::vars [ _ ( "antiaim.standing.fakelag_factor" ) ].val.i;
	static auto& lag_slow_walk = options::vars [ _ ( "antiaim.slow_walk.fakelag_factor" ) ].val.i;

	/* pitch */
	static auto& pitch_air = options::vars [ _ ( "antiaim.air.pitch" ) ].val.i;
	static auto& pitch_move = options::vars [ _ ( "antiaim.moving.pitch" ) ].val.i;
	static auto& pitch_stand = options::vars [ _ ( "antiaim.standing.pitch" ) ].val.i;
	static auto& pitch_slow_walk = options::vars [ _ ( "antiaim.slow_walk.pitch" ) ].val.i;

	/* base yaw */
	static auto& yaw_offset_air = options::vars [ _ ( "antiaim.air.yaw_offset" ) ].val.f;
	static auto& yaw_offset_move = options::vars [ _ ( "antiaim.moving.yaw_offset" ) ].val.f;
	static auto& yaw_offset_stand = options::vars [ _ ( "antiaim.standing.yaw_offset" ) ].val.f;
	static auto& yaw_offset_slow_walk = options::vars [ _ ( "antiaim.slow_walk.yaw_offset" ) ].val.f;

	/* slow walk settings */
	static auto& slow_walk_speed = options::vars [ _ ( "antiaim.slow_walk_speed" ) ].val.f;
	static auto& fakewalk = options::vars [ _ ( "antiaim.fakewalk" ) ].val.b;

	/* jitter desync */
	static auto& jitter_air = options::vars [ _ ( "antiaim.air.jitter_desync_side" ) ].val.b;
	static auto& jitter_move = options::vars [ _ ( "antiaim.moving.jitter_desync_side" ) ].val.b;
	static auto& jitter_stand = options::vars [ _ ( "antiaim.standing.jitter_desync_side" ) ].val.b;
	static auto& jitter_slow_walk = options::vars [ _ ( "antiaim.slow_walk.jitter_desync_side" ) ].val.b;

	/* desync ranges */
	static auto& desync_amount_air = options::vars [ _ ( "antiaim.air.desync_range" ) ].val.f;
	static auto& desync_amount_move = options::vars [ _ ( "antiaim.moving.desync_range" ) ].val.f;
	static auto& desync_amount_stand = options::vars [ _ ( "antiaim.standing.desync_range" ) ].val.f;
	static auto& desync_amount_slow_walk = options::vars [ _ ( "antiaim.slow_walk.desync_range" ) ].val.f;

	static auto& desync_amount_inverted_air = options::vars [ _ ( "antiaim.air.desync_range_inverted" ) ].val.f;
	static auto& desync_amount_inverted_move = options::vars [ _ ( "antiaim.moving.desync_range_inverted" ) ].val.f;
	static auto& desync_amount_inverted_stand = options::vars [ _ ( "antiaim.standing.desync_range_inverted" ) ].val.f;
	static auto& desync_amount_inverted_slow_walk = options::vars [ _ ( "antiaim.slow_walk.desync_range_inverted" ) ].val.f;

	/* jitter amount */
	static auto& jitter_amount_air = options::vars [ _ ( "antiaim.air.jitter_range" ) ].val.f;
	static auto& jitter_amount_move = options::vars [ _ ( "antiaim.moving.jitter_range" ) ].val.f;
	static auto& jitter_amount_stand = options::vars [ _ ( "antiaim.standing.jitter_range" ) ].val.f;
	static auto& jitter_amount_slow_walk = options::vars [ _ ( "antiaim.slow_walk.jitter_range" ) ].val.f;

	static auto& auto_direction_amount_air = options::vars [ _ ( "antiaim.air.auto_direction_amount" ) ].val.f;
	static auto& auto_direction_amount_move = options::vars [ _ ( "antiaim.moving.auto_direction_amount" ) ].val.f;
	static auto& auto_direction_amount_stand = options::vars [ _ ( "antiaim.standing.auto_direction_amount" ) ].val.f;
	static auto& auto_direction_amount_slow_walk = options::vars [ _ ( "antiaim.slow_walk.auto_direction_amount" ) ].val.f;

	static auto& auto_direction_range_air = options::vars [ _ ( "antiaim.air.auto_direction_range" ) ].val.f;
	static auto& auto_direction_range_move = options::vars [ _ ( "antiaim.moving.auto_direction_range" ) ].val.f;
	static auto& auto_direction_range_stand = options::vars [ _ ( "antiaim.standing.auto_direction_range" ) ].val.f;
	static auto& auto_direction_range_slow_walk = options::vars [ _ ( "antiaim.slow_walk.auto_direction_range" ) ].val.f;

	static auto& left_key = options::vars [ _ ( "antiaim.manual_left_key" ) ].val.i;
	static auto& right_key = options::vars [ _ ( "antiaim.manual_right_key" ) ].val.i;
	static auto& back_key = options::vars [ _ ( "antiaim.manual_back_key" ) ].val.i;

	static auto& slowwalk_key = options::vars [ _ ( "antiaim.slow_walk_key" ) ].val.i;
	static auto& slowwalk_key_mode = options::vars [ _ ( "antiaim.slow_walk_key_mode" ) ].val.i;

	static auto& fd_enabled = options::vars [ _ ( "antiaim.fakeduck" ) ].val.b;
	static auto& fd_key = options::vars [ _ ( "antiaim.fakeduck_key" ) ].val.i;
	static auto& fd_key_mode = options::vars [ _ ( "antiaim.fakeduck_key_mode" ) ].val.i;

	static auto& desync_flip_key = options::vars [ _ ( "antiaim.desync_invert_key" ) ].val.i;
	static auto& desync_flip_key_mode = options::vars [ _ ( "antiaim.desync_invert_key_mode" ) ].val.i;

	static auto& center_real_air = options::vars [ _ ( "antiaim.air.center_real" ) ].val.b;
	static auto& center_real_move = options::vars [ _ ( "antiaim.moving.center_real" ) ].val.b;
	static auto& center_real_stand = options::vars [ _ ( "antiaim.standing.center_real" ) ].val.b;
	static auto& center_real_slow_walk = options::vars [ _ ( "antiaim.slow_walk.center_real" ) ].val.b;

	static auto& anti_bruteforce_air = options::vars [ _ ( "antiaim.air.anti_bruteforce" ) ].val.b;
	static auto& anti_bruteforce_move = options::vars [ _ ( "antiaim.moving.anti_bruteforce" ) ].val.b;
	static auto& anti_bruteforce_stand = options::vars [ _ ( "antiaim.standing.anti_bruteforce" ) ].val.b;
	static auto& anti_bruteforce_slow_walk = options::vars [ _ ( "antiaim.slow_walk.anti_bruteforce" ) ].val.b;

	static auto& anti_freestand_prediction_air = options::vars [ _ ( "antiaim.air.anti_freestand_prediction" ) ].val.b;
	static auto& anti_freestand_prediction_move = options::vars [ _ ( "antiaim.moving.anti_freestand_prediction" ) ].val.b;
	static auto& anti_freestand_prediction_stand = options::vars [ _ ( "antiaim.standing.anti_freestand_prediction" ) ].val.b;
	static auto& anti_freestand_prediction_slow_walk = options::vars [ _ ( "antiaim.slow_walk.anti_freestand_prediction" ) ].val.b;

	static auto& base_yaw_air = options::vars [ _ ( "antiaim.air.base_yaw" ) ].val.i;
	static auto& base_yaw_move = options::vars [ _ ( "antiaim.moving.base_yaw" ) ].val.i;
	static auto& base_yaw_stand = options::vars [ _ ( "antiaim.standing.base_yaw" ) ].val.i;
	static auto& base_yaw_slow_walk = options::vars [ _ ( "antiaim.slow_walk.base_yaw" ) ].val.i;

	static auto& rotation_range_air = options::vars [ _ ( "antiaim.air.rotation_range" ) ].val.f;
	static auto& rotation_range_move = options::vars [ _ ( "antiaim.moving.rotation_range" ) ].val.f;
	static auto& rotation_range_stand = options::vars [ _ ( "antiaim.standing.rotation_range" ) ].val.f;
	static auto& rotation_range_slow_walk = options::vars [ _ ( "antiaim.slow_walk.rotation_range" ) ].val.f;

	static auto& rotation_speed_air = options::vars [ _ ( "antiaim.air.rotation_speed" ) ].val.f;
	static auto& rotation_speed_move = options::vars [ _ ( "antiaim.moving.rotation_speed" ) ].val.f;
	static auto& rotation_speed_stand = options::vars [ _ ( "antiaim.standing.rotation_speed" ) ].val.f;
	static auto& rotation_speed_slow_walk = options::vars [ _ ( "antiaim.slow_walk.rotation_speed" ) ].val.f;

	static auto& desync_type = options::vars [ _ ( "antiaim.standing.desync_type" ) ].val.i;
	static auto& desync_type_inverted = options::vars [ _ ( "antiaim.standing.desync_type_inverted" ) ].val.i;

	static auto& break_bt = options::vars [ _ ( "antiaim.break_backtrack" ) ].val.b;
	static auto& break_bt_key = options::vars [ _ ( "antiaim.break_backtrack_key" ) ].val.i;
	static auto& break_bt_mode = options::vars [ _ ( "antiaim.break_backtrack_key_mode" ) ].val.i;

	security_handler::update ( );

	const auto choke_limit = g::cvars::sv_maxusrcmdprocessticks->get_int ( );

	if ( !left_key && side == 1 )
		side = -1;

	if ( !right_key && side == 2 )
		side = -1;

	if ( !back_key && side == 0 )
		side = -1;

	if ( !desync_flip_key && desync_side != -1 )
		desync_side = -1;

	if ( utils::keybind_active ( left_key, 0 ) )
		side = 1;

	if ( utils::keybind_active ( right_key, 0 ) )
		side = 2;

	if ( utils::keybind_active ( back_key, 0 ) )
		side = 0;

	if ( desync_flip_key && utils::keybind_active ( desync_flip_key, desync_flip_key_mode ) )
		desync_side = 0;
	else if ( desync_flip_key )
		desync_side = 1;

	/* reset anti-bruteforce when new round starts */
	if ( g::round == round_t::starting )
		for ( auto& shot_count : anims::shot_count )
			shot_count = 0;

	/* manage fakelag */
	auto force_standing_antiaim = false;
	auto found_enemy = false;

	for ( auto i = 0; i < cs::i::globals->m_max_clients; i++ ) {
		const auto player = cs::i::ent_list->get<player_t*> ( i );

		if ( player && player->alive ( ) && g::local->is_enemy( player ) ) {
			found_enemy = true;
			break;
		}
	}

	bool hittable = false;
	const auto target_player = looking_at ( hittable );

	if ( g::local->valid ( ) && ( g::round == round_t::in_progress || ( g::round == round_t::ending /*&& found_enemy*/ ) ) ) {
		static auto last_final_shift_amount = 0;
		auto final_shift_amount_max = static_cast< int >( features::ragebot::active_config.max_dt_ticks );

		if ( !features::ragebot::active_config.dt_enabled || !utils::keybind_active ( features::ragebot::active_config.dt_key, features::ragebot::active_config.dt_key_mode ) )
			final_shift_amount_max = 0;

		/* amount of ticks to shift increased */
		if ( final_shift_amount_max > last_final_shift_amount )
			exploits::force_recharge ( features::ragebot::active_config.max_dt_ticks, 0.0f );

		last_final_shift_amount = final_shift_amount_max;

		/* fakelag */
		fakelag ( ucmd, hittable ? target_player : nullptr );

		auto max_lag = cs::is_valve_server ( ) ? 8 : g::cvars::sv_maxusrcmdprocessticks->get_int ( );

		if ( exploits::will_shift || was_shooting /*|| exploits::has_shifted*/ ) {
			g::send_packet = true;
			g::just_shot = true;
		}

		aa::was_on_ground = !!( g::local->flags ( ) & flags_t::on_ground );

		static bool reached_choke_limit = false;
		if ( fd_enabled && utils::keybind_active ( fd_key, fd_key_mode ) ) {
			aa::was_fd = true;

			if ( cs::i::client_state->choked ( ) < max_lag / 2 )
				ucmd->m_buttons &= ~buttons_t::duck;
			else
				ucmd->m_buttons |= buttons_t::duck;

			if ( cs::i::client_state->choked ( ) >= max_lag )
				reached_choke_limit = true;

			if ( reached_choke_limit ) {
				if ( g::local->crouch_amount ( ) < 1.0f )
					ucmd->m_buttons |= buttons_t::duck;
				else
					reached_choke_limit = false;

				g::send_packet = true;
			}
			else
				g::send_packet = false;

			ucmd->m_buttons |= buttons_t::bullrush;
		}
		else if ( aa::was_fd ) {
			reached_choke_limit = false;
			ducked_ticks = 0;
			aa::was_fd = false;

			if ( features::ragebot::active_config.dt_enabled && utils::keybind_active ( features::ragebot::active_config.dt_key, features::ragebot::active_config.dt_key_mode ) )
				exploits::force_recharge ( features::ragebot::active_config.max_dt_ticks, 0.0f );
		}

		antiaim::slow_walk ( ucmd, old_angs );

		if ( utils::keybind_active ( slowwalk_key, slowwalk_key_mode ) ) {
			if ( fakewalk && !cs::is_valve_server ( ) )
				force_standing_antiaim = true;
		}
	}
	else {
		g::send_packet = true;
	}

	if ( g::send_packet )
		aa::flip = !aa::flip;

	const auto pressing_use = !!( ucmd->m_buttons & buttons_t::use ) && g::local->weapon ( ) && g::local->weapon ( )->item_definition_index ( ) != weapons_t::c4;
	const auto should_shoot = ( exploits::can_shoot ( ) && !!( ucmd->m_buttons & ( buttons_t::attack | ( g::local->weapon ( )->data ( )->m_type == weapon_type_t::knife ? buttons_t::attack2 : static_cast< buttons_t >( 0 ) ) ) ) );

	if ( !g::local->valid ( )
		|| g::local->movetype ( ) == movetypes_t::noclip
		|| g::local->movetype ( ) == movetypes_t::ladder
		|| !g::local->weapon ( )
		|| !g::local->weapon ( )->data ( )
		|| should_shoot
		|| g::round == round_t::starting ) {
		antiaiming = false;

		if ( should_shoot && g::local && g::local->alive ( ) && !aa::was_fd )
			g::send_packet = true;

		return;
	}

	if ( g::local->weapon ( )->data ( )->m_type == weapon_type_t::grenade && g::local->weapon ( )->throw_time ( ) > 0.0f ) {
		antiaiming = false;
		return;
	}

	//update_anti_bruteforce ( );
	auto desync_amnt = ( desync_side || desync_side == -1 ) ? 120.0f : -120.0f;

	auto process_base_yaw = [ & ] ( int base_yaw, float auto_dir_amount, float auto_dir_range ) {
		switch ( base_yaw ) {
		case 0: {
			vec3_t ang;
			cs::i::engine->get_viewangles ( ang );
			ucmd->m_angs.y = ang.y;
		} break;
		case 1: {
			ucmd->m_angs.y = 0.0f;
		} break;
		case 2: {
			if ( target_player ) {
				const auto yaw_to = cs::normalize ( cs::calc_angle ( g::local->origin ( ), target_player->origin ( ) ).y );
				ucmd->m_angs.y = yaw_to;
			}
			else {
				vec3_t ang;
				cs::i::engine->get_viewangles ( ang );
				ucmd->m_angs.y = ang.y;
			}
		} break;
		case 3: {
			if ( target_player ) {
				const auto yaw_to = cs::normalize ( cs::calc_angle ( g::local->origin ( ), target_player->origin ( ) ).y + 180.0f );
				const auto desync_side = find_freestand_side ( target_player, auto_dir_range );

				if ( desync_side != -1 )
					ucmd->m_angs.y = yaw_to + ( desync_side ? -auto_dir_amount : auto_dir_amount );
				else
					ucmd->m_angs.y = yaw_to;
			}
			else {
				vec3_t ang;
				cs::i::engine->get_viewangles ( ang );
				ucmd->m_angs.y = ang.y + 180.0f;
			}
		} break;
		}
	};

	/* manage antiaim */ {
		if ( !( g::local->flags ( ) & flags_t::on_ground ) ) {
			if ( air ) {
				if ( !pressing_use ) {
					process_base_yaw ( base_yaw_air, auto_direction_amount_air, auto_direction_range_air );

					ucmd->m_angs.y += yaw_offset_air;
				}

				if ( side != -1 ) {
					vec3_t ang;
					cs::i::engine->get_viewangles ( ang );

					switch ( side ) {
					case 0: ucmd->m_angs.y = ang.y + 180.0f; break;
					case 1: ucmd->m_angs.y = ang.y + 90.0f; break;
					case 2: ucmd->m_angs.y = ang.y - 90.0f; break;
					}
				}

				if ( !pressing_use ) {
					switch ( pitch_air ) {
					case 0: break;
					case 1: ucmd->m_angs.x = 89.0f; break;
					case 2: ucmd->m_angs.x = -89.0f; break;
					case 3: ucmd->m_angs.x = 0.0f; break;
					}
				}

				if ( target_player && anti_freestand_prediction_air ) {
					const auto desync_side = find_freestand_side ( target_player, auto_direction_range_air );

					if ( desync_side != -1 )
						desync_amnt = !desync_side ? 120.0f : -120.0f;
				}

				if ( anti_bruteforce_air && target_player )
					desync_amnt = ( anims::shot_count [ target_player->idx ( ) ] % 2 ) ? -desync_amnt : desync_amnt;

				if ( desync_air && jitter_air && !g::send_packet ) {
					if ( ( aa::flip ? -desync_amnt : desync_amnt ) > 0.0f )
						desync_amnt *= desync_amount_air / 60.0f;
					else
						desync_amnt *= desync_amount_inverted_air / 60.0f;

					ucmd->m_angs.y += aa::flip ? -desync_amnt : desync_amnt;
				}
				else if ( desync_air && !g::send_packet ) {
					if ( ( desync_side_air ? -desync_amnt : desync_amnt ) > 0.0f )
						desync_amnt *= desync_amount_air / 60.0f;
					else
						desync_amnt *= desync_amount_inverted_air / 60.0f;

					ucmd->m_angs.y += desync_side_air ? -desync_amnt : desync_amnt;
				}

				if ( desync_air && center_real_air ) {
					if ( ( desync_side_air ? desync_amnt : -desync_amnt ) > 0.0f )
						desync_amnt *= desync_amount_air / 60.0f;
					else
						desync_amnt *= desync_amount_inverted_air / 60.0f;

					ucmd->m_angs.y += ( ( ( desync_side_air ? desync_amnt : -desync_amnt ) > 0.0f ) ? g::local->desync_amount ( ) : -g::local->desync_amount ( ) ) * abs ( desync_amnt / 120.0f ) * 0.5f;
				}

				ucmd->m_angs.y += aa::flip ? -jitter_amount_air : jitter_amount_air;

				if ( rotation_range_air )
					ucmd->m_angs.y += fmod ( cs::i::globals->m_curtime * rotation_range_air * rotation_speed_air, rotation_range_air ) - rotation_range_air * 0.5f;

				antiaiming = true;
			}
			else {
				antiaiming = false;
			}
		}
		else if ( prediction::vel.length_2d ( ) > 5.0f && g::local->weapon ( ) && g::local->weapon ( )->data ( ) && !force_standing_antiaim ) {
			if ( utils::keybind_active ( slowwalk_key, slowwalk_key_mode ) && slow_walk ) {
				if ( !pressing_use ) {
					process_base_yaw ( base_yaw_slow_walk, auto_direction_amount_slow_walk, auto_direction_range_slow_walk );

					ucmd->m_angs.y += yaw_offset_slow_walk;
				}

				if ( side != -1 ) {
					vec3_t ang;
					cs::i::engine->get_viewangles ( ang );

					switch ( side ) {
					case 0: ucmd->m_angs.y = ang.y + 180.0f; break;
					case 1: ucmd->m_angs.y = ang.y + 90.0f; break;
					case 2: ucmd->m_angs.y = ang.y - 90.0f; break;
					}
				}

				if ( !pressing_use ) {
					switch ( pitch_slow_walk ) {
					case 0: break;
					case 1: ucmd->m_angs.x = 89.0f; break;
					case 2: ucmd->m_angs.x = -89.0f; break;
					case 3: ucmd->m_angs.x = 0.0f; break;
					}
				}

				if ( target_player && anti_freestand_prediction_slow_walk ) {
					const auto desync_side = find_freestand_side ( target_player, auto_direction_range_slow_walk );

					if ( desync_side != -1 )
						desync_amnt = !desync_side ? 120.0f : -120.0f;
				}

				if ( anti_bruteforce_slow_walk && target_player )
					desync_amnt = ( anims::shot_count [ target_player->idx ( ) ] % 2 ) ? -desync_amnt : desync_amnt;

				if ( desync_slow_walk && jitter_slow_walk && !g::send_packet ) {
					if ( ( aa::flip ? -desync_amnt : desync_amnt ) > 0.0f )
						desync_amnt *= desync_amount_slow_walk / 60.0f;
					else
						desync_amnt *= desync_amount_inverted_slow_walk / 60.0f;

					ucmd->m_angs.y += aa::flip ? -desync_amnt : desync_amnt;
				}
				else if ( desync_slow_walk && !g::send_packet ) {
					if ( ( desync_side_slow_walk ? -desync_amnt : desync_amnt ) > 0.0f )
						desync_amnt *= desync_amount_slow_walk / 60.0f;
					else
						desync_amnt *= desync_amount_inverted_slow_walk / 60.0f;

					ucmd->m_angs.y += desync_side_slow_walk ? -desync_amnt : desync_amnt;
				}

				if ( desync_slow_walk && center_real_slow_walk ) {
					if ( ( desync_side_slow_walk ? desync_amnt : -desync_amnt ) > 0.0f )
						desync_amnt *= desync_amount_slow_walk / 60.0f;
					else
						desync_amnt *= desync_amount_inverted_slow_walk / 60.0f;

					ucmd->m_angs.y += ( ( ( desync_side_slow_walk ? desync_amnt : -desync_amnt ) > 0.0f ) ? g::local->desync_amount ( ) : -g::local->desync_amount ( ) ) * abs ( desync_amnt / 120.0f ) * 0.5f;
				}

				ucmd->m_angs.y += aa::flip ? -jitter_amount_slow_walk : jitter_amount_slow_walk;

				if ( rotation_range_slow_walk )
					ucmd->m_angs.y += fmod ( cs::i::globals->m_curtime * rotation_range_slow_walk * rotation_speed_slow_walk, rotation_range_slow_walk ) - rotation_range_slow_walk * 0.5f;

				antiaiming = true;
			}
			else if ( move ) {
				if ( !pressing_use ) {
					process_base_yaw ( base_yaw_move, auto_direction_amount_move, auto_direction_range_move );

					ucmd->m_angs.y += yaw_offset_move;
				}

				if ( side != -1 ) {
					vec3_t ang;
					cs::i::engine->get_viewangles ( ang );

					switch ( side ) {
					case 0: ucmd->m_angs.y = ang.y + 180.0f; break;
					case 1: ucmd->m_angs.y = ang.y + 90.0f; break;
					case 2: ucmd->m_angs.y = ang.y - 90.0f; break;
					}
				}

				if ( !pressing_use ) {
					switch ( pitch_move ) {
					case 0: break;
					case 1: ucmd->m_angs.x = 89.0f; break;
					case 2: ucmd->m_angs.x = -89.0f; break;
					case 3: ucmd->m_angs.x = 0.0f; break;
					}
				}

				if ( target_player && anti_freestand_prediction_move ) {
					const auto desync_side = find_freestand_side ( target_player, auto_direction_range_move );

					if ( desync_side != -1 )
						desync_amnt = !desync_side ? 120.0f : -120.0f;
				}

				if ( anti_bruteforce_move && target_player )
					desync_amnt = ( anims::shot_count [ target_player->idx ( ) ] % 2 ) ? -desync_amnt : desync_amnt;

				if ( desync_move && jitter_move && !g::send_packet ) {
					if ( ( aa::flip ? -desync_amnt : desync_amnt ) > 0.0f )
						desync_amnt *= desync_amount_move / 60.0f;
					else
						desync_amnt *= desync_amount_inverted_move / 60.0f;

					ucmd->m_angs.y += aa::flip ? -desync_amnt : desync_amnt;
				}
				else if ( desync_move && !g::send_packet ) {
					if ( ( desync_side_move ? -desync_amnt : desync_amnt ) > 0.0f )
						desync_amnt *= desync_amount_move / 60.0f;
					else
						desync_amnt *= desync_amount_inverted_move / 60.0f;

					ucmd->m_angs.y += desync_side_move ? -desync_amnt : desync_amnt;
				}

				if ( desync_move && center_real_move ) {
					if ( ( desync_side_move ? desync_amnt : -desync_amnt ) > 0.0f )
						desync_amnt *= desync_amount_move / 60.0f;
					else
						desync_amnt *= desync_amount_inverted_move / 60.0f;

					ucmd->m_angs.y += ( ( ( desync_side_move ? desync_amnt : -desync_amnt ) > 0.0f ) ? g::local->desync_amount ( ) : -g::local->desync_amount ( ) ) * abs ( desync_amnt / 120.0f ) * 0.5f;
				}

				ucmd->m_angs.y += aa::flip ? -jitter_amount_move : jitter_amount_move;

				if ( rotation_range_move )
					ucmd->m_angs.y += fmod ( cs::i::globals->m_curtime * rotation_range_move * rotation_speed_move, rotation_range_move ) - rotation_range_move * 0.5f;

				antiaiming = true;
			}
			else {
				antiaiming = false;
			}
		}
		else if ( stand ) {
			if ( !pressing_use ) {
				process_base_yaw ( base_yaw_stand, auto_direction_amount_stand, auto_direction_range_stand );

				ucmd->m_angs.y += yaw_offset_stand;
			}

			if ( side != -1 ) {
				vec3_t ang;
				cs::i::engine->get_viewangles ( ang );

				switch ( side ) {
				case 0: ucmd->m_angs.y = ang.y + 180.0f; break;
				case 1: ucmd->m_angs.y = ang.y + 90.0f; break;
				case 2: ucmd->m_angs.y = ang.y - 90.0f; break;
				}
			}

			if ( !pressing_use ) {
				switch ( pitch_stand ) {
				case 0: break;
				case 1: ucmd->m_angs.x = 89.0f; break;
				case 2: ucmd->m_angs.x = -89.0f; break;
				case 3: ucmd->m_angs.x = 0.0f; break;
				}
			}

			if ( target_player && anti_freestand_prediction_stand ) {
				const auto desync_side = find_freestand_side ( target_player, auto_direction_range_stand );

				if ( desync_side != -1 )
					desync_amnt = !desync_side ? 120.0f : -120.0f;
			}

			if ( anti_bruteforce_stand && target_player )
				desync_amnt = ( anims::shot_count [ target_player->idx ( ) ] % 2 ) ? -desync_amnt : desync_amnt;

			if ( desync_stand ) {
				auto selected_desync_type = 0;

				if ( ( desync_side_stand ? -desync_amnt : desync_amnt ) > 0.0f )
					selected_desync_type = desync_type;
				else
					selected_desync_type = desync_type_inverted;

				switch ( selected_desync_type ) {
				case 0: /* real around fake */ {
					if ( g::send_packet ) {
						/* micro movements */
						const auto move_amount = g::local->crouch_amount ( ) > 0.0f ? 3.0f : 1.1f;
						ucmd->m_fmove += aa::move_flip ? -move_amount : move_amount;
						aa::move_flip = !aa::move_flip;
					}

					static float last_update_time = cs::i::globals->m_curtime;

					if ( desync_stand && jitter_stand && !g::send_packet ) {
						if ( ( aa::flip ? -desync_amnt : desync_amnt ) > 0.0f )
							desync_amnt *= desync_amount_stand / 60.0f;
						else
							desync_amnt *= desync_amount_inverted_stand / 60.0f;

						ucmd->m_angs.y += aa::flip ? -desync_amnt : desync_amnt;
					}
					else if ( desync_stand && !g::send_packet ) {
						if ( ( desync_side_stand ? -desync_amnt : desync_amnt ) > 0.0f )
							desync_amnt *= desync_amount_stand / 60.0f;
						else
							desync_amnt *= desync_amount_inverted_stand / 60.0f;

						ucmd->m_angs.y += desync_side_stand ? -desync_amnt : desync_amnt;
					}
				}break;
				case 1: /* fake real around */ {
					if ( jitter_stand ) {
						/* go 180 from update location to trigger 979 activity */
						if ( lby::in_update || !g::send_packet ) {
							ucmd->m_angs.y += aa::flip ? 120.0f : -120.0f;
							g::send_packet = false;
						}
					}
					else {
						desync_amnt /= 120.0f;

						if ( ( desync_side_stand ? -desync_amnt : desync_amnt ) > 0.0f )
							desync_amnt *= desync_amount_stand / 60.0f;
						else
							desync_amnt *= desync_amount_inverted_stand / 60.0f;

						if ( lby::in_update ) {
							ucmd->m_angs.y -= copysignf ( 120.0f, desync_side_stand ? -desync_amnt : desync_amnt );
							g::send_packet = false;
						}
						else if ( !g::send_packet ) {
							ucmd->m_angs.y += copysignf ( abs ( desync_amnt ) * 110.0f, desync_side_stand ? -desync_amnt : desync_amnt );
						}
					}
				}break;
				}
			}

			if ( desync_stand && center_real_stand ) {
				if ( ( desync_side_stand ? desync_amnt : -desync_amnt ) > 0.0f )
					desync_amnt *= desync_amount_stand / 60.0f;
				else
					desync_amnt *= desync_amount_inverted_stand / 60.0f;

				ucmd->m_angs.y += ( ( ( desync_side_stand ? desync_amnt : -desync_amnt ) > 0.0f ) ? g::local->desync_amount ( ) : -g::local->desync_amount ( ) ) * abs ( desync_amnt / 120.0f ) * 0.5f;
			}

			ucmd->m_angs.y += aa::flip ? -jitter_amount_stand : jitter_amount_stand;

			if ( rotation_range_stand )
				ucmd->m_angs.y += fmod ( cs::i::globals->m_curtime * rotation_range_stand * rotation_speed_stand, rotation_range_stand ) - rotation_range_stand * 0.5f;

			antiaiming = true;
		}
		else {
			antiaiming = false;
		}
	}
}