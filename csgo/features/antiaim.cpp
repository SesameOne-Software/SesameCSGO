#include "antiaim.hpp"
#include "../menu/menu.hpp"
#include "../globals.hpp"
#include "prediction.hpp"
#include "ragebot.hpp"
#include "autowall.hpp"
#include "../menu/options.hpp"

extern int g_refresh_counter;

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

	int old_lag_air = 0;
	int old_lag_move = 0;
	int old_lag_slow_walk = 0;
	int old_lag_stand = 0;
}

void features::antiaim::simulate_lby( ) {
	lby::tick_before_update = false;
	lby::in_update = false;
	lby::balance_update = false;

	if ( !g::local->valid( ) ) {
		return;
	}

	const auto state = g::local->animstate( );

	if ( !state )
		return;

	if ( g::local->spawn_time( ) != lby::spawn_time ) {
		lby::spawn_time = g::local->spawn_time( );
		lby::last_breaker_time = lby::spawn_time;
	}

	if ( g::local->vel( ).length_2d( ) > 0.1f )
		lby::last_breaker_time = csgo::i::globals->m_curtime + 0.22f;
	else if ( csgo::i::globals->m_curtime >= lby::last_breaker_time ) {
		lby::in_update = true;
		//dbg_print( "updated\n" );
		lby::last_breaker_time = csgo::i::globals->m_curtime + 1.1f;
	}

	if ( lby::last_breaker_time - csgo::i::globals->m_ipt < csgo::i::globals->m_curtime )
		lby::balance_update = true;
}

player_t* looking_at( ) {
	player_t* ret = nullptr;

	vec3_t angs;
	csgo::i::engine->get_viewangles( angs );
	csgo::clamp( angs );

	auto best_fov = 180.0f;

	csgo::for_each_player( [ & ] ( player_t* pl ) {
		if ( pl->team( ) == g::local->team( ) )
			return;

		auto angle_to = csgo::calc_angle( g::local->origin( ), pl->origin( ) );
		csgo::clamp( angle_to );
		auto fov = csgo::calc_fov( angle_to, angs );

		if ( fov < best_fov ) {
			ret = pl;
			best_fov = fov;
		}
		} );

	return ret;
}

int find_freestand_side( player_t* pl, float range ) {
	const auto cross = csgo::angle_vec( csgo::calc_angle( g::local->origin( ) + vec3_t( 0.0f, 0.0f, 64.0f ), pl->origin( ) + vec3_t( 0.0f, 0.0f, 64.0f ) ) ).cross_product( vec3_t( 0.0f, 0.0f, 1.0f ) );

	const auto src = g::local->origin( ) + vec3_t( 0.0f, 0.0f, 64.0f );
	const auto dst = pl->origin( ) + pl->vel( ) * ( csgo::i::globals->m_curtime - pl->simtime( ) ) + vec3_t( 0.0f, 0.0f, 64.0f );

	const auto l_dmg = autowall::dmg( g::local, pl, src + cross * range, dst + cross * range, 0 );
	const auto r_dmg = autowall::dmg( g::local, pl, src - cross * range, dst - cross * range, 0 );

	if ( l_dmg == r_dmg )
		return -1;

	return l_dmg > r_dmg ? 0 : 1;
}

int ducked_ticks = 0;

