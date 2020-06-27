#include "movement.hpp"
#include "../menu/menu.hpp"
#include "../globals.hpp"

void features::movement::run( ucmd_t* ucmd, vec3_t& old_angs ) {
	OPTION ( bool, bhop, "Sesame->E->Movement->Main->Bunnyhop", oxui::object_checkbox );
	OPTION( bool, auto_forward, "Sesame->E->Movement->Main->Auto Forward", oxui::object_checkbox );
	OPTION( bool, strafer, "Sesame->E->Movement->Main->Auto Strafer", oxui::object_checkbox );
	OPTION( bool, directional, "Sesame->E->Movement->Main->Omni-Directional Auto Strafer", oxui::object_checkbox );

	if ( !g::local->valid( )
		|| g::local->movetype( ) == movetypes::movetype_noclip
		|| g::local->movetype( ) == movetypes::movetype_ladder )
		return;

	if ( !( g::local->flags( ) & 1 ) ) {
		if ( bhop && ucmd->m_buttons & 2 )
			ucmd->m_buttons &= ~2;

		if ( strafer ) {
			if ( directional ) {
				const auto speed = g::local->vel( ).length_2d( );
				const auto velocity = g::local->vel( );
				const auto yaw_speed = csgo::rad2deg( std::atan2f( velocity.y, velocity.x ) );

				if ( std::abs( ucmd->m_mousedx > 2 ) ) {
					ucmd->m_smove = ucmd->m_mousedx < 0 ? -450.0f : 450.0f;
					return;
				}

				const auto front = ucmd->m_buttons & 8;
				const auto back = ucmd->m_buttons & 16;
				const auto left = ucmd->m_buttons & 512;
				const auto right = ucmd->m_buttons & 1024;

				if ( front && left ) old_angs.y += 45.0f;
				else if ( back && left ) old_angs.y += 90.0f + 45.0f;
				else if ( front && right ) old_angs.y -= 45.0f;
				else if ( back && right ) old_angs.y -= 90.0f + 45.0f;
				else if ( back ) old_angs.y -= 180.0f;
				else if ( right ) old_angs.y -= 90.0f;
				else if ( left ) old_angs.y += 90.0f;

				const auto vel_delta = csgo::normalize ( old_angs.y - yaw_speed );

				ucmd->m_smove = vel_delta > 0.0f ? -450.0f : 450.0f;
				ucmd->m_fmove = std::clamp ( 300.0f / speed, -450.0f, 450.0f );

				old_angs.y = csgo::normalize ( old_angs.y - vel_delta );
			}
			else {
				if ( std::abs( ucmd->m_mousedx ) > 2 ) {
					ucmd->m_smove = ucmd->m_mousedx < 0 ? -450.0f : 450.0f;
					return;
				}
				else
					ucmd->m_smove = ucmd->m_cmdnum % 2 ? -450.0f : 450.0f;

				if ( auto_forward )
					ucmd->m_fmove = 450.0f;
			}
		}
	}
}