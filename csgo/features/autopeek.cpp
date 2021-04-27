#include "autopeek.hpp"
#include "ragebot.hpp"
#include "../menu/options.hpp"
#include "../renderer/render.hpp"
#include "prediction.hpp"

void features::autopeek::draw ( ) {
	static auto& enabled = options::vars [ _ ( "ragebot.autopeek" ) ].val.b;
	static auto& color = options::vars [ _ ( "visuals.other.autopeek_color" ) ].val.c;

	if ( !enabled || !peek.m_fade || !g::local || !g::local->alive ( ) )
		return;

	/* sine wave from 0.0 to 1.0 */
	const auto fade_amount = sin ( cs::i::globals->m_curtime * 4.0f ) * 0.5f + 0.5f;
	const auto fade_radius = peek.m_fade * 16.0f/* radius */ + fade_amount * 1.0f /* breathing effect */;

	render::circle3d ( peek.m_pos, fade_radius, 48, rgba ( static_cast< int >( color.r * 255.0f ), static_cast< int >( color.g * 255.0f ), static_cast< int >( color.b * 255.0f ), static_cast< int >( color.a * peek.m_fade * 255.0f ) ) );
	render::circle3d ( peek.m_pos, fade_radius, 48, rgba ( static_cast< int >( color.r * 255.0f ), static_cast< int >( color.g * 255.0f ), static_cast< int >( color.b * 255.0f ), static_cast< int >( peek.m_fade * 255.0f ) ), true, std::lerp ( 1.5f, 3.0f, fade_amount ) );
}

void features::autopeek::run ( ucmd_t* ucmd, float& side_move, float& fwd_move, vec3_t& move_ang ) {
	static auto& enabled = options::vars [ _ ( "ragebot.autopeek" ) ].val.b;
	static auto& autopeek_key = options::vars [ _ ( "ragebot.autopeek_key" ) ].val.i;
	static auto& autopeek_key_mode = options::vars [ _ ( "ragebot.autopeek_key_mode" ) ].val.i;

	peek.m_fade = std::clamp ( peek.m_fade + ( peek.m_recorded ? 1.0f : -1.0f ) / 0.25f/* time to fade (sec)*/ * cs::i::globals->m_frametime, 0.0f, 1.0f );

	if ( !enabled || !g::local || !g::local->alive ( ) || !(g::local->flags() & flags_t::on_ground) ) {
		peek.m_time = 0.0f;
		peek.m_retrack = peek.m_recorded = false;
		peek.m_fade = 0.0f;
		return;
	}

	/* start retracking postion if we attacked */
	if ( !!(ucmd->m_buttons & buttons_t::attack ))
		peek.m_retrack = true;

	const auto key_active = utils::keybind_active ( autopeek_key, autopeek_key_mode );

	/* record initial position if peek key pressed */
	if ( key_active && !peek.m_recorded ) {
		peek.m_pos = g::local->abs_origin ( );
		peek.m_time = cs::i::globals->m_curtime;
		peek.m_recorded = true;
	}
	/* stop recording if we already have position and are in movement phase */
	else if ( !key_active ) {
		peek.m_time = 0.0f;
		peek.m_retrack = peek.m_recorded = false;
		return;
	}

	/* retrack position */
	if ( peek.m_retrack ) {
		/* move towards original position */
		move_ang.y = cs::normalize ( cs::calc_angle ( g::local->abs_origin ( ), peek.m_pos ).y );

		/* move with full speed */
		fwd_move = 450.0f;
		side_move = 0.0f;

		const auto dist_to_target = ( peek.m_pos - g::local->abs_origin ( ) ).length_2d ( );

		/* start slowing down if we will reach the target within stopping speed */
		if ( dist_to_target < 25.0f ) {
			/* if we reached target position, tracking finished */
			/* calculate how many ticks early we need to stop to hit the center */
			static auto predict_stop = [ ] ( vec3_t& vel ) {
				const auto speed = vel.length ( );

				if ( speed >= 0.1f ) {
					const auto stop_speed = std::max< float > ( speed, g::cvars::sv_stopspeed->get_float ( ) );
					vel *= std::max< float > ( 0.0f, speed - g::cvars::sv_friction->get_float ( ) * stop_speed * cs::i::globals->m_ipt / speed );
				}
			};

			auto ticks_until_stop = 0;
			auto pred_vel = features::prediction::vel;
			auto needed_dist = 0.0f;

			for ( ticks_until_stop = 0; ticks_until_stop < 16; ++ticks_until_stop ) {
				if ( pred_vel.length_2d ( ) < 5.0f )
					break;

				needed_dist += pred_vel.length_2d ( ) * cs::ticks2time ( 1 );

				predict_stop ( pred_vel );
			}

			if ( dist_to_target <= needed_dist && features::prediction::vel.length_2d ( ) > 5.0f )
				features::ragebot::slow ( ucmd, side_move, fwd_move );

			/* stop tracking if we reached target */
			if ( dist_to_target < 12.0f )
				peek.m_retrack = false;
		}
	}
}