void features::antiaim::run( ucmd_t* ucmd, float& old_smove, float& old_fmove ) {
	/* toggle */
	auto air = options::vars [ _( "antiaim.air.enabled" ) ].val.b;
	auto move = options::vars [ _( "antiaim.moving.enabled" ) ].val.b;
	auto stand = options::vars [ _( "antiaim.standing.enabled" ) ].val.b;
	auto slow_walk = options::vars [ _( "antiaim.slow_walk.enabled" ) ].val.b;

	/* desync */
	static auto& desync_air = options::vars [ _( "antiaim.air.desync" ) ].val.b;
	static auto& desync_move = options::vars [ _( "antiaim.moving.desync" ) ].val.b;
	static auto& desync_stand = options::vars [ _( "antiaim.standing.desync" ) ].val.b;
	static auto& desync_slow_walk = options::vars [ _( "antiaim.slow_walk.desync" ) ].val.b;

	/* desync side */
	static auto& desync_side_air = options::vars [ _( "antiaim.air.invert_initial_side" ) ].val.b;
	static auto& desync_side_move = options::vars [ _( "antiaim.moving.invert_initial_side" ) ].val.b;
	static auto& desync_side_stand = options::vars [ _( "antiaim.standing.invert_initial_side" ) ].val.b;
	static auto& desync_side_slow_walk = options::vars [ _( "antiaim.slow_walk.invert_initial_side" ) ].val.b;

	/* fake lag */
	static auto& lag_air = options::vars [ _( "antiaim.air.fakelag_factor" ) ].val.i;
	static auto& lag_move = options::vars [ _( "antiaim.moving.fakelag_factor" ) ].val.i;
	static auto& lag_stand = options::vars [ _( "antiaim.standing.fakelag_factor" ) ].val.i;
	static auto& lag_slow_walk = options::vars [ _( "antiaim.slow_walk.fakelag_factor" ) ].val.i;

	/* pitch */
	static auto& pitch_air = options::vars [ _( "antiaim.air.pitch" ) ].val.i;
	static auto& pitch_move = options::vars [ _( "antiaim.moving.pitch" ) ].val.i;
	static auto& pitch_stand = options::vars [ _( "antiaim.standing.pitch" ) ].val.i;
	static auto& pitch_slow_walk = options::vars [ _( "antiaim.slow_walk.pitch" ) ].val.i;

	/* base yaw */
	static auto& yaw_offset_air = options::vars [ _( "antiaim.air.yaw_offset" ) ].val.f;
	static auto& yaw_offset_move = options::vars [ _( "antiaim.moving.yaw_offset" ) ].val.f;
	static auto& yaw_offset_stand = options::vars [ _( "antiaim.standing.yaw_offset" ) ].val.f;
	static auto& yaw_offset_slow_walk = options::vars [ _( "antiaim.slow_walk.yaw_offset" ) ].val.f;

	/* slow walk settings */
	static auto& slow_walk_speed = options::vars [ _( "antiaim.slow_walk_speed" ) ].val.f;
	static auto& fakewalk = options::vars [ _( "antiaim.fakewalk" ) ].val.b;

	/* jitter desync */
	static auto& jitter_air = options::vars [ _( "antiaim.air.jitter_desync_side" ) ].val.b;
	static auto& jitter_move = options::vars [ _( "antiaim.moving.jitter_desync_side" ) ].val.b;
	static auto& jitter_stand = options::vars [ _( "antiaim.standing.jitter_desync_side" ) ].val.b;
	static auto& jitter_slow_walk = options::vars [ _( "antiaim.slow_walk.jitter_desync_side" ) ].val.b;

	/* desync ranges */
	static auto& desync_amount_air = options::vars [ _( "antiaim.air.desync_range" ) ].val.f;
	static auto& desync_amount_move = options::vars [ _( "antiaim.moving.desync_range" ) ].val.f;
	static auto& desync_amount_stand = options::vars [ _( "antiaim.standing.desync_range" ) ].val.f;
	static auto& desync_amount_slow_walk = options::vars [ _( "antiaim.slow_walk.desync_range" ) ].val.f;

	static auto& desync_amount_inverted_air = options::vars [ _( "antiaim.air.desync_range_inverted" ) ].val.f;
	static auto& desync_amount_inverted_move = options::vars [ _( "antiaim.moving.desync_range_inverted" ) ].val.f;
	static auto& desync_amount_inverted_stand = options::vars [ _( "antiaim.standing.desync_range_inverted" ) ].val.f;
	static auto& desync_amount_inverted_slow_walk = options::vars [ _( "antiaim.slow_walk.desync_range_inverted" ) ].val.f;

	/* jitter amount */
	static auto& jitter_amount_air = options::vars [ _( "antiaim.air.jitter_range" ) ].val.f;
	static auto& jitter_amount_move = options::vars [ _( "antiaim.moving.jitter_range" ) ].val.f;
	static auto& jitter_amount_stand = options::vars [ _( "antiaim.standing.jitter_range" ) ].val.f;
	static auto& jitter_amount_slow_walk = options::vars [ _( "antiaim.slow_walk.jitter_range" ) ].val.f;

	static auto& auto_direction_amount_air = options::vars [ _( "antiaim.air.auto_direction_amount" ) ].val.f;
	static auto& auto_direction_amount_move = options::vars [ _( "antiaim.moving.auto_direction_amount" ) ].val.f;
	static auto& auto_direction_amount_stand = options::vars [ _( "antiaim.standing.auto_direction_amount" ) ].val.f;
	static auto& auto_direction_amount_slow_walk = options::vars [ _( "antiaim.slow_walk.auto_direction_amount" ) ].val.f;

	static auto& auto_direction_range_air = options::vars [ _( "antiaim.air.auto_direction_range" ) ].val.f;
	static auto& auto_direction_range_move = options::vars [ _( "antiaim.moving.auto_direction_range" ) ].val.f;
	static auto& auto_direction_range_stand = options::vars [ _( "antiaim.standing.auto_direction_range" ) ].val.f;
	static auto& auto_direction_range_slow_walk = options::vars [ _( "antiaim.slow_walk.auto_direction_range" ) ].val.f;

	static auto& left_key = options::vars [ _( "antiaim.manual_left_key" ) ].val.i;
	static auto& right_key = options::vars [ _( "antiaim.manual_right_key" ) ].val.i;
	static auto& back_key = options::vars [ _( "antiaim.manual_back_key" ) ].val.i;

	static auto& slowwalk_key = options::vars [ _( "antiaim.slow_walk_key" ) ].val.i;
	static auto& slowwalk_key_mode = options::vars [ _( "antiaim.slow_walk_key_mode" ) ].val.i;

	static auto& fd_mode = options::vars [ _( "antiaim.fakeduck_mode" ) ].val.i;
	static auto& fd_enabled = options::vars [ _( "antiaim.fakeduck" ) ].val.b;
	static auto& fd_key = options::vars [ _( "antiaim.fakeduck_key" ) ].val.i;
	static auto& fd_key_mode = options::vars [ _( "antiaim.fakeduck_key_mode" ) ].val.i;

	static auto& desync_flip_key = options::vars [ _( "antiaim.desync_invert_key" ) ].val.i;
	static auto& desync_flip_key_mode = options::vars [ _( "antiaim.desync_invert_key_mode" ) ].val.i;

	static auto& center_real_air = options::vars [ _( "antiaim.air.center_real" ) ].val.b;
	static auto& center_real_move = options::vars [ _( "antiaim.moving.center_real" ) ].val.b;
	static auto& center_real_stand = options::vars [ _( "antiaim.standing.center_real" ) ].val.b;
	static auto& center_real_slow_walk = options::vars [ _( "antiaim.slow_walk.center_real" ) ].val.b;

	static auto& anti_bruteforce_air = options::vars [ _( "antiaim.air.anti_bruteforce" ) ].val.b;
	static auto& anti_bruteforce_move = options::vars [ _( "antiaim.moving.anti_bruteforce" ) ].val.b;
	static auto& anti_bruteforce_stand = options::vars [ _( "antiaim.standing.anti_bruteforce" ) ].val.b;
	static auto& anti_bruteforce_slow_walk = options::vars [ _( "antiaim.slow_walk.anti_bruteforce" ) ].val.b;

	static auto& anti_freestand_prediction_air = options::vars [ _( "antiaim.air.anti_freestand_prediction" ) ].val.b;
	static auto& anti_freestand_prediction_move = options::vars [ _( "antiaim.moving.anti_freestand_prediction" ) ].val.b;
	static auto& anti_freestand_prediction_stand = options::vars [ _( "antiaim.standing.anti_freestand_prediction" ) ].val.b;
	static auto& anti_freestand_prediction_slow_walk = options::vars [ _( "antiaim.slow_walk.anti_freestand_prediction" ) ].val.b;

	static auto& base_yaw_air = options::vars [ _( "antiaim.air.base_yaw" ) ].val.i;
	static auto& base_yaw_move = options::vars [ _( "antiaim.moving.base_yaw" ) ].val.i;
	static auto& base_yaw_stand = options::vars [ _( "antiaim.standing.base_yaw" ) ].val.i;
	static auto& base_yaw_slow_walk = options::vars [ _( "antiaim.slow_walk.base_yaw" ) ].val.i;

	static auto& rotation_range_air = options::vars [ _( "antiaim.air.rotation_range" ) ].val.f;
	static auto& rotation_range_move = options::vars [ _( "antiaim.moving.rotation_range" ) ].val.f;
	static auto& rotation_range_stand = options::vars [ _( "antiaim.standing.rotation_range" ) ].val.f;
	static auto& rotation_range_slow_walk = options::vars [ _( "antiaim.slow_walk.rotation_range" ) ].val.f;

	static auto& rotation_speed_air = options::vars [ _( "antiaim.air.rotation_speed" ) ].val.f;
	static auto& rotation_speed_move = options::vars [ _( "antiaim.moving.rotation_speed" ) ].val.f;
	static auto& rotation_speed_stand = options::vars [ _( "antiaim.standing.rotation_speed" ) ].val.f;
	static auto& rotation_speed_slow_walk = options::vars [ _( "antiaim.slow_walk.rotation_speed" ) ].val.f;

	static auto& desync_type = options::vars [ _( "antiaim.standing.desync_type" ) ].val.i;
	static auto& desync_type_inverted = options::vars [ _( "antiaim.standing.desync_type_inverted" ) ].val.i;

	security_handler::update( );

	const auto choke_limit = g::cvars::sv_maxusrcmdprocessticks->get_int();

	if ( !left_key && side == 1 )
		side = -1;

	if ( !right_key && side == 2 )
		side = -1;

	if ( !back_key && side == 0 )
		side = -1;

	if ( !desync_flip_key && desync_side != -1 )
		desync_side = -1;

	if ( utils::keybind_active( left_key, 0 ) )
		side = 1;

	if ( utils::keybind_active( right_key, 0 ) )
		side = 2;

	if ( utils::keybind_active( back_key, 0 ) )
		side = 0;

	if ( desync_flip_key && utils::keybind_active( desync_flip_key, desync_flip_key_mode ) )
		desync_side = 0;
	else if ( desync_flip_key )
		desync_side = 1;

	/* reset anti-bruteforce when new round starts */
	if ( g::round == round_t::starting )
		for ( auto& shot_count : lagcomp::data::shot_count )
			shot_count = 0;

	/* manage fakelag */
	auto force_standing_antiaim = false;

	if ( g::local->valid( ) && g::round != round_t::starting ) {
		auto max_lag = 1;

		if ( air && !( g::local->flags( ) & 1 ) )
			max_lag = lag_air;
		else if ( slow_walk && g::local->vel( ).length_2d( ) > 5.0f && utils::keybind_active( slowwalk_key, slowwalk_key_mode ) )
			max_lag = lag_slow_walk;
		else if ( move && g::local->vel( ).length_2d( ) > 0.1f )
			max_lag = lag_move;
		else if ( stand )
			max_lag = lag_stand;

		max_lag = std::clamp( max_lag, 1, choke_limit );

		if ( aa::old_lag_air != static_cast< int >( lag_air ) ) {
			g_refresh_counter = 0;
			g::shifted_amount = lag_air;
			aa::old_lag_air = static_cast< int >( lag_air );
		}

		if ( aa::old_lag_slow_walk != static_cast< int >( lag_slow_walk ) ) {
			g_refresh_counter = 0;
			g::shifted_amount = lag_slow_walk;
			aa::old_lag_slow_walk = static_cast< int >( lag_slow_walk );
		}

		if ( aa::old_lag_move != static_cast< int >( lag_move ) ) {
			g_refresh_counter = 0;
			g::shifted_amount = lag_move;
			aa::old_lag_move = static_cast< int >( lag_move );
		}

		if ( aa::old_lag_stand != static_cast< int >( lag_stand ) ) {
			g_refresh_counter = 0;
			g::shifted_amount = lag_stand;
			aa::old_lag_stand = static_cast< int >( lag_stand );
		}

		/* make up for lost frames */
		if ( csgo::i::globals->m_frametime > csgo::i::globals->m_ipt ) {
			g_refresh_counter = 0;
			g::shifted_amount = std::clamp( csgo::time2ticks( csgo::i::globals->m_frametime ), 0, choke_limit );
		}

		static auto last_final_shift_amount = 0;
		auto final_shift_amount_max = static_cast< int >( features::ragebot::active_config.max_dt_ticks );

		if ( !features::ragebot::active_config.dt_enabled || !utils::keybind_active( features::ragebot::active_config.dt_key, features::ragebot::active_config.dt_key_mode ) )
			final_shift_amount_max = 0;

		/* dont shift tickbase with revolver */
		if ( g::local && g::local->weapon( ) && g::local->weapon( )->data( ) && ( g::local->weapon( )->item_definition_index( ) == 64 || g::local->weapon( )->data( )->m_type == 0 || g::local->weapon( )->data( )->m_type >= 7 ) )
			final_shift_amount_max = 0;

		if ( final_shift_amount_max && !last_final_shift_amount ) {
			g_refresh_counter = 0;
			g::shifted_amount = std::clamp( static_cast< int >( features::ragebot::active_config.max_dt_ticks ), 0, choke_limit );
		}

		last_final_shift_amount = final_shift_amount_max;

		max_lag = std::clamp< int >( max_lag, 1, choke_limit + 1 - final_shift_amount_max );

		/* allow 1 extra tick just for when we land (if we are in air) */
		if ( !( g::local->flags( ) & 1 ) ) {
			max_lag = std::clamp< int >( max_lag, 1, g::cvars::sv_maxusrcmdprocessticks->get_int() - 1 );
		}

		if ( fakewalk && utils::keybind_active( slowwalk_key, slowwalk_key_mode ) ) {
			max_lag = 14;
		}

		g::send_packet = csgo::i::client_state->choked( ) >= max_lag;

		if ( g::local->flags( ) & 1 && !aa::was_on_ground && g::local->weapon( )->item_definition_index( ) != 64 )
			g::send_packet = false;

		aa::was_on_ground = g::local->flags( ) & 1;

		if ( fd_enabled && utils::keybind_active( fd_key, fd_key_mode ) ) {
			aa::was_fd = true;
			g::send_packet = csgo::i::client_state->choked( ) >= choke_limit;

			if ( csgo::is_valve_server( ) ) {
				if ( ducked_ticks <= 9 ) {
					ucmd->m_buttons |= 4;
					g::send_packet = true;
				}
				else
					ucmd->m_buttons = ( csgo::i::client_state->choked( ) > 3 ) ? ( ucmd->m_buttons | 4 ) : ( ucmd->m_buttons & ~4 );

				ducked_ticks++;
			}
			else {
				ucmd->m_buttons = ( csgo::i::client_state->choked( ) > ( fd_mode == 0 ? 9 : 8 ) ) ? ( ucmd->m_buttons | 4 ) : ( ucmd->m_buttons & ~4 );
			}

			ucmd->m_buttons |= 0x400000;
		}
		else if ( aa::was_fd ) {
			ducked_ticks = 0;
			g_refresh_counter = 0;
			g::shifted_amount = choke_limit;
			aa::was_fd = false;
		}

		auto approach_speed = [ & ] ( float target_speed ) {
			const auto vec_move = vec3_t( old_fmove, old_smove, ucmd->m_umove );
			const auto magnitude = vec_move.length_2d( );
			const auto max_speed = g::local->weapon( )->data( )->m_max_speed;
			const auto move_to_button_ratio = 250.0f / g::cvars::cl_forwardspeed->get_float();
			const auto move_ratio = target_speed * move_to_button_ratio;

			if ( !target_speed ) {
				if ( g::local->vel( ).length_2d( ) <= 13.0f ) {
					old_fmove = old_smove = 0.0f;
				}
				else {
					auto as_ang = csgo::vec_angle( vec_move );
					as_ang.y = csgo::normalize( as_ang.y + 180.0f );
					const auto inverted_move = csgo::angle_vec( as_ang );

					old_fmove = inverted_move.x * g::cvars::cl_forwardspeed->get_float ( );
					old_smove = inverted_move.y * g::cvars::cl_forwardspeed->get_float ( );
				}
			}
			else if ( std::fabsf( old_fmove ) > 3.0f || std::fabsf( old_smove ) > 3.0f ) {
				old_fmove = ( old_fmove / magnitude ) * move_ratio;
				old_smove = ( old_smove / magnitude ) * move_ratio;
			}

			ucmd->m_buttons &= ~0x20000;
		};

		if ( utils::keybind_active( slowwalk_key, slowwalk_key_mode ) && g::local->weapon( ) && g::local->weapon( )->data( ) ) {
			if ( fakewalk ) {
				force_standing_antiaim = true;

				if ( csgo::i::client_state->choked( ) > 7 )
					approach_speed( 0.0f );

				/* force lby flick, will update when we stand still and send packet... */
				//if ( !aa::choke_counter ) {
				//	approach_speed ( 0.0f );
				//}
				//else {
				//	if ( aa::choke_counter == 6 )
				//		approach_speed ( 0.0f );
				//	else {
				//		//approach_speed ( g::local->weapon ( )->data ( )->m_max_speed * 0.333f );
				//
				//		if ( aa::choke_counter == 5 )
				//			lby::in_update = true;
				//	}
				//}
			}
			else {
				/* tiny slowwalk */
				if ( std::fabsf( old_fmove ) > 3.3f || std::fabsf( old_smove ) > 3.3f )
					approach_speed( ( g::local->weapon( )->data( )->m_max_speed * 0.34f ) * ( slow_walk_speed / 100.0f ) );
			}
		}
	}
	else {
		g::send_packet = true;
	}

	if ( g::send_packet )
		aa::flip = !aa::flip;

	if ( !g::local->valid( )
		|| g::local->movetype( ) == movetypes::movetype_noclip
		|| g::local->movetype( ) == movetypes::movetype_ladder
		|| ucmd->m_buttons & 32
		|| !g::local->weapon( )
		|| !g::local->weapon( )->data( )
		|| ( g::local->weapon( )->data( )->m_type == 0 && ucmd->m_buttons & 2048 )
		//|| g::local->weapon( )->data( )->m_type == 0
		|| g::round == round_t::starting ) {
		antiaiming = false;
		return;
	}

	if ( ( g::local->weapon( )->data( )->m_type == 9 ) ? ( !g::local->weapon( )->pin_pulled( ) && g::local->weapon( )->throw_time( ) > 0.0f ) : ( ucmd->m_buttons & 1 ) ) {
		antiaiming = false;
		return;
	}

	const auto target_player = looking_at( );
	//update_anti_bruteforce ( );
	auto desync_amnt = ( desync_side || desync_side == -1 ) ? 120.0f : -120.0f;

	auto process_base_yaw = [ & ] ( int base_yaw, float auto_dir_amount, float auto_dir_range ) {
		switch ( base_yaw ) {
			case 0: {
				vec3_t ang;
				csgo::i::engine->get_viewangles( ang );
				ucmd->m_angs.y = ang.y;
			} break;
			case 1: {
				ucmd->m_angs.y = 0.0f;
			} break;
			case 2: {
				if ( target_player ) {
					const auto yaw_to = csgo::normalize( csgo::calc_angle( g::local->origin( ), target_player->origin( ) ).y );
					ucmd->m_angs.y = yaw_to;
				}
				else {
					vec3_t ang;
					csgo::i::engine->get_viewangles( ang );
					ucmd->m_angs.y = ang.y;
				}
			} break;
			case 3: {
				if ( target_player ) {
					const auto desync_side = find_freestand_side( target_player, auto_dir_range );

					if ( desync_side != -1 ) {
						ucmd->m_angs.y += desync_side ? -auto_dir_amount : auto_dir_amount;
					}
					else {
						const auto yaw_to = csgo::normalize( csgo::calc_angle( g::local->origin( ), target_player->origin( ) ).y + 180.0f );
						ucmd->m_angs.y = yaw_to;
					}
				}
				else {
					vec3_t ang;
					csgo::i::engine->get_viewangles( ang );
					ucmd->m_angs.y = ang.y + 180.0f;
				}
			} break;
		}
	};

	/* manage antiaim */ {
		if ( !( g::local->flags( ) & 1 ) ) {
			if ( air ) {
				process_base_yaw( base_yaw_air, auto_direction_amount_air, auto_direction_range_air );

				ucmd->m_angs.y += yaw_offset_air;

				if ( side != -1 ) {
					vec3_t ang;
					csgo::i::engine->get_viewangles( ang );

					switch ( side ) {
						case 0: ucmd->m_angs.y = ang.y + 180.0f; break;
						case 1: ucmd->m_angs.y = ang.y + 90.0f; break;
						case 2: ucmd->m_angs.y = ang.y - 90.0f; break;
					}
				}

				switch ( pitch_air ) {
					case 0: break;
					case 1: ucmd->m_angs.x = 89.0f; break;
					case 2: ucmd->m_angs.x = -89.0f; break;
					case 3: ucmd->m_angs.x = 0.0f; break;
				}

				if ( target_player && anti_freestand_prediction_air ) {
					const auto desync_side = find_freestand_side( target_player, auto_direction_range_air );

					if ( desync_side != -1 )
						desync_amnt = !desync_side ? 120.0f : -120.0f;
				}

				if ( anti_bruteforce_air && target_player )
					desync_amnt = ( lagcomp::data::shot_count [ target_player->idx( ) ] % 2 ) ? -desync_amnt : desync_amnt;

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

					ucmd->m_angs.y += ( ( ( desync_side_air ? desync_amnt : -desync_amnt ) > 0.0f ) ? g::local->desync_amount( ) : -g::local->desync_amount( ) ) * std::fabsf( desync_amnt * 0.5f );
				}

				ucmd->m_angs.y += aa::flip ? -jitter_amount_air : jitter_amount_air;

				if ( rotation_range_air )
					ucmd->m_angs.y += std::fmodf( csgo::i::globals->m_curtime * rotation_range_air * rotation_speed_air, rotation_range_air ) - rotation_range_air * 0.5f;

				antiaiming = true;
			}
			else {
				antiaiming = false;
			}
		}
		else if ( g::local->vel( ).length_2d( ) > 5.0f && g::local->weapon( ) && g::local->weapon( )->data( ) && !force_standing_antiaim ) {
			if ( utils::keybind_active( slowwalk_key, slowwalk_key_mode ) && slow_walk ) {
				process_base_yaw( base_yaw_slow_walk, auto_direction_amount_slow_walk, auto_direction_range_slow_walk );

				ucmd->m_angs.y += yaw_offset_slow_walk;

				if ( side != -1 ) {
					vec3_t ang;
					csgo::i::engine->get_viewangles( ang );

					switch ( side ) {
						case 0: ucmd->m_angs.y = ang.y + 180.0f; break;
						case 1: ucmd->m_angs.y = ang.y + 90.0f; break;
						case 2: ucmd->m_angs.y = ang.y - 90.0f; break;
					}
				}

				switch ( pitch_slow_walk ) {
					case 0: break;
					case 1: ucmd->m_angs.x = 89.0f; break;
					case 2: ucmd->m_angs.x = -89.0f; break;
					case 3: ucmd->m_angs.x = 0.0f; break;
				}

				if ( target_player && anti_freestand_prediction_slow_walk ) {
					const auto desync_side = find_freestand_side( target_player, auto_direction_range_slow_walk );

					if ( desync_side != -1 )
						desync_amnt = !desync_side ? 120.0f : -120.0f;
				}

				if ( anti_bruteforce_slow_walk && target_player )
					desync_amnt = ( lagcomp::data::shot_count [ target_player->idx( ) ] % 2 ) ? -desync_amnt : desync_amnt;

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

					ucmd->m_angs.y += ( ( ( desync_side_slow_walk ? desync_amnt : -desync_amnt ) > 0.0f ) ? g::local->desync_amount( ) : -g::local->desync_amount( ) ) * std::fabsf( desync_amnt * 0.5f );
				}

				ucmd->m_angs.y += aa::flip ? -jitter_amount_slow_walk : jitter_amount_slow_walk;

				if ( rotation_range_slow_walk )
					ucmd->m_angs.y += std::fmodf( csgo::i::globals->m_curtime * rotation_range_slow_walk * rotation_speed_slow_walk, rotation_range_slow_walk ) - rotation_range_slow_walk * 0.5f;

				antiaiming = true;
			}
			else if ( move ) {
				process_base_yaw( base_yaw_move, auto_direction_amount_move, auto_direction_range_move );

				ucmd->m_angs.y += yaw_offset_move;

				if ( side != -1 ) {
					vec3_t ang;
					csgo::i::engine->get_viewangles( ang );

					switch ( side ) {
						case 0: ucmd->m_angs.y = ang.y + 180.0f; break;
						case 1: ucmd->m_angs.y = ang.y + 90.0f; break;
						case 2: ucmd->m_angs.y = ang.y - 90.0f; break;
					}
				}

				switch ( pitch_move ) {
					case 0: break;
					case 1: ucmd->m_angs.x = 89.0f; break;
					case 2: ucmd->m_angs.x = -89.0f; break;
					case 3: ucmd->m_angs.x = 0.0f; break;
				}

				if ( target_player && anti_freestand_prediction_move ) {
					const auto desync_side = find_freestand_side( target_player, auto_direction_range_move );

					if ( desync_side != -1 )
						desync_amnt = !desync_side ? 120.0f : -120.0f;
				}

				if ( anti_bruteforce_move && target_player )
					desync_amnt = ( lagcomp::data::shot_count [ target_player->idx( ) ] % 2 ) ? -desync_amnt : desync_amnt;

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

					ucmd->m_angs.y += ( ( ( desync_side_move ? desync_amnt : -desync_amnt ) > 0.0f ) ? g::local->desync_amount( ) : -g::local->desync_amount( ) ) * std::fabsf( desync_amnt * 0.5f );
				}

				ucmd->m_angs.y += aa::flip ? -jitter_amount_move : jitter_amount_move;

				if ( rotation_range_move )
					ucmd->m_angs.y += std::fmodf( csgo::i::globals->m_curtime * rotation_range_move * rotation_speed_move, rotation_range_move ) - rotation_range_move * 0.5f;

				antiaiming = true;
			}
			else {
				antiaiming = false;
			}
		}
		else if ( stand ) {
			process_base_yaw( base_yaw_stand, auto_direction_amount_stand, auto_direction_range_stand );

			ucmd->m_angs.y += yaw_offset_stand;

			if ( side != -1 ) {
				vec3_t ang;
				csgo::i::engine->get_viewangles( ang );

				switch ( side ) {
					case 0: ucmd->m_angs.y = ang.y + 180.0f; break;
					case 1: ucmd->m_angs.y = ang.y + 90.0f; break;
					case 2: ucmd->m_angs.y = ang.y - 90.0f; break;
				}
			}

			switch ( pitch_stand ) {
				case 0: break;
				case 1: ucmd->m_angs.x = 89.0f; break;
				case 2: ucmd->m_angs.x = -89.0f; break;
				case 3: ucmd->m_angs.x = 0.0f; break;
			}

			if ( target_player && anti_freestand_prediction_stand ) {
				const auto desync_side = find_freestand_side( target_player, auto_direction_range_stand );

				if ( desync_side != -1 )
					desync_amnt = !desync_side ? 120.0f : -120.0f;
			}

			if ( anti_bruteforce_stand && target_player )
				desync_amnt = ( lagcomp::data::shot_count [ target_player->idx( ) ] % 2 ) ? -desync_amnt : desync_amnt;

			if ( desync_stand ) {
				auto selected_desync_type = 0;

				if ( ( desync_side_stand ? -desync_amnt : desync_amnt ) > 0.0f )
					selected_desync_type = desync_type;
				else
					selected_desync_type = desync_type_inverted;

				switch ( selected_desync_type ) {
					case 0: /* real around fake */ {
						static float last_update_time = csgo::i::globals->m_curtime;

						/* micro movements */
						old_fmove += aa::move_flip ? -( g::local->crouch_amount ( ) > 0.0f ? 3.0f : 1.1f ) : ( g::local->crouch_amount ( ) > 0.0f ? 3.0f : 1.1f );

						//if ( fabsf( last_update_time - csgo::i::globals->m_curtime ) > 0.22f ) {
						//	old_fmove = copysignf ( 15.0f, old_fmove );
						//	last_update_time = csgo::i::globals->m_curtime;
						//}

						aa::move_flip = !aa::move_flip;

						desync_amnt *= 0.5f;

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
								ucmd->m_angs.y -= copysignf( 120.0f, desync_side_stand ? -desync_amnt : desync_amnt );
								//ucmd->m_angs.y += 180.0f;
								g::send_packet = false;
							}
							else if ( !g::send_packet ) {
								ucmd->m_angs.y += copysignf( fabsf( desync_amnt ) * 90.0f, desync_side_stand ? -desync_amnt : desync_amnt );
							}
						}
					}break;
				}

				if ( center_real_stand ) {
					if ( ( desync_side_stand ? desync_amnt : -desync_amnt ) > 0.0f )
						desync_amnt *= desync_amount_stand / 30.0f;
					else
						desync_amnt *= desync_amount_inverted_stand / 30.0f;

					ucmd->m_angs.y += ( ( ( desync_side_stand ? desync_amnt : -desync_amnt ) > 0.0f ) ? g::local->desync_amount( ) : -g::local->desync_amount( ) ) * std::fabsf( desync_amnt * 0.5f );
				}
			}

			ucmd->m_angs.y += aa::flip ? -jitter_amount_stand : jitter_amount_stand;

			if ( rotation_range_stand )
				ucmd->m_angs.y += std::fmodf( csgo::i::globals->m_curtime * rotation_range_stand * rotation_speed_stand, rotation_range_stand ) - rotation_range_stand * 0.5f;

			antiaiming = true;
		}
		else {
			antiaiming = false;
		}
	}
}