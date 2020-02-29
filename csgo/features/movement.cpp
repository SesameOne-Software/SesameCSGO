#include "movement.hpp"
#include "../menu/menu.hpp"
#include "../globals.hpp"

void features::movement::run( ucmd_t* ucmd ) {
	FIND( bool, bhop, "Misc.", "Movement", "Bhop", oxui::object_checkbox );
	FIND( bool, strafer, "Misc.", "Movement", "Strafer", oxui::object_checkbox );
	FIND( bool, directional, "Misc.", "Movement", "Directional", oxui::object_checkbox );

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

				if ( ucmd->m_buttons & 16 )
					ucmd->m_angs.y -= 180.0f;
				else if ( ucmd->m_buttons & 512 )
					ucmd->m_angs.y += 90.0f;
				else if ( ucmd->m_buttons & 1024 )
					ucmd->m_angs.y -= 90.0f;

				const auto vel_delta = csgo::normalize( ucmd->m_angs.y - yaw_speed );

				if ( speed <= 0.5f || speed == NAN || speed == INFINITE ) {
					ucmd->m_fmove = 450.0f;
					return;
				}

				ucmd->m_fmove = std::clamp( 5850.0f / speed, -450.0f, 450.0f );
				ucmd->m_smove = vel_delta > 0.0f ? -450.0f : 450.0f;
				ucmd->m_angs.y = csgo::normalize( ucmd->m_angs.y - vel_delta );
			}
			else {
				if ( std::abs( ucmd->m_mousedx ) > 2 ) {
					ucmd->m_smove = ucmd->m_mousedx < 0 ? -450.0f : 450.0f;
					return;
				}
				else
					ucmd->m_smove = ucmd->m_cmdnum % 2 ? -450.0f : 450.0f;

				ucmd->m_fmove = 450.0f;
			}
		}
	}
}