#include "antiaim.hpp"
#include "../menu/menu.hpp"
#include "../globals.hpp"
#include "prediction.hpp"
#include "ragebot.hpp"
#include "autowall.hpp"

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

	int choke_counter = 0;
	int old_lag_air = 0;
	int old_lag_move = 0;
	int old_lag_slow_walk = 0;
	int old_lag_stand = 0;
}

void features::antiaim::simulate_lby( ) {
	OPTION ( double, tickbase_shift_amount, "Sesame->A->Rage Aimbot->Main->Maximum Doubletap Ticks", oxui::object_slider );

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
		if ( static_cast< int >( tickbase_shift_amount ) )
			g::shifted_tickbase = g::ucmd->m_cmdnum;

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

player_t* looking_at ( ) {
	player_t* ret = nullptr;

	vec3_t angs;
	csgo::i::engine->get_viewangles ( angs );
	csgo::clamp ( angs );

	auto best_fov = 180.0f;

	csgo::for_each_player ( [ & ] ( player_t* pl ) {
		if ( pl->team ( ) == g::local->team ( ) )
			return;

		auto angle_to = csgo::calc_angle ( g::local->origin ( ), pl->origin ( ) );
		csgo::clamp ( angle_to );
		auto fov = csgo::normalize( angle_to.dist_to ( angs ) );

		if ( fov < best_fov ) {
			ret = pl;
			best_fov = fov;
		}
	} );

	return ret;
}

int find_freestand_side ( player_t* pl ) {
	const auto cross = csgo::angle_vec ( csgo::calc_angle ( g::local->origin ( ) + vec3_t ( 0.0f, 0.0f, 64.0f ), pl->origin ( ) + vec3_t ( 0.0f, 0.0f, 64.0f ) ) ).cross_product ( vec3_t ( 0.0f, 0.0f, 1.0f ) );

	const auto src = g::local->origin ( ) + vec3_t ( 0.0f, 0.0f, 64.0f );
	const auto dst = pl->origin ( ) + pl->vel ( ) * ( csgo::i::globals->m_curtime - pl->simtime ( ) ) + vec3_t ( 0.0f, 0.0f, 64.0f );

	const auto l_dmg = autowall::dmg ( g::local, pl, src + cross * 45.0f, dst + cross * 45.0f, 0 );
	const auto r_dmg = autowall::dmg ( g::local, pl, src - cross * 45.0f, dst - cross * 45.0f, 0 );

	if ( l_dmg == r_dmg )
		return -1;

	return l_dmg > r_dmg ? 0 : 1;
}

void features::antiaim::run( ucmd_t* ucmd, float& old_smove, float& old_fmove ) {
	OPTION ( double, tickbase_shift_amount, "Sesame->A->Rage Aimbot->Main->Maximum Doubletap Ticks", oxui::object_slider );

	/* toggle */
	OPTION( bool, air, "Sesame->B->Air->Base->In Air", oxui::object_checkbox );
	OPTION( bool, move, "Sesame->B->Moving->Base->On Moving", oxui::object_checkbox );
	OPTION ( bool, stand, "Sesame->B->Standing->Base->On Standing", oxui::object_checkbox );
	OPTION ( bool, slow_walk, "Sesame->B->Slow Walk->Base->On Slow Walk", oxui::object_checkbox );

	/* desync */
	OPTION( bool, desync_air, "Sesame->B->Air->Desync->Desync", oxui::object_checkbox );
	OPTION( bool, desync_move, "Sesame->B->Moving->Desync->Desync", oxui::object_checkbox );
	OPTION ( bool, desync_stand, "Sesame->B->Standing->Desync->Desync", oxui::object_checkbox );
	OPTION( bool, desync_slow_walk, "Sesame->B->Slow Walk->Desync->Desync", oxui::object_checkbox );

	/* desync side */
	OPTION( bool, desync_side_air, "Sesame->B->Air->Desync->Flip Desync Side", oxui::object_checkbox );
	OPTION( bool, desync_side_move, "Sesame->B->Moving->Desync->Flip Desync Side", oxui::object_checkbox );
	OPTION ( bool, desync_side_stand, "Sesame->B->Standing->Desync->Flip Desync Side", oxui::object_checkbox );
	OPTION( bool, desync_side_slow_walk, "Sesame->B->Slow Walk->Desync->Flip Desync Side", oxui::object_checkbox );

	/* fake lag */
	OPTION ( double, lag_air, "Sesame->B->Other->Fakelag->In Air Fakelag", oxui::object_slider );
	OPTION ( double, lag_move, "Sesame->B->Other->Fakelag->Moving Fakelag", oxui::object_slider );
	OPTION ( double, lag_stand, "Sesame->B->Other->Fakelag->Standing Fakelag", oxui::object_slider );
	OPTION ( double, lag_slow_walk, "Sesame->B->Other->Fakelag->Slow Walk Fakelag", oxui::object_slider );
	OPTION ( double, lag_send_air, "Sesame->B->Other->Fakelag->In Air Send Ticks", oxui::object_slider );
	OPTION ( double, lag_send_move, "Sesame->B->Other->Fakelag->Moving Send Ticks", oxui::object_slider );
	OPTION ( double, lag_send_stand, "Sesame->B->Other->Fakelag->Standing Send Ticks", oxui::object_slider );
	OPTION ( double, lag_send_slow_walk, "Sesame->B->Other->Fakelag->Slow Walk Send Ticks", oxui::object_slider );

	/* pitch */
	OPTION( double, pitch_air, "Sesame->B->Air->Base->Base Pitch", oxui::object_slider );
	OPTION( double, pitch_move, "Sesame->B->Moving->Base->Base Pitch", oxui::object_slider );
	OPTION ( double, pitch_stand, "Sesame->B->Standing->Base->Base Pitch", oxui::object_slider );
	OPTION( double, pitch_slow_walk, "Sesame->B->Slow Walk->Base->Base Pitch", oxui::object_slider );

	/* base yaw */
	OPTION( double, yaw_offset_air, "Sesame->B->Air->Base->Yaw Offset", oxui::object_slider );
	OPTION( double, yaw_offset_move, "Sesame->B->Moving->Base->Yaw Offset", oxui::object_slider );
	OPTION ( double, yaw_offset_stand, "Sesame->B->Standing->Base->Yaw Offset", oxui::object_slider );
	OPTION ( double, yaw_offset_slow_walk , "Sesame->B->Slow Walk->Base->Yaw Offset", oxui::object_slider );

	/* slow walk settings */
	OPTION ( double, slow_walk_speed, "Sesame->B->Slow Walk->Slow Walk->Slow Walk Speed", oxui::object_slider );

	/* jitter desync */
	OPTION( bool, jitter_air, "Sesame->B->Air->Desync->Jitter Desync Side", oxui::object_checkbox );
	OPTION( bool, jitter_move, "Sesame->B->Moving->Desync->Jitter Desync Side", oxui::object_checkbox );
	OPTION ( bool, jitter_stand, "Sesame->B->Standing->Desync->Jitter Desync Side", oxui::object_checkbox );
	OPTION( bool, jitter_slow_walk, "Sesame->B->Slow Walk->Desync->Jitter Desync Side", oxui::object_checkbox );

	/* desync ranges */
	OPTION ( double, desync_amount_air, "Sesame->B->Air->Desync->Desync Range", oxui::object_slider );
	OPTION ( double, desync_amount_move, "Sesame->B->Moving->Desync->Desync Range", oxui::object_slider );
	OPTION ( double, desync_amount_stand, "Sesame->B->Standing->Desync->Desync Range", oxui::object_slider );
	OPTION ( double, desync_amount_slow_walk, "Sesame->B->Slow Walk->Desync->Desync Range", oxui::object_slider );

	/* jitter amount */
	OPTION ( double, jitter_amount_air, "Sesame->B->Air->Base->Jitter Range", oxui::object_slider );
	OPTION ( double, jitter_amount_move, "Sesame->B->Moving->Base->Jitter Range", oxui::object_slider );
	OPTION ( double, jitter_amount_stand, "Sesame->B->Standing->Base->Jitter Range", oxui::object_slider );
	OPTION ( double, jitter_amount_slow_walk, "Sesame->B->Slow Walk->Base->Jitter Range", oxui::object_slider );

	OPTION ( double, auto_direction_amount_air, "Sesame->B->Air->Base->Auto Direction Amount", oxui::object_slider );
	OPTION ( double, auto_direction_amount_move, "Sesame->B->Moving->Base->Auto Direction Amount", oxui::object_slider );
	OPTION ( double, auto_direction_amount_stand, "Sesame->B->Standing->Base->Auto Direction Amount", oxui::object_slider );
	OPTION ( double, auto_direction_amount_slow_walk, "Sesame->B->Slow Walk->Base->Auto Direction Amount", oxui::object_slider );

	KEYBIND ( left_key, "Sesame->B->Other->Other->Left Side Key" );
	KEYBIND ( back_key, "Sesame->B->Other->Other->Back Side Key" );
	KEYBIND ( right_key, "Sesame->B->Other->Other->Right Side Key" );
	KEYBIND ( slowwalk_key, "Sesame->B->Slow Walk->Slow Walk->Slow Walk Key" );
	OPTION ( int, fd_mode, "Sesame->B->Other->Other->Fakeduck Mode", oxui::object_dropdown );
	KEYBIND ( fd_key, "Sesame->B->Other->Other->Fakeduck Key" );
	KEYBIND ( desync_flip_key, "Sesame->B->Other->Other->Desync Flip Key" );

	OPTION ( bool, center_real_air, "Sesame->B->Air->Desync->Center Real", oxui::object_checkbox );
	OPTION ( bool, center_real_move, "Sesame->B->Moving->Desync->Center Real", oxui::object_checkbox );
	OPTION ( bool, center_real_stand, "Sesame->B->Standing->Desync->Center Real", oxui::object_checkbox );
	OPTION ( bool, center_real_slow_walk, "Sesame->B->Slow Walk->Desync->Center Real", oxui::object_checkbox );

	KEYBIND ( tickbase_key, "Sesame->A->Rage Aimbot->Main->Doubletap Key" );

	OPTION ( bool, anti_bruteforce_air, "Sesame->B->Air->Anti-Hit->Anti-Bruteforce", oxui::object_checkbox );
	OPTION ( bool, anti_bruteforce_move, "Sesame->B->Moving->Anti-Hit->Anti-Bruteforce", oxui::object_checkbox );
	OPTION ( bool, anti_bruteforce_stand, "Sesame->B->Standing->Anti-Hit->Anti-Bruteforce", oxui::object_checkbox );
	OPTION ( bool, anti_bruteforce_slow_walk, "Sesame->B->Slow Walk->Anti-Hit->Anti-Bruteforce", oxui::object_checkbox );

	OPTION ( bool, anti_freestand_prediction_air, "Sesame->B->Air->Anti-Hit->Anti-Freestand Prediction", oxui::object_checkbox );
	OPTION ( bool, anti_freestand_prediction_move, "Sesame->B->Moving->Anti-Hit->Anti-Freestand Prediction", oxui::object_checkbox );
	OPTION ( bool, anti_freestand_prediction_stand, "Sesame->B->Standing->Anti-Hit->Anti-Freestand Prediction", oxui::object_checkbox );
	OPTION ( bool, anti_freestand_prediction_slow_walk, "Sesame->B->Slow Walk->Anti-Hit->Anti-Freestand Prediction", oxui::object_checkbox );

	OPTION ( int, base_yaw_air, "Sesame->B->Air->Base->Base Yaw", oxui::object_dropdown );
	OPTION ( int, base_yaw_move, "Sesame->B->Moving->Base->Base Yaw", oxui::object_dropdown );
	OPTION ( int, base_yaw_stand, "Sesame->B->Standing->Base->Base Yaw", oxui::object_dropdown );
	OPTION ( int, base_yaw_slow_walk, "Sesame->B->Slow Walk->Base->Base Yaw", oxui::object_dropdown );

	security_handler::update ( );

	if ( MAKE_KEYBIND ( left_key )->key == -1 && side == 1 )
		side = 0;

	if ( MAKE_KEYBIND ( right_key )->key == -1 && side == 2 )
		side = 2;

	if ( MAKE_KEYBIND ( back_key )->key == -1 && side == 0 )
		side = -1;

	if ( desync_flip_key == -1 && desync_side != -1 )
		desync_side = -1;

	if ( MAKE_KEYBIND ( left_key )->key != -1 && utils::key_state ( MAKE_KEYBIND( left_key )->key ) & 1 )
		side = 1;

	if ( MAKE_KEYBIND ( right_key )->key != -1 && utils::key_state ( MAKE_KEYBIND ( right_key )->key ) & 1 )
		side = 2;

	if ( MAKE_KEYBIND ( back_key )->key != -1 && utils::key_state ( MAKE_KEYBIND ( back_key )->key ) & 1 )
		side = 0;

	if ( desync_flip_key )
		desync_side = !desync_side;

	/* reset anti-bruteforce when new round starts */
	if ( g::round == round_t::starting )
		for ( auto& shot_count : lagcomp::data::shot_count )
			shot_count = 0;

	/* manage fakelag */
	if ( g::local->valid ( ) ) {
		auto max_lag = 1;

		if ( air && !( g::local->flags ( ) & 1 ) )
			max_lag = lag_air;
		else if ( slow_walk && g::local->vel ( ).length_2d ( ) > 5.0f && slowwalk_key )
			max_lag = lag_slow_walk;
		else if ( move && g::local->vel ( ).length_2d ( ) > 0.1f )
			max_lag = lag_move;
		else if ( stand )
			max_lag = lag_stand;

		auto max_send = 0;

		if ( air && !( g::local->flags ( ) & 1 ) )
			max_send = lag_send_air;
		else if ( slow_walk && g::local->vel ( ).length_2d ( ) > 5.0f && slowwalk_key )
			max_send = lag_send_slow_walk;
		else if ( move && g::local->vel ( ).length_2d ( ) > 0.1f )
			max_send = lag_send_move;
		else if ( stand )
			max_send = lag_send_stand;

		if ( aa::old_lag_air != static_cast< int >( lag_air ) ) {
			g::shifted_tickbase = ucmd->m_cmdnum + static_cast< int >( lag_air );
			aa::old_lag_air = static_cast< int >( lag_air );
		}

		if ( aa::old_lag_slow_walk != static_cast< int >( lag_slow_walk ) ) {
			g::shifted_tickbase = ucmd->m_cmdnum + static_cast< int >( lag_slow_walk );
			aa::old_lag_slow_walk = static_cast< int >( lag_slow_walk );
		}

		if ( aa::old_lag_move != static_cast< int >( lag_move ) ) {
			g::shifted_tickbase = ucmd->m_cmdnum + static_cast< int >( lag_move );
			aa::old_lag_move = static_cast< int >( lag_move );
		}

		if ( aa::old_lag_stand != static_cast< int >( lag_stand ) ) {
			g::shifted_tickbase = ucmd->m_cmdnum + static_cast< int >( lag_stand );
			aa::old_lag_stand = static_cast< int >( lag_stand );
		}

		/* make up for lost frames */
		if ( csgo::i::globals->m_frametime > csgo::i::globals->m_ipt )
			g::shifted_tickbase = ucmd->m_cmdnum + csgo::time2ticks( csgo::i::globals->m_frametime );

		static auto last_final_shift_amount = 0;
		auto final_shift_amount_max = static_cast< int >( tickbase_shift_amount );

		if ( !tickbase_key )
			final_shift_amount_max = 0;

		/* dont shift tickbase with revolver */
		if ( g::local && g::local->weapon ( ) && g::local->weapon ( )->data ( ) && ( g::local->weapon ( )->item_definition_index ( ) == 64 || g::local->weapon ( )->data ( )->m_type == 0 || g::local->weapon ( )->data ( )->m_type >= 7 ) )
			final_shift_amount_max = 0;

		if ( final_shift_amount_max && !last_final_shift_amount )
			g::shifted_tickbase = ucmd->m_cmdnum + static_cast< int >( tickbase_shift_amount );

		last_final_shift_amount = final_shift_amount_max;

		max_lag = std::clamp< int > ( max_lag, 1, 17 - final_shift_amount_max );
		max_send = std::clamp< int > ( max_send, 0, max_lag - 1 );

		if ( aa::choke_counter < max_lag ) {
			g::send_packet = !( aa::choke_counter >= max_send );
			aa::choke_counter++;
		}
		else {
			g::send_packet = true;
			aa::choke_counter = 0;
		}

		//if ( g::local->flags ( ) & 1 && !aa::was_on_ground && g::local->weapon ( )->item_definition_index ( ) != 64 )
		//	g::send_packet = false;

		aa::was_on_ground = g::local->flags ( ) & 1;

		if ( fd_key && fd_mode ) {
			aa::was_fd = true;
			g::send_packet = csgo::i::client_state->choked ( ) >= 16;
			ucmd->m_buttons = ( csgo::i::client_state->choked ( ) > ( fd_mode == 1 ? 9 : 8 ) ? ( ucmd->m_buttons | 4 ) : ( ucmd->m_buttons & ~4 ) ) | 0x400000;
		}
		else if ( aa::was_fd ) {
			g::shifted_tickbase = ucmd->m_cmdnum + 16;
			aa::was_fd = false;
		}

		if ( std::fabsf ( old_fmove ) <= 3.0f && std::fabsf ( old_smove ) <= 3.0f && desync_amount_stand <= 50.0 ) {
			old_fmove += aa::move_flip ? -3.3f : 3.3f;
			aa::move_flip = !aa::move_flip;
		}

		/* tiny slowwalk */
		if ( slow_walk_speed > 0.0f && slowwalk_key && g::local->valid ( ) && g::local->flags ( ) & 1 && g::local->weapon ( ) && g::local->weapon ( )->data ( ) ) {
			const auto vec_move = vec3_t ( old_fmove, old_smove, ucmd->m_umove );
			const auto magnitude = vec_move.length_2d ( );
			const auto max_speed = g::local->weapon ( )->data ( )->m_max_speed;
			const auto move_to_button_ratio = 250.0f / 450.0f;
			const auto speed_ratio = ( max_speed * 0.34f ) * ( slow_walk_speed / 100.0f );
			const auto move_ratio = speed_ratio * move_to_button_ratio;

			if ( std::fabsf ( old_fmove ) > 3.0f || std::fabsf ( old_smove ) > 3.0f ) {
				old_fmove = ( old_fmove / magnitude ) * move_ratio;
				old_smove = ( old_smove / magnitude ) * move_ratio;
			}
		}
	}
	else {
		g::send_packet = true;
	}

	if ( g::send_packet )
		aa::flip = !aa::flip;

	if ( !g::local->valid ( )
		|| g::local->movetype ( ) == movetypes::movetype_noclip
		|| g::local->movetype ( ) == movetypes::movetype_ladder
		|| ucmd->m_buttons & 32
		|| !g::local->weapon ( )
		|| !g::local->weapon ( )->data ( )
		//|| g::local->weapon( )->data( )->m_type == 0
		|| g::round == round_t::starting )
		return;

	if ( ( g::local->weapon ( )->data ( )->m_type == 9 ) ? ( !g::local->weapon ( )->pin_pulled ( ) && g::local->weapon ( )->throw_time ( ) > 0.0f ) : ( ucmd->m_buttons & 1 ) )
		return;

	const auto target_player = looking_at ( );
	//update_anti_bruteforce ( );
	auto desync_amnt = ( desync_side || desync_side == -1 ) ? 120.0f : -120.0f;

	auto process_base_yaw = [ & ] ( int base_yaw, float auto_dir_amount ) {
		switch ( base_yaw ) {
		case 0: {
			vec3_t ang;
			csgo::i::engine->get_viewangles ( ang );
			ucmd->m_angs.y = ang.y;
		} break;
		case 1: {
			ucmd->m_angs.y = 0.0f;
		} break;
		case 2: {
			if ( target_player ) {
				const auto yaw_to = csgo::normalize ( csgo::calc_angle ( g::local->origin ( ), target_player->origin ( ) ).y );
				ucmd->m_angs.y = yaw_to;
			}
			else {
				vec3_t ang;
				csgo::i::engine->get_viewangles ( ang );
				ucmd->m_angs.y = ang.y;
			}
		} break;
		case 3: {
			if ( target_player ) {
				const auto desync_side = find_freestand_side ( target_player );

				if ( desync_side != -1 ) {
					ucmd->m_angs.y += desync_side ? -auto_dir_amount : auto_dir_amount;
				}
				else {
					const auto yaw_to = csgo::normalize ( csgo::calc_angle ( g::local->origin ( ), target_player->origin ( ) ).y + 180.0f );
					ucmd->m_angs.y = yaw_to;
				}
			}
			else {
				vec3_t ang;
				csgo::i::engine->get_viewangles ( ang );
				ucmd->m_angs.y = ang.y + 180.0f;
			}
		} break;
		}
	};

	/* manage antiaim */ {
		if ( !( g::local->flags( ) & 1 ) ) {
			if ( air ) {
				process_base_yaw ( base_yaw_air, auto_direction_amount_air );

				if ( side != -1 ) {
					switch ( side ) {
					case 0: ucmd->m_angs.y += 180.0f; break;
					case 1: ucmd->m_angs.y += 90.0f; break;
					case 2: ucmd->m_angs.y += -90.0f; break;
					}
				}

				ucmd->m_angs.y += yaw_offset_air;
				ucmd->m_angs.x = pitch_air;

				if ( target_player && anti_freestand_prediction_air ) {
					const auto desync_side = find_freestand_side ( target_player );

					if ( desync_side != -1 )
						desync_amnt = !desync_side ? 120.0f : -120.0f;
				}

				desync_amnt *= desync_amount_air / 100.0f;

				if ( anti_bruteforce_air && target_player )
					desync_amnt = ( lagcomp::data::shot_count [ target_player->idx ( ) ] % 2 ) ? -desync_amnt : desync_amnt;

				if ( desync_air && jitter_air && !g::send_packet )
					ucmd->m_angs.y += aa::flip ? -desync_amnt : desync_amnt;
				else if ( desync_air && !g::send_packet )
					ucmd->m_angs.y += desync_side_air ? -desync_amnt : desync_amnt;

				if ( desync_air && center_real_air )
					ucmd->m_angs.y += ( ( ( desync_side_air ? desync_amnt : -desync_amnt ) > 0.0f ) ? g::local->desync_amount ( ) : -g::local->desync_amount ( ) ) * 0.5f;

				ucmd->m_angs.y += aa::flip ? -jitter_amount_air : jitter_amount_air;
			}
		}
		else if ( g::local->vel( ).length_2d( ) > 5.0f && g::local->weapon( ) && g::local->weapon( )->data( ) ) {
			if ( slowwalk_key && slow_walk ) {
				process_base_yaw ( base_yaw_slow_walk, auto_direction_amount_slow_walk );

				if ( side != -1 ) {
					switch ( side ) {
					case 0: ucmd->m_angs.y += 180.0f; break;
					case 1: ucmd->m_angs.y += 90.0f; break;
					case 2: ucmd->m_angs.y += -90.0f; break;
					}
				}

				ucmd->m_angs.y += yaw_offset_slow_walk;
				ucmd->m_angs.x = pitch_slow_walk;

				if ( target_player && anti_freestand_prediction_slow_walk ) {
					const auto desync_side = find_freestand_side ( target_player );

					if ( desync_side != -1 )
						desync_amnt = !desync_side ? 120.0f : -120.0f;
				}

				desync_amnt *= desync_amount_slow_walk / 100.0f;

				if ( anti_bruteforce_slow_walk && target_player )
					desync_amnt = ( lagcomp::data::shot_count [ target_player->idx ( ) ] % 2 ) ? -desync_amnt : desync_amnt;

				if ( desync_slow_walk && jitter_slow_walk && !g::send_packet )
					ucmd->m_angs.y += aa::flip ? -desync_amnt : desync_amnt;
				else if ( desync_slow_walk && !g::send_packet )
					ucmd->m_angs.y += desync_side_slow_walk ? -desync_amnt : desync_amnt;

				if ( desync_slow_walk && center_real_slow_walk )
					ucmd->m_angs.y += ( ( ( desync_side_slow_walk ? desync_amnt : -desync_amnt ) > 0.0f ) ? g::local->desync_amount ( ) : -g::local->desync_amount ( ) ) * 0.5f;

				ucmd->m_angs.y += aa::flip ? -jitter_amount_slow_walk : jitter_amount_slow_walk;
			}
			else if ( move ) {
				process_base_yaw ( base_yaw_move, auto_direction_amount_move );

				if ( side != -1 ) {
					switch ( side ) {
					case 0: ucmd->m_angs.y += 180.0f; break;
					case 1: ucmd->m_angs.y += 90.0f; break;
					case 2: ucmd->m_angs.y += -90.0f; break;
					}
				}

				ucmd->m_angs.y += yaw_offset_move;
				ucmd->m_angs.x = pitch_move;

				if ( target_player && anti_freestand_prediction_move ) {
					const auto desync_side = find_freestand_side ( target_player );

					if ( desync_side != -1 )
						desync_amnt = !desync_side ? 120.0f : -120.0f;
				}

				desync_amnt *= desync_amount_move / 100.0f;

				if ( anti_bruteforce_move && target_player )
					desync_amnt = ( lagcomp::data::shot_count [ target_player->idx ( ) ] % 2 ) ? -desync_amnt : desync_amnt;

				if ( desync_move && jitter_move && !g::send_packet )
					ucmd->m_angs.y += aa::flip ? -desync_amnt : desync_amnt;
				else if ( desync_move && !g::send_packet )
					ucmd->m_angs.y += desync_side_move ? -desync_amnt : desync_amnt;

				if ( desync_move && center_real_move )
					ucmd->m_angs.y += ( ( ( desync_side_move ? desync_amnt : -desync_amnt ) > 0.0f ) ? g::local->desync_amount ( ) : -g::local->desync_amount ( ) ) * 0.5f;

				ucmd->m_angs.y += aa::flip ? -jitter_amount_move : jitter_amount_move;
			}
		}
		else if ( stand ) {
			process_base_yaw ( base_yaw_stand, auto_direction_amount_stand );

			if ( side != -1 ) {
				switch ( side ) {
				case 0: ucmd->m_angs.y += 180.0f; break;
				case 1: ucmd->m_angs.y += 90.0f; break;
				case 2: ucmd->m_angs.y += -90.0f; break;
				}
			}

			ucmd->m_angs.y += yaw_offset_stand;
			ucmd->m_angs.x = pitch_stand;

			if ( target_player && anti_freestand_prediction_stand ) {
				const auto desync_side = find_freestand_side ( target_player );

				if ( desync_side != -1 )
					desync_amnt = !desync_side ? 120.0f : -120.0f;
			}

			desync_amnt *= desync_amount_stand / 100.0f;

			//if ( target_player )
			//	dbg_print ( "shots: %d\n", lagcomp::data::shot_count [ target_player->idx ( ) ] );

			if ( anti_bruteforce_stand && target_player )
				desync_amnt = ( lagcomp::data::shot_count [ target_player->idx ( ) ] % 2 ) ? -desync_amnt : desync_amnt;

			if ( desync_stand ) {
				if ( desync_amount_stand > 50.0 ) {
					if ( jitter_stand ) {
						/* go 180 from update location to trigger 979 activity */
						if ( ( lby::in_update || lby::balance_update ) || !g::send_packet ) {
							ucmd->m_angs.y += aa::flip ? 120.0f : -120.0f;
							g::send_packet = false;
						}
					}
					else {
						if ( lby::in_update /*|| lby::balance_update*/ ) {
							ucmd->m_angs.y -= std::copysignf ( desync_side_stand ? desync_amnt : -desync_amnt, 120.0f );
							g::send_packet = false;
						}
						else if ( !g::send_packet )
							ucmd->m_angs.y += std::copysignf( desync_side_stand ? desync_amnt : -desync_amnt, 30.0f + 30.0f * ( ( desync_amount_stand - 50.0f ) / 50.0f ) );
					}
				}
				else {
					if ( desync_stand && jitter_stand && !g::send_packet )
						ucmd->m_angs.y += aa::flip ? -desync_amnt : desync_amnt;
					else if ( desync_stand && !g::send_packet )
						ucmd->m_angs.y += desync_side_stand ? -desync_amnt : desync_amnt;
				}

				if ( center_real_stand )
					ucmd->m_angs.y += ( ( ( desync_side_stand ? desync_amnt : -desync_amnt ) > 0.0f ) ? g::local->desync_amount ( ) : -g::local->desync_amount ( ) ) * 0.5f;
			}

			ucmd->m_angs.y += aa::flip ? -jitter_amount_stand : jitter_amount_stand;
		}
	}
}