#include "ragebot.hpp"
#include <deque>
#include "autowall.hpp"
#include "../menu/menu.hpp"
#include "../globals.hpp"

void features::ragebot::best_point( player_t* pl, vec3_t& point, float& dmg ) {
	FIND( bool, head, "rage", "target selection", "head", oxui::object_checkbox );
	FIND( bool, neck, "rage", "target selection", "neck", oxui::object_checkbox );
	FIND( bool, chest, "rage", "target selection", "chest", oxui::object_checkbox );
	FIND( bool, pelvis, "rage", "target selection", "pelvis", oxui::object_checkbox );
	FIND( bool, arms, "rage", "target selection", "arms", oxui::object_checkbox );
	FIND( bool, legs, "rage", "target selection", "legs", oxui::object_checkbox );
	FIND( bool, feet, "rage", "target selection", "feet", oxui::object_checkbox );
	FIND( double, damage, "rage", "aimbot", "min dmg", oxui::object_slider );

	std::deque< int > hitboxes { };

	if ( head )
		hitboxes.push_back( 0 ); // head

	if ( neck )
		hitboxes.push_back( 1 ); // neck

	if ( pelvis )
		hitboxes.push_back( 2 ); // pelvis

	if ( feet ) {
		hitboxes.push_back( 11 ); // right foot
		hitboxes.push_back( 12 ); // left foot
	}

	if ( chest ) {
		hitboxes.push_back( 3 ); // body
		hitboxes.push_back( 5 ); // chest
	}

	if ( legs ) {
		hitboxes.push_back( 7 ); // right thigh
		hitboxes.push_back( 8 ); // left thigh
	}

	if ( arms ) {
		hitboxes.push_back( 18 ); // right forearm
		hitboxes.push_back( 16 ); // left forearm
	}

	auto best_point = vec3_t( );
	auto best_dmg = 0.0f;

	// find best point on best hitbox
	for ( auto& hb : hitboxes ) {
		auto mdl = pl->mdl( );

		if ( !mdl )
			continue;

		auto studio_mdl = csgo::i::mdl_info->studio_mdl( mdl );

		if ( !studio_mdl )
			continue;

		auto s = studio_mdl->hitbox_set( 0 );

		if ( !s )
			continue;

		auto hitbox = s->hitbox( hb );

		if ( !hitbox )
			continue;

		auto get_hitbox_pos = [ & ] ( ) {
			vec3_t vmin, vmax;

			VEC_TRANSFORM( hitbox->m_bbmin, pl->bone_accessor( ).get_bone( hitbox->m_bone ), vmin );
			VEC_TRANSFORM( hitbox->m_bbmax, pl->bone_accessor( ).get_bone( hitbox->m_bone ), vmax );

			auto pos = ( vmin + vmax ) * 0.5f;

			return pos;
		};

		auto a1 = csgo::calc_angle( get_hitbox_pos( ), g::local->eyes( ) );
		auto fwd = csgo::angle_vec( a1 );
		auto right = fwd.cross_product( vec3_t( 0.0f, 0.0f, 1.0f ) );
		auto left = -right;
		auto top = vec3_t( 0.0f, 0.0f, 1.0f );

		constexpr auto pointscale = 0.8f;

		auto rad_coeff = pointscale * hitbox->m_radius;

		// calculate best points on hitbox
		switch ( hb ) {
			// aim above head to try to hit
		case 0: {
			auto head = get_hitbox_pos( );
			auto head_top = head + top * rad_coeff;
			auto dmg1 = autowall::dmg( g::local, pl, g::local->eyes( ), head, hb );
			auto dmg2 = autowall::dmg( g::local, pl, g::local->eyes( ), head_top, hb );

			if ( dmg1 > best_dmg || dmg2 > best_dmg ) {
				if ( dmg1 > dmg2 || dmg1 == dmg2 ) {
					best_dmg = dmg1;
					best_point = head;
				}
				else if ( dmg2 > dmg1 ) {
					best_dmg = dmg2;
					best_point = head_top;
				}
			}
		} break;
			// body stuff (aim outwards)
		case 3:
		case 7:
		case 8:
		case 11:
		case 12: {
			auto body = get_hitbox_pos( );
			auto body_left = body + left * rad_coeff;
			auto body_right = body + right * rad_coeff;
			auto dmg1 = autowall::dmg( g::local, pl, g::local->eyes( ), body, hb );
			auto dmg2 = autowall::dmg( g::local, pl, g::local->eyes( ), body_left, hb );
			auto dmg3 = autowall::dmg( g::local, pl, g::local->eyes( ), body_right, hb );

			if ( dmg1 > best_dmg || dmg2 > best_dmg || dmg3 > best_dmg ) {
				if ( dmg1 > dmg2&& dmg1 > dmg3 || ( dmg1 == dmg2 && dmg1 == dmg3 ) ) {
					best_dmg = dmg1;
					best_point = body;
				}
				else if ( dmg2 > dmg1&& dmg2 > dmg3 ) {
					best_dmg = dmg2;
					best_point = body_left;
				}
				else if ( dmg3 > dmg1&& dmg3 > dmg2 ) {
					best_dmg = dmg3;
					best_point = body_right;
				}
			}
		} break;
			// no need to test multiple points on these
		case 9:
		case 10:
		case 13:
		case 14: {
			auto hitbox_pos = get_hitbox_pos( );
			auto dmg = autowall::dmg( g::local, pl, g::local->eyes( ), hitbox_pos, hb );

			if ( dmg > best_dmg ) {
				best_dmg = dmg;
				best_point = hitbox_pos;
			}
		} break;
		}
	}

	if ( best_dmg > 0.0f && best_dmg >= damage || best_dmg >= pl->health( ) ) {
		point = best_point;
		dmg = best_dmg;
	}

	best_dmg = 0.0f;
}

