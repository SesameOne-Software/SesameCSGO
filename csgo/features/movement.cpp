#include "movement.hpp"
#include "../menu/menu.hpp"
#include "../globals.hpp"
#include "../menu/options.hpp"
#include "exploits.hpp"
#include "prediction.hpp"

#include "ragebot.hpp"

#undef min
#undef max

static float onetap_delta_yaw = 0.0f;
static bool was_autostrafing = false;

/* turn speed */
static float ot__cfg__autostrafe_turn_speed = 80.0f;

/*
float onetap_autostrafer_choose_dir ( float delta_yaw, vec3_t& angles, buttons_t buttons ) {
	float wanted_dir = 0.0f;

	const bool left = ( static_cast< int >( buttons ) >> 9 ) & 0xFFFFFF01;
	const bool right = ( static_cast< int >( buttons ) >> 10 ) & 1;
	const bool front = ( static_cast< int >( buttons ) >> 3 ) & 1;

	if ( front || !( ( static_cast< int >( buttons ) >> 4 ) & 1 ) ) {
		if ( left ) {
			if ( front )
				wanted_dir = 45.0f;
			else
				wanted_dir = 90.0f;
		}
		if ( right ) {
			if ( front )
				wanted_dir = -45.0f;
			else
				wanted_dir = -90.0f;
		}
	}
	else {
		//if ( left ) {
		//	if ( right )
		//		wanted_dir = -135.0f;
		//	else
		//		wanted_dir = 135.0f;
		//}
		//else if ( !right )
		//	wanted_dir = -180.0f;
		//else
		//	wanted_dir = -135.0f;

		if ( left ) {
			wanted_dir = 135.0f;

			if ( !right )
				goto LABEL_16;
		}
		else if ( !right ) {
			wanted_dir = -180.0f;
			goto LABEL_16;
		}

		wanted_dir = -135.0f;
	}

LABEL_16:
	float delta_yaw_1 = onetap_delta_yaw;

	if ( wanted_dir != 0.0f ) {
		const auto delta_target_yaw = cs::normalize( wanted_dir - cs::normalize( onetap_delta_yaw ) );
		const auto min_move = std::min ( delta_yaw, abs( delta_target_yaw ) );

		delta_yaw_1 = delta_target_yaw <= 0.0f ? -min_move : min_move;
	}

	onetap_delta_yaw = delta_yaw_1;

	dbg_print ( "onetap_delta_yaw: %.2f\n", onetap_delta_yaw );

	delta_yaw_1 += angles.y;

	return cs::normalize( delta_yaw_1 );
}

void onetap_autostrafer_multidir ( ucmd_t* cmd, vec3_t& old_angs ) {
	const auto speed = features::prediction::vel.length_2d ( );

	if ( !!( cmd->m_buttons & buttons_t::jump ) && speed >= 30.0f ) {
		if ( !( g::local->flags ( ) & flags_t::on_ground ) ) {
			auto turn_rate = cs::rad2deg ( asin ( 30.0f / speed ) ) * 0.5f;
			const auto base_delta_yaw_rate = was_autostrafing ? 180.0f : ( abs ( turn_rate ) * ( ot__cfg__autostrafe_turn_speed / 100.0f + 1.0f ) );

			const auto strafer_target_dir = onetap_autostrafer_choose_dir ( base_delta_yaw_rate, old_angs, cmd->m_buttons );
			const auto vel_dir = cs::normalize( cs::rad2deg ( atan2 ( features::prediction::vel.y, features::prediction::vel.x ) ) );
			const auto delta_target_yaw = cs::normalize( strafer_target_dir - vel_dir );
			const auto sign = abs ( delta_target_yaw ) <= 1.0f ? cmd->m_cmdnum % 2 : ( delta_target_yaw > 0.0f );
			const auto move_with_dir = sign ? 450.0f : -450.0f;

			if ( sign )
				turn_rate = -turn_rate;

			const auto turn_rate_rad = cs::deg2rad ( turn_rate );

			cmd->m_smove = 0.0f;
			cmd->m_fmove = g::cvars::cl_forwardspeed->get_float ( );

			const auto dir_sin = sin ( turn_rate_rad );
			const auto dir_cos = cos ( turn_rate_rad );

			const auto side_rot = cmd->m_smove * dir_cos - cmd->m_fmove * dir_sin;
			const auto fwd_rot = cmd->m_smove * dir_sin - cmd->m_fmove * dir_cos;

			dbg_print ( "side: %.2f\n", side_rot );
			dbg_print ( "forward: %.2f\n", fwd_rot );

			cmd->m_smove = std::clamp ( side_rot, -g::cvars::cl_sidespeed->get_float ( ), g::cvars::cl_sidespeed->get_float ( ) );
			cmd->m_fmove = std::clamp ( fwd_rot, -g::cvars::cl_forwardspeed->get_float ( ), g::cvars::cl_forwardspeed->get_float ( ) );

			old_angs.y = strafer_target_dir;

			was_autostrafing = false;
		}
	}
	else {
		onetap_delta_yaw = 0.0f;
		was_autostrafing = true;
	}
}

void features::movement::run( ucmd_t* ucmd, vec3_t& old_angs ) {
	static auto& bhop = options::vars [ _ ( "misc.movement.bhop" ) ].val.b;
	static auto& strafer = options::vars [ _ ( "misc.movement.auto_strafer" ) ].val.b;
	static auto& directional = options::vars [ _ ( "misc.movement.omnidirectional_auto_strafer" ) ].val.b;
	static auto& accurate_move = options::vars [ _ ( "misc.movement.accurate_move" ) ].val.b;

	static flags_t last_flags;

	if ( !g::local->valid ( )
		|| g::local->movetype ( ) == movetypes_t::noclip
		|| g::local->movetype ( ) == movetypes_t::ladder )
		return;

	if ( strafer && directional ) {
		onetap_autostrafer_multidir ( ucmd, old_angs );
	}
	else if ( strafer && !directional && !!( ucmd->m_buttons & buttons_t::jump ) && !( g::local->flags ( ) & flags_t::on_ground ) && features::prediction::vel.length_2d ( ) >= 30.0f ) {
		if ( abs ( ucmd->m_mousedx ) > 2 ) {
			ucmd->m_smove = ucmd->m_mousedx < 0 ? -g::cvars::cl_sidespeed->get_float ( ) : g::cvars::cl_sidespeed->get_float ( );
		}
		else {
			ucmd->m_fmove = g::cvars::cl_forwardspeed->get_float ( );
			ucmd->m_smove = ucmd->m_cmdnum % 2 ? -g::cvars::cl_sidespeed->get_float ( ) : g::cvars::cl_sidespeed->get_float ( );
		}
	}

	if ( accurate_move
		&& !( ucmd->m_buttons & buttons_t::jump )
		&& !!( last_flags & flags_t::on_ground )
		&& !!( g::local->flags ( ) & flags_t::on_ground )
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
	
	if ( bhop && !!( ucmd->m_buttons & buttons_t::jump ) && !( g::local->flags ( ) & flags_t::on_ground ) )
		ucmd->m_buttons &= ~buttons_t::jump;

	last_flags = g::local->flags ( );
}*/

