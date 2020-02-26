#include "antiaim.hpp"
#include "../menu/menu.hpp"
#include "../globals.hpp"

namespace lby {
	float next_update = 0.0f;
	bool tick_before_update = false;
	bool in_update = false;
}

namespace aa {
	bool flip = false;
}

void features::antiaim::simulate_lby( ) {
	lby::tick_before_update = false;
	lby::in_update = false;

	if ( !g::local->valid( ) || g::local->vel( ).length_2d( ) > 0.1f ) {
		lby::next_update = csgo::i::globals->m_curtime + 0.22f;
		return;
	}

	const auto ticks_until_update = lby::next_update - csgo::i::globals->m_curtime;

	/* if it will flick the current tick */
	if ( ticks_until_update <= 0.0f ) {
		lby::next_update = csgo::i::globals->m_curtime + 1.1f;
		lby::in_update = true;
	}
	/* if it will flick within the next frame or tick */
	else if ( ticks_until_update <= csgo::i::globals->m_frametime || ticks_until_update <= csgo::ticks2time( 1 ) ) {
		lby::tick_before_update = true;
	}
}

void features::antiaim::run( ucmd_t* ucmd ) {
	simulate_lby( );

	/* toggle */
	FIND( bool, air, "antiaim", "air", "aa on air", oxui::object_checkbox );
	FIND( bool, move, "antiaim", "moving", "aa on move", oxui::object_checkbox );
	FIND( bool, stand, "antiaim", "standing", "aa on stand", oxui::object_checkbox );

	/* desync */
	FIND( bool, desync_air, "antiaim", "air", "desync", oxui::object_checkbox );
	FIND( bool, desync_move, "antiaim", "moving", "desync", oxui::object_checkbox );
	FIND( bool, desync_stand, "antiaim", "standing", "desync", oxui::object_checkbox );

	/* desync side */
	FIND( bool, desync_side_air, "antiaim", "air", "desync side", oxui::object_checkbox );
	FIND( bool, desync_side_move, "antiaim", "moving", "desync side", oxui::object_checkbox );
	FIND( bool, desync_side_stand, "antiaim", "standing", "desync side", oxui::object_checkbox );

	/* fake lag */
	FIND( double, lag_air, "antiaim", "air", "lag", oxui::object_slider );
	FIND( double, lag_move, "antiaim", "moving", "lag", oxui::object_slider );
	FIND( double, lag_stand, "antiaim", "standing", "lag", oxui::object_slider );

	/* pitch */
	FIND( double, pitch_air, "antiaim", "air", "pitch", oxui::object_slider );
	FIND( double, pitch_move, "antiaim", "moving", "pitch", oxui::object_slider );
	FIND( double, pitch_stand, "antiaim", "standing", "pitch", oxui::object_slider );

	/* base yaw */
	FIND( double, base_yaw_air, "antiaim", "air", "base yaw", oxui::object_slider );
	FIND( double, base_yaw_move, "antiaim", "moving", "base yaw", oxui::object_slider );
	FIND( double, base_yaw_stand, "antiaim", "standing", "base yaw", oxui::object_slider );

	/* slow walk settings */
	FIND( double, slow_walk_speed, "antiaim", "moving", "slow walk speed", oxui::object_slider );

	/* jitter desync */
	FIND( bool, jitter_air, "antiaim", "air", "jitter desync", oxui::object_checkbox );
	FIND( bool, jitter_move, "antiaim", "moving", "jitter desync", oxui::object_checkbox );
	FIND( bool, jitter_stand, "antiaim", "standing", "jitter desync", oxui::object_checkbox );

	/* jitter amount */
	FIND( double, jitter_amount_air, "antiaim", "air", "jitter amount", oxui::object_slider );
	FIND( double, jitter_amount_move, "antiaim", "moving", "jitter amount", oxui::object_slider );
	FIND( double, jitter_amount_stand, "antiaim", "standing", "jitter amount", oxui::object_slider );

	/* ghetto slow walk */
	if ( GetAsyncKeyState( VK_XBUTTON2 ) && g::local->valid( ) && g::local->flags( ) & 1 && g::local->weapon( ) && g::local->weapon( )->data( ) ) {
		const auto vec_move = vec3_t( ucmd->m_fmove, ucmd->m_smove, ucmd->m_umove );
		const auto magnitude = vec_move.length_2d( );
		const auto max_speed = g::local->weapon( )->data( )->m_max_speed;
		const auto move_to_button_ratio = 250.0f / 450.0f;
		const auto speed_ratio = ( max_speed * 0.34f ) * ( slow_walk_speed / 100.0f );
		const auto move_ratio = speed_ratio * move_to_button_ratio;

		if ( ucmd->m_fmove != 0.0f || ucmd->m_smove != 0.0f ) {
			ucmd->m_fmove = ( ucmd->m_fmove / magnitude ) * move_ratio;
			ucmd->m_smove = ( ucmd->m_smove / magnitude ) * move_ratio;
		}
	}

	/* manage fakelag */
	if ( g::local->valid( ) ) {
		auto max_lag = 1;

		if ( air && !( g::local->flags( ) & 1 ) )
			max_lag = lag_air;
		else if ( move && g::local->vel( ).length_2d( ) > 0.1f )
			max_lag = lag_move;
		else if ( stand )
			max_lag = lag_stand;

		if ( !max_lag )
			max_lag = 1;

		g::send_packet = csgo::i::client_state->choked( ) >= max_lag;

		if ( GetAsyncKeyState( VK_XBUTTON1 ) ) {
			g::send_packet = csgo::i::client_state->choked( ) >= 14;
			ucmd->m_buttons = ( csgo::i::client_state->choked( ) > 7 ? ( ucmd->m_buttons | 4 ) : ( ucmd->m_buttons & ~4 ) ) | 0x400000;
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
		|| ucmd->m_buttons & 1
		|| ucmd->m_buttons & 32
		|| ucmd->m_buttons & 2048 )
		return;

	/* manage antiaim */ {
		if ( !( g::local->flags( ) & 1 ) ) {
			if ( air ) {
				ucmd->m_angs.y += base_yaw_air;
				ucmd->m_angs.x = pitch_air;

				if ( jitter_air && !g::send_packet )
					ucmd->m_angs.y += aa::flip ? -60.0f : 60.0f;
				else if ( desync_air && !g::send_packet )
					ucmd->m_angs.y += desync_side_air ? -60.0f : 60.0f;

				ucmd->m_angs.y += aa::flip ? -jitter_amount_air : jitter_amount_air;
			}
		}
		else if ( g::local->vel( ).length_2d( ) > 0.1f ) {
			if ( move && g::local->vel( ).length_2d( ) > 0.1f ) {
				ucmd->m_angs.y += base_yaw_move;
				ucmd->m_angs.x = pitch_move;

				if ( jitter_move && !g::send_packet )
					ucmd->m_angs.y += aa::flip ? -60.0f : 60.0f;
				else if ( desync_move && !g::send_packet )
					ucmd->m_angs.y += desync_side_move ? -60.0f : 60.0f;

				ucmd->m_angs.y += aa::flip ? -jitter_amount_move : jitter_amount_move;
			}
		}
		else if ( stand ) {
			ucmd->m_angs.y += base_yaw_stand;
			ucmd->m_angs.x = pitch_stand;

			if ( desync_stand ) {
				if ( jitter_stand ) {
					/* go 180 from update location to trigger 979 activity */
					if ( lby::in_update ) {
						ucmd->m_angs.y += aa::flip ? 125.0f : -125.0f;
						g::send_packet = false;
					}
					else if ( !g::send_packet ) {
						ucmd->m_angs.y += aa::flip ? -120.0f : 120.0f;
					}
				}
				else {
					/* go 180 from update location to trigger 979 activity */
					if ( lby::in_update ) {
						ucmd->m_angs.y += desync_side_stand ? 125.0f : -125.0f;
						g::send_packet = false;
					}
					else if ( !g::send_packet ) {
						ucmd->m_angs.y += desync_side_stand ? -80.0f : 80.0f;
					}
				}
			}

			ucmd->m_angs.y += aa::flip ? -jitter_amount_stand : jitter_amount_stand;
		}
	}
}