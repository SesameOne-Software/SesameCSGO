#include "movement.hpp"
#include "../menu/menu.hpp"
#include "../globals.hpp"
#include "../menu/options.hpp"
#include "exploits.hpp"
#include "prediction.hpp"

#include "ragebot.hpp"

#undef min
#undef max

void features::movement::directional_strafer ( ucmd_t* cmd, vec3_t& old_angs ) {
	VMP_BEGINMUTATION ( );
	static int strafer_flags = 0;

	if ( !!( g::local->flags ( ) & flags_t::on_ground ) ) {
		strafer_flags = 0;
		return;
	}

	auto velocity = g::local->vel ( );
	auto velocity_len = velocity.length_2d ( );

	if ( velocity_len <= 0.0f ) {
		strafer_flags = 0;
		return;
	}

	auto ideal_step = std::min ( 90.0f, 845.5f / velocity_len );
	auto velocity_yaw = ( velocity.y || velocity.x ) ? cs::rad2deg ( atan2f ( velocity.y, velocity.x ) ) : 0.0f;

	auto unmod_angles = old_angs;
	auto angles = old_angs;

	if ( velocity_len < 2.0f && !!( cmd->m_buttons & buttons_t::jump ) )
		cmd->m_fmove = 450.0f;

	auto forward_move = cmd->m_fmove;
	auto onground = !!( g::local->flags ( ) & flags_t::on_ground );

	if ( forward_move || cmd->m_smove ) {
		cmd->m_fmove = 0.0f;

		if ( velocity_len != 0.0f && abs ( velocity.z ) != 0.0f ) {
			if ( !onground ) {
			DO_IT_AGAIN:
				auto fwd = cs::angle_vec ( angles );
				auto right = fwd.cross_product ( vec3_t ( 0.0f, 0.0f, 1.0f ) );

				auto v262 = ( fwd.x * forward_move ) + ( cmd->m_smove * right.x );
				auto v263 = ( right.y * cmd->m_smove ) + ( fwd.y * forward_move );

				angles.y = ( v262 || v263 ) ? cs::rad2deg ( atan2f ( v263, v262 ) ) : 0.0f;
			}
		}
	}

	auto yaw_to_use = 0.0f;

	strafer_flags &= ~4;

	if ( !onground ) {
		auto clamped_angles = angles.y;

		if ( clamped_angles < -180.0f ) clamped_angles += 360.0f;
		if ( clamped_angles > 180.0f ) clamped_angles -= 360.0f;

		yaw_to_use = old_angs.y;

		strafer_flags |= 4;
	}

	if ( strafer_flags & 4 ) {
		auto diff = angles.y - yaw_to_use;

		if ( diff < -180.0f ) diff += 360.0f;
		if ( diff > 180.0f ) diff -= 360.0f;

		if ( abs ( diff ) > ideal_step && abs ( diff ) <= 30.0f ) {
			auto move = 450.0f;

			if ( diff < 0.0f )
				move *= -1.0f;

			cmd->m_smove = move;
			return;
		}
	}

	auto diff = angles.y - velocity_yaw;

	if ( diff < -180.0f ) diff += 360.0f;
	if ( diff > 180.0f ) diff -= 360.0f;

	auto step = 0.6f * ( ideal_step + ideal_step );
	auto side_move = 0.0f;

	if ( abs ( diff ) > 170.0f && velocity_len > 80.0f || diff > step && velocity_len > 80.0f ) {
		angles.y = step + velocity_yaw;
		cmd->m_smove = -450.0f;
	}
	else if ( -step <= diff || velocity_len <= 80.0f ) {
		if ( strafer_flags & 1 ) {
			angles.y -= ideal_step;
			cmd->m_smove = -450.0f;
		}
		else {
			angles.y += ideal_step;
			cmd->m_smove = 450.0f;
		}
	}
	else {
		angles.y = velocity_yaw - step;
		cmd->m_smove = 450.0f;
	}

	if ( !( cmd->m_buttons & buttons_t::back ) && !cmd->m_smove )
		goto DO_IT_AGAIN;

	strafer_flags ^= ( strafer_flags ^ ~strafer_flags ) & 1;

	if ( angles.y < -180.0f ) angles.y += 360.0f;
	if ( angles.y > 180.0f ) angles.y -= 360.0f;

	cs::rotate_movement ( cmd, angles );
	VMP_END ( );
}

void features::movement::run ( ucmd_t* ucmd, vec3_t& old_angs ) {
	VMP_BEGINMUTATION ( );
	static auto& bhop = options::vars [ _ ( "misc.movement.bhop" ) ].val.b;
	static auto& strafer = options::vars [ _ ( "misc.movement.auto_strafer" ) ].val.b;
	static auto& directional = options::vars [ _ ( "misc.movement.omnidirectional_auto_strafer" ) ].val.b;
	static auto& accurate_move = options::vars [ _ ( "misc.movement.accurate_move" ) ].val.b;
	static flags_t last_flags;

	if ( !g::local->valid ( )
		|| g::local->movetype ( ) == movetypes_t::noclip
		|| g::local->movetype ( ) == movetypes_t::ladder ) {
		return;
	}

	if ( !( g::local->flags ( ) & flags_t::on_ground ) ) {
		last_flags = g::local->flags ( );

		if ( bhop && !!( ucmd->m_buttons & buttons_t::jump ) )
			ucmd->m_buttons &= ~buttons_t::jump;

		if ( strafer && features::prediction::vel.length_2d ( ) >= 30.0f ) {
			if ( directional ) {
				directional_strafer ( ucmd, old_angs );
			}
			else {
				if ( std::abs ( ucmd->m_mousedx ) > 2 ) {
					ucmd->m_smove = ucmd->m_mousedx < 0 ? -g::cvars::cl_sidespeed->get_float ( ) : g::cvars::cl_sidespeed->get_float ( );
				}
				else {
					ucmd->m_smove = ucmd->m_cmdnum % 2 ? -g::cvars::cl_sidespeed->get_float ( ) : g::cvars::cl_sidespeed->get_float ( );
					ucmd->m_fmove = g::cvars::cl_forwardspeed->get_float ( );
				}
			}
		}
	}
	else {
		if ( accurate_move
			&& !!( last_flags & flags_t::on_ground )
			&& !( ucmd->m_buttons & buttons_t::back )
			&& !( ucmd->m_buttons & buttons_t::left )
			&& !( ucmd->m_buttons & buttons_t::right )
			&& !( ucmd->m_buttons & buttons_t::forward )
			&& !exploits::in_exploit ) {
			if ( g::local->vel ( ).length_2d ( ) > 15.0f ) {
				auto fwd = cs::angle_vec ( old_angs );
				auto right = fwd.cross_product ( vec3_t ( 0.0f, 0.0f, 1.0f ) );

				auto wishvel = -g::local->vel ( );

				ucmd->m_fmove = ( wishvel.y - ( right.y / right.x ) * wishvel.x ) / ( fwd.y - ( right.y / right.x ) * fwd.x );
				ucmd->m_smove = ( wishvel.x - fwd.x * ucmd->m_fmove ) / right.x;

				ucmd->m_buttons &= ~buttons_t::walk;
			}
		}

		last_flags = g::local->flags ( );
	}
	VMP_END ( );
}