#include "movement.hpp"
#include "../menu/menu.hpp"
#include "../globals.hpp"
#include "../menu/options.hpp"

#include "ragebot.hpp"

enum e_strafe_dirs {
	qd_front,
	qd_left,
	qd_right,
	qd_back
};

void features::movement::run( ucmd_t* ucmd, vec3_t& old_angs ) {
	static auto& bhop = options::vars [ _ ( "misc.movement.bhop" ) ].val.b;
	static auto& auto_forward = options::vars [ _ ( "misc.movement.auto_forward" ) ].val.b;
	static auto& strafer = options::vars [ _ ( "misc.movement.auto_strafer" ) ].val.b;
	static auto& directional = options::vars [ _ ( "misc.movement.omnidirectional_auto_strafer" ) ].val.b;
	static auto& accurate_move = options::vars [ _ ( "misc.movement.accurate_move" ) ].val.b;

	if ( !g::local->valid( )
		|| g::local->movetype ( ) == movetypes_t::noclip
		|| g::local->movetype( ) == movetypes_t::ladder )
		return;

	if ( !( g::local->flags( ) & flags_t::on_ground ) ) {
		if ( bhop && !!(ucmd->m_buttons & buttons_t::jump) )
			ucmd->m_buttons &= ~buttons_t::jump /*IN_JUMP*/;

		if ( strafer ) {
			if ( directional ) {
				/* credits to chambers, TY ^.^ */
				const auto back = !!(ucmd->m_buttons & buttons_t::back);
				const auto left = !!(ucmd->m_buttons & buttons_t::left);
				const auto right = !!( ucmd->m_buttons & buttons_t::right);
				const auto front = !!( ucmd->m_buttons & buttons_t::forward);

				static bool of, ort, ob, ol;

				auto vel = g::local->vel ( );

				if ( !( g::local->flags ( ) & flags_t::on_ground ) ) {
					//	manually turning
					if ( abs ( ucmd->m_mousedx ) > 2 ) {
						ucmd->m_smove = ucmd->m_mousedx < 0 ? -g::cvars::cl_sidespeed->get_float ( ) : g::cvars::cl_sidespeed->get_float ( );
						return;
					}

					//	keeps us fast when moving almost straight
					ucmd->m_fmove = std::clamp< float > ( 5850.f / vel.length_2d ( ), -g::cvars::cl_forwardspeed->get_float ( ), g::cvars::cl_forwardspeed->get_float() );

					//	easy strafe direction selection (4dir)
					{
						static std::array< int, 4 > q_dirs;

						auto handle_dir = [ & ] ( bool dir, bool& odir, int qd ) {
							auto old_qd_v = q_dirs [ qd ];

							//	on release
							if ( !dir ) {

								//	set key priority to invalid
								q_dirs [ qd ] = -1;

								//	set all keys higher than we were to their priority - 1
								//	this allows the key to be pushed to the top again without breaking shit
								if ( odir ) {
									for ( int i = qd_front; i <= qd_back; i++ ) {
										if ( q_dirs [ i ] >= old_qd_v )
											q_dirs [ i ] = std::max< int > ( q_dirs [ i ] - 1, -1 );
									}
								}
							}

							//	push to the top if freshly pressed
							if ( dir && !odir ) {
								q_dirs [ qd ] = 3;
							}
						};

						handle_dir ( front, of, qd_front );
						handle_dir ( left, ol, qd_left );
						handle_dir ( right, ort, qd_right );
						handle_dir ( back, ob, qd_back );

						int best = 0;

						//	bullshit to set flags so we can do less edge case handling down below
						{
							int chosen = qd_front, val = -1;
							for ( int i = qd_front; i <= qd_back; i++ ) {
								if ( q_dirs [ i ] > val ) {
									val = q_dirs [ i ];
									chosen = i;
								}
							}

							best |= 1 << chosen;

							val = -1;
							chosen = -1;
							for ( int i = qd_front; i <= qd_back; i++ ) {
								if ( q_dirs [ i ] > val && !( best & ( 1 << i ) ) ) {
									val = q_dirs [ i ];
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

						if ( ifr && il ) old_angs.y += 45.0f;
						else if ( ib && il ) old_angs.y += 90.0f + 45.0f;
						else if ( ifr && ir ) old_angs.y -= 45.0f;
						else if ( ib && ir ) old_angs.y -= 90.0f + 45.0f;
						else if ( ib ) old_angs.y -= 180.0f;
						else if ( ir ) old_angs.y -= 90.0f;
						else if ( il ) old_angs.y += 90.0f;

						cs::clamp ( old_angs );
					}

					//	for easy strafe
					of = front;
					ob = back;
					ol = left;
					ort = right;

					//	find difference in goal vs current vel as a yaw
					const float vel_dir = cs::rad2deg ( std::atan2f ( vel.y, vel.x ) );
					const float target_vel_diff = cs::normalize ( old_angs.y - vel_dir );

					//	if we are turning greatly, don't do any forwardmove to keep the angle tight
					auto change_diff = std::clamp< float > ( cs::rad2deg ( std::asinf ( 15.0f / vel.length_2d ( ) ) ), 0.0f, 90.0f );

					old_angs.y = vel_dir - ( target_vel_diff > 0.0f ? -change_diff : change_diff );
					ucmd->m_smove = ( target_vel_diff > 0.0f ) ? -g::cvars::cl_sidespeed->get_float ( ) : g::cvars::cl_sidespeed->get_float ( );

					if ( fabsf ( target_vel_diff ) > change_diff )
						ucmd->m_fmove = 0.0f;
				}
			}
			else {
				if ( std::abs( ucmd->m_mousedx ) > 2 ) {
					ucmd->m_smove = ucmd->m_mousedx < 0 ? -g::cvars::cl_sidespeed->get_float ( ) : g::cvars::cl_sidespeed->get_float ( );
					return;
				}
				else
					ucmd->m_smove = ucmd->m_cmdnum % 2 ? -g::cvars::cl_sidespeed->get_float ( ) : g::cvars::cl_sidespeed->get_float ( );

				if ( auto_forward )
					ucmd->m_fmove = g::cvars::cl_forwardspeed->get_float ( );
			}
		}
	}
	else {
	if ( accurate_move
		&& !( ucmd->m_buttons & buttons_t::back )
		&& !( ucmd->m_buttons & buttons_t::left )
		&& !( ucmd->m_buttons & buttons_t::right )
		&& !( ucmd->m_buttons & buttons_t::forward ) ) {
		if ( g::local->vel ( ).length_2d ( ) > 12.0f ) {
			auto fwd = cs::angle_vec ( old_angs );
			auto right = fwd.cross_product ( vec3_t ( 0.0f, 0.0f, 1.0f ) );

			auto wishvel = -g::local->vel ( );

			ucmd->m_fmove = ( wishvel.y - ( right.y / right.x ) * wishvel.x ) / ( fwd.y - ( right.y / right.x ) * fwd.x );
			ucmd->m_smove = ( wishvel.x - fwd.x * ucmd->m_fmove ) / right.x;

			ucmd->m_buttons &= ~buttons_t::walk;
		}
	}
	}
}