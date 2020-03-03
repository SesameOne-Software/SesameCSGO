#include "ragebot.hpp"
#include <deque>
#include "autowall.hpp"
#include "../menu/menu.hpp"
#include "../globals.hpp"

void features::ragebot::hitscan( player_t* pl, vec3_t& point, float& dmg ) {
	FIND( bool, head, "Rage", "Target Selection", "Head", oxui::object_checkbox );
	FIND( bool, neck, "Rage", "Target Selection", "Neck", oxui::object_checkbox );
	FIND( bool, chest, "Rage", "Target Selection", "Chest", oxui::object_checkbox );
	FIND( bool, pelvis, "Rage", "Target Selection", "Pelvis", oxui::object_checkbox );
	FIND( bool, arms, "Rage", "Target Selection", "Arms", oxui::object_checkbox );
	FIND( bool, legs, "Rage", "Target Selection", "Legs", oxui::object_checkbox );
	FIND( bool, feet, "Rage", "Target Selection", "Feet", oxui::object_checkbox );
	FIND( double, damage, "Rage", "Aimbot", "Min. Dmg", oxui::object_slider );

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
	FIND( bool, ragebot, "Rage", "Aimbot", "Ragebot", oxui::object_checkbox );
	FIND( double, hitchance, "Rage", "Aimbot", "Hit Chance", oxui::object_slider );
	FIND( double, hitchance_tolerance, "Rage", "Aimbot", "Hit Chance Tolerance", oxui::object_slider );
	FIND( bool, autoshoot, "Rage", "Aimbot", "Auto-Shoot", oxui::object_checkbox );
	FIND( bool, silent, "Rage", "Aimbot", "Silent", oxui::object_checkbox );
	FIND( bool, autoscope, "Rage", "Aimbot", "Auto-Scope", oxui::object_checkbox );

	if ( !ragebot )
		return;

	vec3_t engine_ang;
	csgo::i::engine->get_viewangles( engine_ang );

	auto can_shoot = [ & ] ( ) {
		return g::local->weapon( ) && g::local->weapon( )->next_primary_attack( ) <= csgo::ticks2time( g::local->tick_base( ) ) && g::local->weapon( )->ammo( );
	};

	auto run_hitchance = [ & ] ( vec3_t ang, player_t* pl, vec3_t point ) {
		auto weapon = g::local->weapon( );

		if ( !weapon )
			return false;

		auto src = g::local->eyes( );
		csgo::clamp( ang );
		auto forward = csgo::angle_vec( ang );
		auto right = csgo::angle_vec( ang + vec3_t( 0.0f, 90.0f, 0.0f ) );
		auto up = csgo::angle_vec( ang + vec3_t( 90.0f, 0.0f, 0.0f ) );

		auto hits = 0;
		auto needed_hits = static_cast< int >( 150.0f * ( hitchance / 100.0f ) );

		//weapon->UpdateAccuracyPenalty( );
		auto weap_spread = weapon->spread( );
		auto weap_inaccuracy = weapon->inaccuracy( );

		for ( auto i = 0; i < 150; i++ ) {
			auto a = ( rand( ) % 180 ) * ( csgo::pi / 180.0f );
			auto b = ( rand( ) % 360 ) * ( csgo::pi / 180.0f );
			auto c = ( rand( ) % 180 ) * ( csgo::pi / 180.0f );
			auto d = ( rand( ) % 360 ) * ( csgo::pi / 180.0f );

			auto inaccuracy = a * weap_inaccuracy;
			auto spread = c * weap_spread;

			if ( weapon->item_definition_index( ) == 64 ) {
				a = 1.0f - a * a;
				a = 1.0f - c * c;
			}

			vec3_t spread_v( ( std::cosf( b ) * inaccuracy ) + ( std::cosf( d ) * spread ), ( std::sinf( b ) * inaccuracy ) + ( std::sinf( d ) * spread ), 0 ), direction;

			direction.x = forward.x + ( spread_v.x * right.x ) + ( spread_v.y * up.x );
			direction.y = forward.y + ( spread_v.x * right.y ) + ( spread_v.y * up.y );
			direction.z = forward.z + ( spread_v.x * right.z ) + ( spread_v.y * up.z );
			direction.normalize( );

			auto va_spread = csgo::vec_angle( direction );
			csgo::clamp( va_spread );

			auto fwd_v = csgo::angle_vec( va_spread );
			fwd_v.normalize( );

			fwd_v = src + ( fwd_v * src.dist_to( point ) );

			trace_t tr;

			csgo::util_traceline( src, fwd_v, 0x600400B, g::local, &tr );

			const auto dst = tr.m_endpos.dist_to( point );

			// dbg_print( "%3.f\n", dst );

			if ( dst <= hitchance_tolerance )
				hits++;

			if ( ( static_cast< float >( hits ) / 150.0f ) * 100.0f >= hitchance )
				return true;

			if ( ( 150 - i + hits ) < needed_hits )
				return false;
		}

		return false;
	};

	auto lerp = [ ] ( ) {
		auto cv_lerp = 0.031000f;
		auto cv_ratio = 2.0f;
		auto cv_udrate = 1.0f / csgo::i::globals->m_ipt;
		return std::max< float >( cv_lerp, cv_ratio / cv_udrate );
	};

	player_t* best_pl = nullptr;
	auto best_ang = vec3_t( );
	auto best_point = vec3_t( );
	auto best_fov = 180.0f;
	auto best_dmg = 0.0f;

	csgo::for_each_player( [ & ] ( player_t* pl ) {
		if ( pl->team( ) == g::local->team( ) || pl->immune( ) )
			return;

		vec3_t point;
		float dmg = 0.0f;

		hitscan( pl, point, dmg );

		auto ang = csgo::calc_angle( g::local->eyes( ), point );
		csgo::clamp( ang );

		const auto fov = ang.dist_to( engine_ang );

		if ( dmg > best_dmg&& fov < best_fov ) {
			best_pl = pl;
			best_dmg = dmg;
			best_fov = fov;
			best_ang = ang;
			best_point = point;
		}
		} );

	if ( autoshoot && !can_shoot( ) ) {
		ucmd->m_buttons &= ~1;
	}
	else if ( can_shoot( ) ) {
		auto should_aim = best_dmg > 0.0f && run_hitchance( best_ang, best_pl, best_point );

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