void features::ragebot::run( ucmd_t* ucmd ) {
	FIND( bool, ragebot, "rage", "aimbot", "ragebot", oxui::object_checkbox );
	FIND( double, hitchance, "rage", "aimbot", "hit chance", oxui::object_slider );
	FIND( bool, autoshoot, "rage", "aimbot", "auto-shoot", oxui::object_checkbox );
	FIND( bool, silent, "rage", "aimbot", "silent", oxui::object_checkbox );
	FIND( bool, autoscope, "rage", "aimbot", "auto-scope", oxui::object_checkbox );

	if ( !ragebot )
		return;

	vec3_t engine_ang;
	csgo::i::engine->get_viewangles( engine_ang );

	auto can_shoot = [ & ] ( ) {
		return g::local->weapon( ) && g::local->weapon( )->next_primary_attack( ) <= csgo::ticks2time( g::local->tick_base( ) ) && g::local->weapon( )->ammo( );
	};

	auto optimize_hitchance = [ & ] ( ) {
		if ( !g::local->weapon( ) )
			return 0.0f;

		if ( g::local->weapon( )->inaccuracy( ) <= 0.01f )
			return 100.0f;

		return 1.0f / g::local->weapon( )->inaccuracy( );
	};

	auto lerp = [ ] ( ) {
		auto cv_lerp = 0.031000f;
		auto cv_ratio = 2.0f;
		auto cv_udrate = 1.0f / csgo::i::globals->m_ipt;
		return std::max< float >( cv_lerp, cv_ratio / cv_udrate );
	};

	player_t* best_pl = nullptr;
	auto best_ang = vec3_t( );
	auto best_fov = 180.0f;
	auto best_dmg = 0.0f;

	csgo::for_each_player( [ & ] ( player_t* pl ) {
		if ( pl->team( ) == g::local->team( ) || pl->immune( ) )
			return;
		
		vec3_t point;
		float dmg = 0.0f;

		best_point( pl, point, dmg );

		auto ang = csgo::calc_angle( g::local->eyes( ), point );
		csgo::clamp( ang );

		const auto fov = ang.dist_to( engine_ang );

		if ( dmg > best_dmg && fov < best_fov ) {
			best_pl = pl;
			best_dmg = dmg;
			best_fov = fov;
			best_ang = ang;
		}
	} );

	if ( autoshoot && !can_shoot( ) ) {
		ucmd->m_buttons &= ~1;
	}
	else if ( can_shoot( ) ) {
		auto should_aim = best_dmg > 0.0f && optimize_hitchance( ) >= hitchance;

		if ( !autoshoot )
			should_aim = ucmd->m_buttons & 1 && best_dmg > 0.0f;

		if ( should_aim ) {
			if ( autoshoot )
				ucmd->m_buttons |= 1;

			if ( autoscope && !g::local->scoped( ) )
				ucmd->m_buttons |= 0x80000;

			auto ang = best_ang - g::local->aim_punch( ) * 2.0f;
			csgo::clamp( ang );

			ucmd->m_angs = ang;

			if ( !silent )
				csgo::i::engine->set_viewangles( ang );

			ucmd->m_tickcount = csgo::time2ticks( best_pl->simtime( ) ) + csgo::time2ticks( lerp( ) );
		}
		else if ( best_dmg > 0.0f ) {
			if ( autoscope && !g::local->scoped( ) )
				ucmd->m_buttons |= 0x80000;
			else if ( autoscope && g::local->scoped( ) )
				ucmd->m_buttons &= ~0x80000;
		}
	}
}