void features::movement::run ( ucmd_t* ucmd, vec3_t& old_angs ) {
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
				/* credits to chambers, TY ^.^ */
				const auto back = !!( ucmd->m_buttons & buttons_t::back );
				const auto left = !!( ucmd->m_buttons & buttons_t::left );
				const auto right = !!( ucmd->m_buttons & buttons_t::right );
				const auto front = !!( ucmd->m_buttons & buttons_t::forward );

				static bool of, ort, ob, ol;

				auto vel = g::local->vel ( );
				auto wanted_dir = 0.0f;

				enum e_strafe_dirs {
					qd_front,
					qd_left,
					qd_right,
					qd_back
				};

				//	easy strafe direction selection (4dir)
				{
					std::array< int, 4 > q_dirs;

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

					wanted_dir = old_angs.y;

					if ( ifr && il ) wanted_dir += 45.0f;
					else if ( ib && il ) wanted_dir += 90.0f + 45.0f;
					else if ( ifr && ir ) wanted_dir -= 45.0f;
					else if ( ib && ir ) wanted_dir -= 90.0f + 45.0f;
					else if ( ib ) wanted_dir -= 180.0f;
					else if ( ir ) wanted_dir -= 90.0f;
					else if ( il ) wanted_dir += 90.0f;

					wanted_dir = cs::normalize ( wanted_dir );
				}

				//	for easy strafe
				of = front;
				ob = back;
				ol = left;
				ort = right;

				const auto turn_rate = cs::rad2deg ( asin ( 30.0f / vel.length_2d ( ) ) ) * 0.5f;
				const auto vel_dir = cs::rad2deg ( atan2 ( vel.y, vel.x ) );
				const auto target_vel_diff = cs::normalize ( wanted_dir - vel_dir );
				const auto max_angle_change = std::clamp<float> ( cs::rad2deg ( atan2 ( 15.0f, vel.length_2d ( ) ) ), 0.0f, 45.0f );
				const auto sign = target_vel_diff > 0.0f ? -1.0f : 1.0f;
				
				old_angs.y = cs::normalize ( vel_dir - sign * max_angle_change );

				ucmd->m_smove = sign * g::cvars::cl_sidespeed->get_float ( );
				ucmd->m_fmove = abs ( target_vel_diff ) >= max_angle_change ? 0.0f : g::cvars::cl_forwardspeed->get_float ( );

				if ( abs ( target_vel_diff ) < max_angle_change ) {
					ucmd->m_smove = ucmd->m_cmdnum % 2 ? g::cvars::cl_sidespeed->get_float ( ) : -g::cvars::cl_sidespeed->get_float ( );
					old_angs.y = cs::normalize ( old_angs.y + ( ucmd->m_cmdnum % 2 ? turn_rate : -turn_rate ) );
				}
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
}