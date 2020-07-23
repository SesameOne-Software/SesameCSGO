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

/*
@CBRS

enum e_strafe_dirs {
    qd_front,
    qd_left,
    qd_right,
    qd_back
};

void c_misc::strafe( ) {
    if ( !cs::local->valid( )
         || cs::local->movetype( ) == e_movetype::movetype_noclip
         || cs::local->movetype( ) == e_movetype::movetype_ladder
         || !ui::menu.opts.misc.strafe.val )
        return;


    const auto back = frontend.cmd->buttons & in_back;
    const auto left = frontend.cmd->buttons & in_moveleft;
    const auto right = frontend.cmd->buttons & in_moveright;
    const auto front = frontend.cmd->buttons & in_forward;

    static bool of, ort, ob, ol;

    auto vel = cs::local->velocity( );

    if ( !( cs::local->flags( ) & fl_onground )) {
        static auto cl_sidespeed = cs::cvars->find_var( "cl_sidespeed" );

        //	manually turning
        if ( abs( frontend.cmd->mouse_dx ) > 2 ) {
            frontend.cmd->move.y = frontend.cmd->mouse_dx < 0 ? -cl_sidespeed->get_float( ) : cl_sidespeed->get_float( );
            return;
        }

        //	keeps us fast when moving almost straight
        frontend.cmd->move.x = std::clamp< float >( 5850.f / vel.length_2d( ), -450.f, 450.f );

        //	easy strafe direction selection (4dir)
        {
            static std::array< int, 4 > q_dirs;

            auto handle_dir = [ & ]( bool dir, bool &odir, int qd ) {
                auto old_qd_v = q_dirs[ qd ];

                //	on release
                if ( !dir ) {

                    //	set key priority to invalid
                    q_dirs[ qd ] = -1;

                    //	set all keys higher than we were to their priority - 1
                    //	this allows the key to be pushed to the top again without breaking shit
                    if ( odir ) {
                        for ( int i = qd_front; i <= qd_back; i++ ) {
                            if ( q_dirs[ i ] >= old_qd_v )
                                q_dirs[ i ] = std::max< int >( q_dirs[ i ] - 1, -1 );
                        }
                    }
                }

                //	push to the top if freshly pressed
                if ( dir && !odir ) {
                    q_dirs[ qd ] = 3;
                }
            };

            handle_dir( front, of, qd_front );
            handle_dir( left, ol, qd_left );
            handle_dir( right, ort, qd_right );
            handle_dir( back, ob, qd_back );

            int best = 0;

            //	bullshit to set flags so we can do less edge case handling down below
            {
                int chosen = qd_front, val = -1;
                for ( int i = qd_front; i <= qd_back; i++ ) {
                    if ( q_dirs[ i ] > val ) {
                        val = q_dirs[ i ];
                        chosen = i;
                    }
                }

                best |= 1 << chosen;

                val = -1;
                chosen = -1;
                for ( int i = qd_front; i <= qd_back; i++ ) {
                    if ( q_dirs[ i ] > val && !( best & ( 1 << i ))) {
                        val = q_dirs[ i ];
                        chosen = i;
                    }
                }

                if ( chosen >= 0 )
                    best |= 1 << chosen;
            }

            //	pretty much no edge cases now :)

            auto il = best & ( 1 << qd_left );
            auto ir = best & ( 1 << qd_right );
            auto ib = best & ( 1 << qd_back );
            auto ifr = best & ( 1 << qd_front );

            if ( ifr && il ) frontend.cmd->view_angles.y += 45;
            else if ( ib && il ) frontend.cmd->view_angles.y += 90 + 45;
            else if ( ifr && ir ) frontend.cmd->view_angles.y -= 45;
            else if ( ib && ir ) frontend.cmd->view_angles.y -= 90 + 45;
            else if ( ib ) frontend.cmd->view_angles.y -= 180;
            else if ( ir ) frontend.cmd->view_angles.y -= 90;
            else if ( il ) frontend.cmd->view_angles.y += 90;
        }

        //	for easy strafe
        of = front;
        ob = back;
        ol = left;
        ort = right;

        //	find difference in goal vs current vel as a yaw
        const float vel_dir = RAD2DEG( std::atan2( vel.y, vel.x ));
        const float target_vel_diff = math::normalize_yaw( frontend.cmd->view_angles.y - vel_dir );

        //	if we are turning greatly, don't do any forwardmove to keep the angle tight
        auto change_diff = std::clamp< float >(RAD2DEG( std::asinf( 15.f / vel.length_2d( ))), 0.f, 90.f );

        frontend.cmd->view_angles.y = vel_dir - ( target_vel_diff > 0.f ? -change_diff : change_diff );
        frontend.cmd->move.y = ( target_vel_diff > 0.f ) ? -cl_sidespeed->get_float( ) : cl_sidespeed->get_float( );

        if ( fabsf( target_vel_diff ) > change_diff ) {
            frontend.cmd->move.x = 0;
        }
    }
}

*/