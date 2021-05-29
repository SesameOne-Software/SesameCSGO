#include <array>
#include "resolver.hpp"
#include "../features/ragebot.hpp"
#include "../globals.hpp"
#include "../security/security_handler.hpp"
#include "../features/autowall.hpp"
#include "../menu/menu.hpp"
#include "../features/esp.hpp"
#include "../features/autowall.hpp"
#include "../hitsounds.h"
#include "../features/other_visuals.hpp"
#include "../menu/options.hpp"

#include "anims.hpp"

#include "../renderer/render.hpp"

#undef min
#undef max

template < typename ...args_t >
void print_console ( const options::option::colorf& clr, const char* fmt, args_t ...args ) {
	if ( !fmt )
		return;

	struct {
		uint8_t r, g, b, a;
	} s_clr;

	s_clr = { static_cast < uint8_t > ( clr.r * 255.0f ), static_cast < uint8_t > ( clr.g * 255.0f ), static_cast < uint8_t > ( clr.b * 255.0f ), static_cast < uint8_t > ( clr.a * 255.0f ) };

	static auto con_color_msg = reinterpret_cast< void ( * )( const decltype( s_clr )&, const char*, ... ) >( LI_FN ( GetProcAddress ) ( LI_FN ( GetModuleHandleA ) ( _ ( "tier0.dll" ) ), _ ( "?ConColorMsg@@YAXABVColor@@PBDZZ" ) ) );

	con_color_msg ( s_clr, fmt, args... );
}

struct impact_rec_t {
	vec3_t m_src, m_dst;
	std::string m_msg;
	float m_time;
	bool m_hurt;
	uint32_t m_clr;
	bool m_beam_created;
};

std::deque< anims::resolver::hit_matrix_rec_t > hit_matrix_rec { };
std::deque< impact_rec_t > impact_recs { };

void anims::resolver::process_impact_clientside ( event_t* event ) {
	static auto& bullet_impacts_client = options::vars [ _ ( "visuals.other.bullet_impacts_client" ) ].val.b;
	static auto& bullet_impacts_client_color = options::vars [ _ ( "visuals.other.bullet_impacts_client_color" ) ].val.c;

	auto shooter = cs::i::ent_list->get< player_t* > ( cs::i::engine->get_player_for_userid ( event->get_int ( _ ( "userid" ) ) ) );
	auto& target = features::ragebot::get_target ( );

	if ( !g::local || !g::local->alive ( ) || !shooter || shooter != g::local )
		return;

	//const auto impact_pos = vec3_t ( event->get_float ( _ ( "x" ) ), event->get_float ( _ ( "y" ) ), event->get_float ( _ ( "z" ) ) );
	//
	//if ( bullet_impacts_client )
	//	cs::add_box_overlay ( impact_pos, vec3_t ( -2.0f, -2.0f, -2.0f ), vec3_t ( 2.0f, 2.0f, 2.0f ), cs::calc_angle ( target ? features::ragebot::get_shot_pos ( target->idx ( ) ) : g::local->eyes ( ), impact_pos ), bullet_impacts_client_color.r * 255.0f, bullet_impacts_client_color.g * 255.0f, bullet_impacts_client_color.b * 255.0f, bullet_impacts_client_color.a * 255.0f, 7.0f );

	rdata::clientside_shot = true;
}

void anims::resolver::process_impact ( event_t* event ) {
	static auto& bullet_impacts_server = options::vars [ _ ( "visuals.other.bullet_impacts_server" ) ].val.b;
	static auto& bullet_impacts_server_color = options::vars [ _ ( "visuals.other.bullet_impacts_server_color" ) ].val.c;

	auto shooter = cs::i::ent_list->get< player_t* > ( cs::i::engine->get_player_for_userid ( event->get_int ( _ ( "userid" ) ) ) );
	auto& target = features::ragebot::get_target ( );

	if ( !g::local || !g::local->alive ( ) || !shooter || shooter != g::local )
		return;

	const auto impact_pos = vec3_t ( event->get_float ( _ ( "x" ) ), event->get_float ( _ ( "y" ) ), event->get_float ( _ ( "z" ) ) );

	if ( bullet_impacts_server )
		cs::add_box_overlay ( impact_pos, vec3_t ( -2.0f, -2.0f, -2.0f ), vec3_t ( 2.0f, 2.0f, 2.0f ), cs::calc_angle ( target ? features::ragebot::get_shot_pos ( target->idx ( ) ) : g::local->eyes ( ), impact_pos ), bullet_impacts_server_color.r * 255.0f, bullet_impacts_server_color.g * 255.0f, bullet_impacts_server_color.b * 255.0f, bullet_impacts_server_color.a * 255.0f, 7.0f );
	
	rdata::clientside_shot = false;

	if ( target ) {
		const auto shot_pos = features::ragebot::get_shot_pos ( target->idx ( ) );

		if ( !target || target->dormant ( ) || target->idx ( ) <= 0 || target->idx ( ) > 64 || shot_pos == vec3_t ( 0.0f, 0.0f, 0.0f ) || !g::local->weapon ( ) || !g::local->weapon ( )->data ( ) )
			return;

		auto target_rec = features::ragebot::get_lag_rec ( target->idx ( ) );
		const auto towards = cs::angle_vec ( cs::calc_angle ( shot_pos, impact_pos ) );
		const auto vec_max = shot_pos + towards * g::local->weapon ( )->data ( )->m_range;

		const auto backup_origin = target->origin ( );
		auto backup_abs_origin = target->abs_origin ( );
		const auto backup_bone_cache = target->bone_cache ( );
		const auto backup_mins = target->mins ( );
		const auto backup_maxs = target->maxs ( );

		target->origin ( ) = target_rec.m_origin;
		//target->set_abs_origin ( target_rec.m_origin );
		target->bone_cache ( ) = target_rec.m_aim_bones[ target_rec.m_side ].data();
		target->mins ( ) = target_rec.m_mins;
		target->maxs ( ) = target_rec.m_maxs;

		auto hit_hitgroup = -1;
		vec3_t impact_out = impact_pos;

		const auto dmg = autowall::dmg ( g::local, target, shot_pos, vec_max, -1, &impact_out );

		target->origin ( ) = backup_origin;
		//target->set_abs_origin ( backup_abs_origin );
		target->bone_cache ( ) = backup_bone_cache;
		target->mins ( ) = backup_mins;
		target->maxs ( ) = backup_maxs;

		rdata::impact_dmg [ target->idx ( ) ] = dmg;
		rdata::impacts [ target->idx ( ) ] = impact_pos;

		return;
	}

	rdata::non_player_impacts = impact_pos;
}

/* missed shot detection stuff */
std::array< anims::desync_side_t , 65 > last_recorded_resolve { anims::desync_side_t::desync_middle };
std::array< int, 65 > hit_hitgroup { 0 };

void anims::resolver::process_hurt ( event_t* event ) {
	static auto& hit_sound = options::vars [ _ ( "visuals.other.hit_sound" ) ].val.i;

	auto dmg = event->get_int ( _ ( "dmg_health" ) );
	auto attacker = cs::i::ent_list->get< player_t* > ( cs::i::engine->get_player_for_userid ( event->get_int ( _ ( "attacker" ) ) ) );
	auto victim = cs::i::ent_list->get< player_t* > ( cs::i::engine->get_player_for_userid ( event->get_int ( _ ( "userid" ) ) ) );
	auto hitgroup = event->get_int ( _ ( "hitgroup" ) );

	if ( !attacker || !victim || attacker != g::local || victim->team ( ) == g::local->team ( ) || victim->idx ( ) <= 0 || victim->idx ( ) > 64 )
		return;

	switch ( hit_sound ) {
	case 0: break;
	case 1: cs::i::engine->client_cmd_unrestricted ( _ ( "play buttons\\arena_switch_press_02" ) ); break;
	case 2: cs::i::engine->client_cmd_unrestricted ( _ ( "play player\\pl_fallpain3" ) ); break;
	case 3: cs::i::engine->client_cmd_unrestricted ( _ ( "play weapons\\awp\\awp_boltback" ) ); break;
	case 4: cs::i::engine->client_cmd_unrestricted ( _ ( "play weapons\\flashbang\\grenade_hit1" ) ); break;
	case 5: LI_FN ( PlaySoundA ) ( reinterpret_cast < const char* > ( hitsound_sesame ), nullptr, SND_MEMORY | SND_ASYNC ); break;
	default: break;
	}

	rdata::player_dmg [ victim->idx ( ) ] = dmg;
	rdata::player_hurt [ victim->idx ( ) ] = true;
	features::ragebot::get_hits ( victim->idx ( ) )++;

	hit_hitgroup [ victim->idx ( ) ] = hitgroup;

	/* hit wrong hitbox, probably due to resolver */
	if ( hitgroup != autowall::hitbox_to_hitgroup ( features::ragebot::get_hitbox ( victim->idx ( ) ) ) )
		rdata::wrong_hitbox [ victim->idx ( ) ] = true;
}

void anims::resolver::process_event_buffer ( int pl_idx ) {
	static auto& logs = options::vars [ _ ( "visuals.other.logs" ) ].val.l;

	if ( !g::local ) {
			return;
	}

	auto ray_intersects_sphere = [ ] ( const vec3_t& from, const vec3_t& to, const vec3_t& sphere, float rad ) -> bool {
		auto q = sphere - from;
		auto v = q.dot_product ( cs::angle_vec ( cs::calc_angle ( from, to ) ).normalized ( ) );
		auto d = rad - ( q.length_sqr ( ) - v * v );

		return d >= FLT_EPSILON;
	};

	if ( rdata::non_player_impacts != vec3_t ( 0.0f, 0.0f, 0.0f ) ) {
		impact_recs.push_back ( impact_rec_t { g::local->eyes ( ), rdata::non_player_impacts, _ ( "" ), cs::i::globals->m_curtime, false, rgba ( 161, 66, 245, 150 ) } );
		rdata::non_player_impacts = vec3_t ( 0.0f, 0.0f, 0.0f );
	}

	if ( rdata::last_shots [ pl_idx ] != features::ragebot::get_shots ( pl_idx ) && rdata::impacts [ pl_idx ] != vec3_t ( 0.0f, 0.0f, 0.0f ) && !rdata::player_hurt [ pl_idx ] ) {
		if ( rdata::impact_dmg [ pl_idx ] <= 0.0f && !g::cvars::weapon_accuracy_nospread->get_bool ( ) ) {
			features::ragebot::get_misses ( pl_idx ).spread++;
			impact_recs.push_back ( impact_rec_t { features::ragebot::get_shot_pos ( pl_idx ), rdata::impacts [ pl_idx ], _ ( "spread ( " ) + std::to_string ( features::ragebot::get_misses ( pl_idx ).spread ) + _ ( " )" ), cs::i::globals->m_curtime, false, rgba ( 161, 66, 245, 150 ) } );

			if ( logs [ 1 ] ) {
				print_console ( { 0.85f, 0.31f, 0.83f, 1.0f }, _ ( "sesame" ) );
				print_console ( { 1.0f, 1.0f, 1.0f, 1.0f }, _ ( " ～ missed shot due to spread\n" ) );
			}
		}
		else {
			features::ragebot::get_misses ( pl_idx ).bad_resolve++;
			impact_recs.push_back ( impact_rec_t { features::ragebot::get_shot_pos ( pl_idx ), rdata::impacts [ pl_idx ], _ ( "resolve ( " ) + std::to_string ( features::ragebot::get_misses ( pl_idx ).bad_resolve ) + _ ( " )" ), cs::i::globals->m_curtime, false, rgba ( 161, 66, 245, 150 ) } );

			/* missed the cached resolve? don't shoot at it again until we have it fixed */
			//fix_cached_resolves( pl_idx );

			if ( logs [ 2 ] ) {
				auto& rec = features::ragebot::get_lag_rec ( pl_idx );
				print_console ( { 0.85f, 0.31f, 0.83f, 1.0f }, _ ( "sesame" ) );
				print_console ( { 1.0f, 1.0f, 1.0f, 1.0f }, _ ( " ～ missed shot due to bad resolve at %.1f°" ), cs::normalize( cs::normalize( rec.m_abs_angles[rec.m_side].y )- cs::normalize( rec.m_angles.y) ) );

				if ( rec.m_shot ) {
					print_console ( { 1.0f, 1.0f, 1.0f, 1.0f }, _ ( ", " ) );
					print_console ( { 0.678f, 1.0f, 0.996f, 1.0f }, _ ( "onshot" ) );
				}

				//if ( rec.m_predicted ) {
				//	print_console ( { 1.0f, 1.0f, 1.0f, 1.0f }, _ ( ", " ) );
				//	print_console ( { 0.678f, 1.0f, 0.682f, 1.0f }, _ ( "extrapolated" ) );
				//}

				print_console ( { 1.0f, 1.0f, 1.0f, 1.0f }, _ ( "\n" ) );
			}
		}

		rdata::impacts [ pl_idx ] = vec3_t ( 0.0f, 0.0f, 0.0f );
		rdata::player_hurt [ pl_idx ] = false;
		rdata::last_shots [ pl_idx ] = features::ragebot::get_shots ( pl_idx );
		features::ragebot::get_shot_pos ( pl_idx ) = vec3_t ( 0.0f, 0.0f, 0.0f );
		rdata::player_dmg [ pl_idx ] = 0;
		rdata::impact_dmg [ pl_idx ] = 0.0f;
		features::ragebot::get_target_idx ( ) = 0;
		features::ragebot::get_target ( ) = nullptr;
		rdata::wrong_hitbox [ pl_idx ] = false;
	}
	else if ( rdata::wrong_hitbox [ pl_idx ] ) {
		hit_matrix_rec.push_back ( { uint32_t ( pl_idx ), { }, cs::i::globals->m_curtime, rgba ( 202, 252, 3, 255 ) } );

		memcpy ( hit_matrix_rec.back ( ).m_bones.data ( ), features::ragebot::get_lag_rec ( pl_idx ).m_aim_bones.data(), sizeof matrix3x4_t * 128 );

		impact_recs.push_back ( impact_rec_t { features::ragebot::get_shot_pos ( pl_idx ), features::ragebot::get_target_pos ( pl_idx ), std::to_string ( rdata::player_dmg [ pl_idx ] ), cs::i::globals->m_curtime, true, rgba ( 161, 66, 245, 150 ) } );

		player_info_t pl_info;
		cs::i::engine->get_player_info ( pl_idx, &pl_info );

		if ( logs [ 3 ] ) {
			print_console ( { 0.85f, 0.31f, 0.83f, 1.0f }, _ ( "sesame" ) );
			print_console ( { 1.0f, 1.0f, 1.0f, 1.0f }, _ ( " ～ hit %s in the " ), pl_info.m_name );

			switch ( hit_hitgroup [ pl_idx ] ) {
			case 0: print_console ( { 0.46f, 1.0f, 0.19f, 1.0f }, _ ( "generic" ) ); break;
			case 1: print_console ( { 0.46f, 1.0f, 0.19f, 1.0f }, _ ( "head" ) ); break;
			case 2: print_console ( { 0.46f, 1.0f, 0.19f, 1.0f }, _ ( "chest" ) ); break;
			case 3: print_console ( { 0.46f, 1.0f, 0.19f, 1.0f }, _ ( "stomach" ) ); break;
			case 4: print_console ( { 0.46f, 1.0f, 0.19f, 1.0f }, _ ( "left arm" ) ); break;
			case 5: print_console ( { 0.46f, 1.0f, 0.19f, 1.0f }, _ ( "right arm" ) ); break;
			case 6: print_console ( { 0.46f, 1.0f, 0.19f, 1.0f }, _ ( "left leg" ) ); break;
			case 7: print_console ( { 0.46f, 1.0f, 0.19f, 1.0f }, _ ( "right leg" ) ); break;
			case 8: print_console ( { 0.46f, 1.0f, 0.19f, 1.0f }, _ ( "neck" ) ); break;
			case 10: print_console ( { 0.46f, 1.0f, 0.19f, 1.0f }, _ ( "gear" ) ); break;
			default: print_console ( { 0.46f, 1.0f, 0.19f, 1.0f }, _ ( "unknown" ) ); break;
			}

			print_console ( { 1.0f, 1.0f, 1.0f, 1.0f }, _ ( " for " ) );
			print_console ( { 1.0f, 0.34f, 0.34f, 1.0f }, _ ( "%d" ), rdata::player_dmg [ pl_idx ] );
			print_console ( { 1.0f, 1.0f, 1.0f, 1.0f }, _ ( "; wrong hitbox ( target " ) );

			switch ( autowall::hitbox_to_hitgroup ( features::ragebot::get_hitbox ( pl_idx ) ) ) {
			case 0: print_console ( { 0.46f, 1.0f, 0.19f, 1.0f }, _ ( "generic" ) ); break;
			case 1: print_console ( { 0.46f, 1.0f, 0.19f, 1.0f }, _ ( "head" ) ); break;
			case 2: print_console ( { 0.46f, 1.0f, 0.19f, 1.0f }, _ ( "chest" ) ); break;
			case 3: print_console ( { 0.46f, 1.0f, 0.19f, 1.0f }, _ ( "stomach" ) ); break;
			case 4: print_console ( { 0.46f, 1.0f, 0.19f, 1.0f }, _ ( "left arm" ) ); break;
			case 5: print_console ( { 0.46f, 1.0f, 0.19f, 1.0f }, _ ( "right arm" ) ); break;
			case 6: print_console ( { 0.46f, 1.0f, 0.19f, 1.0f }, _ ( "left leg" ) ); break;
			case 7: print_console ( { 0.46f, 1.0f, 0.19f, 1.0f }, _ ( "right leg" ) ); break;
			case 8: print_console ( { 0.46f, 1.0f, 0.19f, 1.0f }, _ ( "neck" ) ); break;
			case 10: print_console ( { 0.46f, 1.0f, 0.19f, 1.0f }, _ ( "gear" ) ); break;
			default: print_console ( { 0.46f, 1.0f, 0.19f, 1.0f }, _ ( "unknown" ) ); break;
			}

			print_console ( { 1.0f, 1.0f, 1.0f, 1.0f }, _ ( " )" ) );

			auto& rec = features::ragebot::get_lag_rec ( pl_idx );
			if ( rec.m_shot ) {
				print_console ( { 1.0f, 1.0f, 1.0f, 1.0f }, _ ( ", " ) );
				print_console ( { 0.678f, 1.0f, 0.996f, 1.0f }, _ ( "onshot" ) );
			}

			//if ( rec.m_predicted ) {
			//	print_console ( { 1.0f, 1.0f, 1.0f, 1.0f }, _ ( ", " ) );
			//	print_console ( { 0.678f, 1.0f, 0.682f, 1.0f }, _ ( "extrapolated" ) );
			//}

			print_console ( { 1.0f, 1.0f, 1.0f, 1.0f }, _ ( "\n" ) );
		}

		rdata::impacts [ pl_idx ] = vec3_t ( 0.0f, 0.0f, 0.0f );
		rdata::queued_hit [ pl_idx ] = true;
		rdata::player_hurt [ pl_idx ] = false;
		rdata::last_shots [ pl_idx ] = features::ragebot::get_shots ( pl_idx );
		features::ragebot::get_shot_pos ( pl_idx ) = vec3_t ( 0.0f, 0.0f, 0.0f );
		rdata::player_dmg [ pl_idx ] = 0;
		rdata::impact_dmg [ pl_idx ] = 0.0f;
		features::ragebot::get_target_idx ( ) = 0;
		features::ragebot::get_target ( ) = nullptr;
		rdata::wrong_hitbox [ pl_idx ] = false;
	}
	else if ( rdata::player_hurt [ pl_idx ] ) {
		hit_matrix_rec.push_back ( { uint32_t ( pl_idx ), { }, cs::i::globals->m_curtime, rgba ( 202, 252, 3, 255 ) } );

		memcpy ( hit_matrix_rec.back ( ).m_bones.data ( ), features::ragebot::get_lag_rec ( pl_idx ).m_aim_bones.data ( ), sizeof matrix3x4_t * 128 );

		impact_recs.push_back ( impact_rec_t { features::ragebot::get_shot_pos ( pl_idx ), features::ragebot::get_target_pos ( pl_idx ), std::to_string ( rdata::player_dmg [ pl_idx ] ), cs::i::globals->m_curtime, true, rgba ( 161, 66, 245, 150 ) } );

		player_info_t pl_info;
		cs::i::engine->get_player_info ( pl_idx, &pl_info );

		if ( logs [ 0 ] ) {
			print_console ( { 0.85f, 0.31f, 0.83f, 1.0f }, _ ( "sesame" ) );
			print_console ( { 1.0f, 1.0f, 1.0f, 1.0f }, _ ( " ～ hit %s in the " ), pl_info.m_name );

			switch ( hit_hitgroup [ pl_idx ] ) {
			case 0: print_console ( { 0.46f, 1.0f, 0.19f, 1.0f }, _ ( "generic" ) ); break;
			case 1: print_console ( { 0.46f, 1.0f, 0.19f, 1.0f }, _ ( "head" ) ); break;
			case 2: print_console ( { 0.46f, 1.0f, 0.19f, 1.0f }, _ ( "chest" ) ); break;
			case 3: print_console ( { 0.46f, 1.0f, 0.19f, 1.0f }, _ ( "stomach" ) ); break;
			case 4: print_console ( { 0.46f, 1.0f, 0.19f, 1.0f }, _ ( "left arm" ) ); break;
			case 5: print_console ( { 0.46f, 1.0f, 0.19f, 1.0f }, _ ( "right arm" ) ); break;
			case 6: print_console ( { 0.46f, 1.0f, 0.19f, 1.0f }, _ ( "left leg" ) ); break;
			case 7: print_console ( { 0.46f, 1.0f, 0.19f, 1.0f }, _ ( "right leg" ) ); break;
			case 8: print_console ( { 0.46f, 1.0f, 0.19f, 1.0f }, _ ( "neck" ) ); break;
			case 10: print_console ( { 0.46f, 1.0f, 0.19f, 1.0f }, _ ( "gear" ) ); break;
			default: print_console ( { 0.46f, 1.0f, 0.19f, 1.0f }, _ ( "unknown" ) ); break;
			}

			print_console ( { 1.0f, 1.0f, 1.0f, 1.0f }, _ ( " for " ) );
			print_console ( { 1.0f, 0.34f, 0.34f, 1.0f }, _ ( "%d" ), rdata::player_dmg [ pl_idx ] );

			auto& rec = features::ragebot::get_lag_rec ( pl_idx );
			if ( rec.m_shot ) {
				print_console ( { 1.0f, 1.0f, 1.0f, 1.0f }, _ ( ", " ) );
				print_console ( { 0.678f, 1.0f, 0.996f, 1.0f }, _ ( "onshot" ) );
			}

			//if ( rec.m_predicted ) {
			//	print_console ( { 1.0f, 1.0f, 1.0f, 1.0f }, _ ( ", " ) );
			//	print_console ( { 0.678f, 1.0f, 0.682f, 1.0f }, _ ( "extrapolated" ) );
			//}

			print_console ( { 1.0f, 1.0f, 1.0f, 1.0f }, _ ( "\n" ) );
		}

		rdata::impacts [ pl_idx ] = vec3_t ( 0.0f, 0.0f, 0.0f );
		rdata::queued_hit [ pl_idx ] = true;
		rdata::player_hurt [ pl_idx ] = false;
		rdata::last_shots [ pl_idx ] = features::ragebot::get_shots ( pl_idx );
		features::ragebot::get_shot_pos ( pl_idx ) = vec3_t ( 0.0f, 0.0f, 0.0f );
		rdata::player_dmg [ pl_idx ] = 0;
		rdata::impact_dmg [ pl_idx ] = 0.0f;
		features::ragebot::get_target_idx ( ) = 0;
		features::ragebot::get_target ( ) = nullptr;
		rdata::wrong_hitbox [ pl_idx ] = false;
	}
}

void anims::resolver::create_beams ( ) {
	static auto& bullet_tracers = options::vars [ _ ( "visuals.other.bullet_tracers" ) ].val.b;
	static auto& bullet_tracer_color = options::vars [ _ ( "visuals.other.bullet_tracer_color" ) ].val.c;

	/* erase resolver data if we exit the game or connect to another server */
	if ( !cs::i::engine->is_connected ( ) || !cs::i::engine->is_in_game ( ) ) {
		for ( auto i = 0; i < 65; i++ ) {
			if ( !cs::i::ent_list->get< player_t* > ( i ) || !cs::i::ent_list->get< player_t* > ( i )->alive ( ) )
				features::ragebot::get_misses ( i ).bad_resolve = 0;
		}
	}

	if ( !g::local || !g::local->alive ( ) ) {
		if ( !impact_recs.empty ( ) )
			impact_recs.clear ( );

		if ( !hit_matrix_rec.empty ( ) )
			hit_matrix_rec.clear ( );

		return;
	}

	auto calc_alpha = [ & ] ( float time, float fade_time, float base_alpha, bool add = false ) {
		const auto dormant_time = 10.0f;
		return static_cast< int >( ( std::clamp< float > ( dormant_time - ( std::clamp< float > ( add ? ( dormant_time - std::clamp< float > ( std::fabsf ( cs::i::globals->m_curtime - time ), 0.0f, dormant_time ) ) : std::fabsf ( cs::i::globals->m_curtime - time ), std::max< float > ( dormant_time - fade_time, 0.0f ), dormant_time ) ), 0.0f, fade_time ) / fade_time ) * base_alpha );
	};

	int cur_ray = 0;

	for ( auto& impact : impact_recs ) {
		vec3_t scrn_src;
		vec3_t scrn_dst;

		const auto alpha2 = calc_alpha ( impact.m_time, 2.0f, 255 );

		if ( !alpha2 ) {
			impact_recs.erase ( impact_recs.begin ( ) + cur_ray );
			continue;
		}

		if ( bullet_tracers && !impact.m_beam_created ) {
			beam_info_t beam_info;

			beam_info.m_type = 0;
			beam_info.m_model_name = _ ( "sprites/physbeam.vmt" );
			beam_info.m_model_idx = -1;
			beam_info.m_halo_scale = 0.0f;
			beam_info.m_start = impact.m_dst;
			beam_info.m_end = impact.m_src - vec3_t ( 0.0f, 0.0f, 2.0f );
			beam_info.m_life = 7.0f;
			beam_info.m_fade_len = 2.0f;
			beam_info.m_amplitude = 0.0f;
			beam_info.m_segments = 2;
			beam_info.m_renderable = true;
			beam_info.m_speed = 0.0f;
			beam_info.m_start_frame = 0;
			beam_info.m_frame_rate = 0.0f;
			beam_info.m_width = 1.5f;
			beam_info.m_end_width = 1.5f;
			beam_info.m_flags = 0;

			beam_info.m_red = static_cast< float > ( bullet_tracer_color.r * 255.0f );
			beam_info.m_green = static_cast< float > ( bullet_tracer_color.g * 255.0f );
			beam_info.m_blue = static_cast< float > ( bullet_tracer_color.b * 255.0f );
			beam_info.m_brightness = static_cast< float > ( bullet_tracer_color.a * 255.0f );

			auto beam = cs::i::beams->create_beam_points ( beam_info );

			if ( beam )
				cs::i::beams->draw_beam ( beam );

			impact.m_beam_created = true;
		}

		cur_ray++;
	}
}

void anims::resolver::render_impacts ( ) {
	static auto& bullet_tracers = options::vars [ _ ( "visuals.other.bullet_tracers" ) ].val.b;
	static auto& bullet_tracer_color = options::vars [ _ ( "visuals.other.bullet_tracer_color" ) ].val.c;
	static auto& damage_indicator = options::vars [ _ ( "visuals.other.damage_indicator" ) ].val.b;
	static auto& damage_indicator_color = options::vars [ _ ( "visuals.other.damage_indicator_color" ) ].val.c;
	static auto& player_hits = options::vars [ _ ( "visuals.other.player_hits" ) ].val.b;
	static auto& player_hits_color = options::vars [ _ ( "visuals.other.player_hits_color" ) ].val.c;

	/* erase resolver data if we exit the game or connect to another server */
	if ( !cs::i::engine->is_connected ( ) || !cs::i::engine->is_in_game ( ) ) {
		for ( auto i = 0; i < 65; i++ ) {
			if ( !cs::i::ent_list->get< player_t* > ( i ) || !cs::i::ent_list->get< player_t* > ( i )->alive ( ) )
				features::ragebot::get_misses ( i ).bad_resolve = 0;
		}
	}

	if ( !g::local || !g::local->alive ( ) ) {
		if ( !impact_recs.empty ( ) )
			impact_recs.clear ( );

		if ( !hit_matrix_rec.empty ( ) )
			hit_matrix_rec.clear ( );

		return;
	}

	auto calc_alpha = [ & ] ( float time, float fade_time, float base_alpha, bool add = false ) {
		const auto dormant_time = 4.0f;
		return static_cast< int >( ( std::clamp< float > ( dormant_time - ( std::clamp< float > ( add ? ( dormant_time - std::clamp< float > ( abs ( ( g::local ? cs::ticks2time ( g::local->tick_base ( ) ) : cs::i::globals->m_curtime ) - time ), 0.0f, dormant_time ) ) : abs ( cs::i::globals->m_curtime - time ), std::max< float > ( dormant_time - fade_time, 0.0f ), dormant_time ) ), 0.0f, fade_time ) / fade_time ) * base_alpha );
	};

	if ( !hit_matrix_rec.empty ( ) ) {
		int cur_matrix = 0;

		for ( auto& hit_matrix : hit_matrix_rec ) {
			const auto pl = cs::i::ent_list->get< player_t* > ( hit_matrix.m_pl );

			if ( !pl || !pl->alive ( ) ) {
				hit_matrix_rec.erase ( hit_matrix_rec.begin ( ) + cur_matrix );
				continue;
			}

			features::visual_config_t visuals;

			if ( !features::get_visuals ( pl, visuals ) )
				continue;

			const auto alpha = calc_alpha ( hit_matrix.m_time, 2.0f, visuals.hit_matrix_color.a * 255.0f );

			if ( !alpha ) {
				hit_matrix_rec.erase ( hit_matrix_rec.begin ( ) + cur_matrix );
				continue;
			}

			hit_matrix.m_clr = rgba (
				static_cast< int >( visuals.hit_matrix_color.r * 255.0f ),
				static_cast< int >( visuals.hit_matrix_color.g * 255.0f ),
				static_cast< int >( visuals.hit_matrix_color.b * 255.0f ),
				alpha
			);

			cur_matrix++;
		}
	}

	if ( impact_recs.empty ( ) )
		return;

	int cur_ray = 0;

	for ( auto& impact : impact_recs ) {
		vec3_t scrn_src;
		vec3_t scrn_dst;

		const auto alpha2 = calc_alpha ( impact.m_time, 2.0f, 255 );

		if ( !alpha2 ) {
			impact_recs.erase ( impact_recs.begin ( ) + cur_ray );
			continue;
		}

		cs::render::world_to_screen ( scrn_src, impact.m_src );
		cs::render::world_to_screen ( scrn_dst, impact.m_dst );

		impact.m_clr = rgba <int>( bullet_tracer_color.r * 255.0f, bullet_tracer_color.g * 255.0f, bullet_tracer_color.b * 255.0f, calc_alpha ( impact.m_time, 2.0f, bullet_tracer_color.a * 255.0f ) );

		if ( impact.m_hurt ) {
			if ( damage_indicator ) {
				vec3_t dim;
				render::text_size ( impact.m_msg, _ ( "watermark_font" ), dim );
				render::text ( scrn_dst.x - dim.x / 2, scrn_dst.y - 26.0f - ( std::clamp ( ( g::local ? cs::ticks2time ( g::local->tick_base ( ) ) : cs::i::globals->m_curtime ) - impact.m_time, 0.0f, 6.0f ) / 6.0f ) * 100.0f, impact.m_msg, ( "watermark_font" ), rgba<int> ( damage_indicator_color.r * 255.0f, damage_indicator_color.g * 255.0f, damage_indicator_color.b * 255.0f, calc_alpha ( impact.m_time, 2.0f, damage_indicator_color.a * 255.0f ) ), true );
			}
			
			/* hit dot */
			if ( player_hits){
				const auto radius = ( std::clamp ( ( g::local ? cs::ticks2time ( g::local->tick_base ( ) ) : cs::i::globals->m_curtime ) - impact.m_time, 0.0f, 6.0f ) / 6.0f ) * 50.0f;
				const auto x = scrn_dst.x;
				const auto y = scrn_dst.y;
				const auto verticies = 16;
				
				struct vtx_t {
					float x, y, z, rhw;
					std::uint32_t color;
				};

				std::vector< vtx_t > circle ( verticies + 2 );

				const auto angle = 0.0f;

				circle [ 0 ].x = static_cast< float > ( x ) - 0.5f;
				circle [ 0 ].y = static_cast< float > ( y ) - 0.5f;
				circle [ 0 ].z = 0.0f;
				circle [ 0 ].rhw = 1.0f;
				circle [ 0 ].color = D3DCOLOR_RGBA ( static_cast< int >( player_hits_color.r * 255.0f ), static_cast< int >( player_hits_color.g * 255.0f ), static_cast< int >( player_hits_color.b * 255.0f ), static_cast< int >( ( 1.0f - std::clamp ( ( g::local ? cs::ticks2time ( g::local->tick_base ( ) ) : cs::i::globals->m_curtime ) - impact.m_time, 0.0f, 6.0f ) / 6.0f ) * ( bullet_tracer_color.a * 255.0f )) );

				for ( auto i = 1; i < verticies + 2; i++ ) {
					circle [ i ].x = ( float ) ( x - radius * cos ( std::numbers::pi * ( ( i - 1 ) / ( static_cast< float >( verticies ) / 2.0f ) ) ) ) - 0.5f;
					circle [ i ].y = ( float ) ( y - radius * sin ( std::numbers::pi * ( ( i - 1 ) / ( static_cast< float >( verticies ) / 2.0f ) ) ) ) - 0.5f;
					circle [ i ].z = 0.0f;
					circle [ i ].rhw = 1.0f;
					circle [ i ].color = D3DCOLOR_RGBA ( static_cast< int >( player_hits_color.r * 255.0f ), static_cast< int >( player_hits_color.g * 255.0f ), static_cast< int >( player_hits_color.b * 255.0f ), 0 );
				}

				for ( auto i = 0; i < verticies + 2; i++ ) {
					circle [ i ].x = x + cos ( angle ) * ( circle [ i ].x - x ) - sin ( angle ) * ( circle [ i ].y - y ) - 0.5f;
					circle [ i ].y = y + sin ( angle ) * ( circle [ i ].x - x ) + cos ( angle ) * ( circle [ i ].y - y ) - 0.5f;
				}

				IDirect3DVertexBuffer9* vb = nullptr;

				cs::i::dev->CreateVertexBuffer ( ( verticies + 2 ) * sizeof ( vtx_t ), D3DUSAGE_WRITEONLY, D3DFVF_XYZRHW | D3DFVF_DIFFUSE, D3DPOOL_DEFAULT, &vb, nullptr );

				void* verticies_ptr;
				vb->Lock ( 0, ( verticies + 2 ) * sizeof ( vtx_t ), ( void** ) &verticies_ptr, 0 );
				std::memcpy ( verticies_ptr, &circle [ 0 ], ( verticies + 2 ) * sizeof ( vtx_t ) );
				vb->Unlock ( );

				cs::i::dev->SetStreamSource ( 0, vb, 0, sizeof ( vtx_t ) );
				cs::i::dev->DrawPrimitive ( D3DPT_TRIANGLEFAN, 0, verticies );

				if ( vb )
					vb->Release ( );
			}
		}

		cur_ray++;
	}
}

std::array < float , 65 > pitch_timer { 0.0f };
std::array < float, 65 > last_abs_yaw { 0.0f };

float angle_diff1 ( float src, float dst ) {
	auto delta = fmodf ( dst - src, 360.0f );

	if ( dst > src ) {
		if ( delta >= 180.0f )
			delta -= 360.0f;
	}
	else {
		if ( delta <= -180.0f )
			delta += 360.0f;
	}

	return delta;
}

bool anims::resolver::resolve_desync( player_t* ent , anim_info_t& rec ) {
	/* set default desync direction to middle (0 deg range) */
	const auto idx = ent->idx ( );

	rec.m_side = rdata::resolved_side [ idx ];

	static auto& resolver = options::vars [ _ ( "ragebot.resolve_desync" ) ].val.b;

	security_handler::update ( );

	const auto anim_layers = ent->layers ( );

	if ( !resolver || !g::local || !g::local->alive ( ) || !anim_layers || ent->team ( ) == g::local->team ( ) || anim_info [ idx ].size ( ) <= 1 ) {
		last_recorded_resolve [ idx ] = rdata::resolved_side [ idx ];
		return false;
	}

	memcpy ( &rdata::latest_layers [ idx ], anim_layers, sizeof( rdata::latest_layers [ idx ] ) );

	player_info_t pl_info;
	cs::i::engine->get_player_info ( idx, &pl_info );

	/* bot check */
	if ( pl_info.m_fake_player ) {
		last_recorded_resolve [ idx ] = rdata::resolved_side [ idx ];
		return false;
	}

	/* start of resolver */
	const auto speed_2d = rec.m_vel.length_2d ( );
	const auto delta_yaw = angle_diff1 ( rec.m_anim_state [ anims::desync_side_t::desync_max ].m_eye_yaw, rec.m_anim_state [ anims::desync_side_t::desync_max ].m_abs_yaw );
	const auto delta_yaw_old = angle_diff1 ( anim_info [ idx ][ 0 ].m_anim_state [ anims::desync_side_t::desync_max ].m_eye_yaw, anim_info [ idx ][ 0 ].m_anim_state [ anims::desync_side_t::desync_max ].m_abs_yaw );
	const auto delta_yaw_total = angle_diff1 ( rec.m_anim_state [ anims::desync_side_t::desync_max ].m_abs_yaw, anim_info [ idx ][ 0 ].m_anim_state [ anims::desync_side_t::desync_max ].m_abs_yaw );
	const auto delta_eye_yaw = angle_diff1 ( rec.m_anim_state [ anims::desync_side_t::desync_max ].m_eye_yaw, anim_info [ idx ][ 0 ].m_anim_state [ anims::desync_side_t::desync_max ].m_eye_yaw );

	/* turning around */
	/*if ( abs( delta_yaw_total ) > 0.1f ) {
		rdata::resolved_side [ idx ] = ( abs ( delta_yaw_total ) > 0.1f ) ? desync_side_t::desync_right_max : desync_side_t::desync_left_max;
		rdata::new_resolve [ idx ] = true;
		rdata::prefer_edge [ idx ] = false;

		if ( speed_2d > 0.1f )
			rdata::was_moving [ idx ] = true;
	}*/
	/* jitter resolver */
	/*else*/ if ( delta_yaw > 0.1f && delta_yaw_old > 0.1f && abs( delta_yaw - delta_yaw_old ) > abs( delta_eye_yaw ) ) {
		float some_yaw = 0.0f;

		if ( delta_yaw - delta_yaw_old <= 0.0f )
			some_yaw = rec.m_anim_state [ anims::desync_side_t::desync_max ].m_eye_yaw - 90.0f;
		else
			some_yaw = rec.m_anim_state [ anims::desync_side_t::desync_max ].m_eye_yaw + 90.0f;
		
		some_yaw = cs::normalize ( some_yaw );

		float v41 = angle_diff1 ( some_yaw, rec.m_anim_state [ anims::desync_side_t::desync_max ].m_abs_yaw );
		float v42 = angle_diff1 ( some_yaw, anim_info [ idx ][ 0 ].m_anim_state [ anims::desync_side_t::desync_max ].m_abs_yaw );
		
		rdata::resolved_side [ idx ] = (abs ( v41 ) <= abs ( v42 )) ? desync_side_t::desync_right_max : desync_side_t::desync_middle;
		rdata::new_resolve [ idx ] = true;
		rdata::prefer_edge [ idx ] = false;

		if ( speed_2d > 0.1f )
			rdata::was_moving [ idx ] = true;
	}
	/* on ground */
	else if ( !!(rec.m_flags & flags_t::on_ground) && !!(anim_info [ idx ][ 0 ].m_flags & flags_t::on_ground) ) {
		//dbg_print ( "delta_lean: %.4f\n", abs ( anim_layers [ 12 ].m_weight * 1000.0f - anim_info [ idx ][ 0 ].m_anim_layers [ anims::desync_side_t::desync_max ][ 12 ].m_weight * 1000.0f ) );
		//dbg_print ( "delta_weight: %.4f\n", abs ( anim_layers [ 6 ].m_weight * 1000.0f - anim_info [ idx ][ 0 ].m_anim_layers [ anims::desync_side_t::desync_max ][ 6 ].m_weight * 1000.0f ) );

		/* standing */
		if ( speed_2d <= 0.1f ) {
			if ( rdata::was_moving [ idx ] ) {
				if ( !rdata::new_resolve [ idx ] )
					rdata::prefer_edge [ idx ] = true;

				rdata::new_resolve [ idx ] = false;
				rdata::was_moving [ idx ] = false;
			}

			if ( !anim_layers [ 3 ].m_weight
				&& !anim_layers [ 3 ].m_cycle
				&& !anim_layers [ 6 ].m_weight
				&& !anim_layers [ 6 ].m_playback_rate ) {
				anims::desync_side_t new_desync_side = anims::desync_side_t::desync_middle;

				if ( delta_yaw > 0.0f && rdata::resolved_side [ idx ] < anims::desync_side_t::desync_middle )
					new_desync_side = rdata::resolved_side [ idx ];
				else if ( delta_yaw <= 0.0f && rdata::resolved_side [ idx ] >= anims::desync_side_t::desync_middle )
					new_desync_side = rdata::resolved_side [ idx ];
				else
					new_desync_side = ( delta_yaw <= 0.0f ) ? anims::desync_side_t::desync_right_max : anims::desync_side_t::desync_left_max;

				if ( rdata::resolved_side [ idx ] != new_desync_side )
					features::ragebot::get_misses ( idx ).bad_resolve = 0;

				rdata::resolved_side [ idx ] = new_desync_side;
				rdata::new_resolve [ idx ] = true;
				rdata::prefer_edge [ idx ] = false;
			}
		}
		/* moving */
		else if ( anim_layers [ 6 ].m_weight > 0.0f
			&& ( anim_layers [ 12 ].m_weight * 1000.0f <= 1.0f
			&& abs ( anim_layers [ 6 ].m_weight - anim_info [ idx ][ 0 ].m_anim_layers [ anims::desync_side_t::desync_max ][6].m_weight ) * 1000.0f <= 1.0f ) ) {
			//dbg_print ( "resolver update\n");

			const auto middle_delta_rate = abs (
				anim_layers [ 6 ].m_playback_rate
				- rec.m_anim_layers [ anims::desync_side_t::desync_middle ][ 6 ].m_playback_rate ) * 1000.0f;
			const auto left_half_delta_rate = abs (
				anim_layers [ 6 ].m_playback_rate
				- rec.m_anim_layers [ anims::desync_side_t::desync_left_half ][ 6 ].m_playback_rate ) * 1000.0f;
			const auto right_half_delta_rate = abs (
				anim_layers [ 6 ].m_playback_rate
				- rec.m_anim_layers [ anims::desync_side_t::desync_right_half ][ 6 ].m_playback_rate ) * 1000.0f;
			const auto left_delta_rate = abs (
				anim_layers [ 6 ].m_playback_rate
				- rec.m_anim_layers [ anims::desync_side_t::desync_left_max ][ 6 ].m_playback_rate ) * 1000.0f;
			const auto right_delta_rate = abs (
				anim_layers [ 6 ].m_playback_rate
				- rec.m_anim_layers [ anims::desync_side_t::desync_right_max ][ 6 ].m_playback_rate ) * 1000.0f;

			dbg_print ( "left: %.4f\n", left_delta_rate );
			dbg_print ( "lhalf: %.4f\n", left_half_delta_rate );
			dbg_print ( "mid: %.4f\n", middle_delta_rate );
			dbg_print ( "rhalf: %.4f\n", right_half_delta_rate );
			dbg_print ( "right: %.4f\n", right_delta_rate );

			if ( middle_delta_rate < left_delta_rate
				|| right_delta_rate <= left_delta_rate
				|| left_delta_rate > 0.1f ) {
				if ( middle_delta_rate >= right_delta_rate
					&& left_delta_rate > right_delta_rate
					&& right_delta_rate <= 0.1f ) {
					if ( rdata::resolved_side [ idx ] != anims::desync_side_t::desync_right_max )
						features::ragebot::get_misses ( idx ).bad_resolve = 0;
					rdata::resolved_side [ idx ] = anims::desync_side_t::desync_right_max;
					rdata::new_resolve [ idx ] = true;
					rdata::prefer_edge [ idx ] = false;
				}
			}
			else {
				//const auto left_deltas = abs ( left_half_delta_rate - left_delta_rate );
				//
				//if ( left_half_delta_rate < left_delta_rate ) {
				//	if ( rdata::resolved_side [ idx ] != anims::desync_side_t::desync_left_half )
				//		features::ragebot::get_misses ( idx ).bad_resolve = 0;
				//	rdata::resolved_side [ idx ] = anims::desync_side_t::desync_left_half;
				//	rdata::new_resolve [ idx ] = true;
				//	rdata::prefer_edge [ idx ] = false;
				//}
				//else {
					if ( rdata::resolved_side [ idx ] != anims::desync_side_t::desync_left_max )
						features::ragebot::get_misses ( idx ).bad_resolve = 0;
					rdata::resolved_side [ idx ] = anims::desync_side_t::desync_left_max;
					rdata::new_resolve [ idx ] = true;
					rdata::prefer_edge [ idx ] = false;
			//	}
			}

			rdata::was_moving [ idx ] = true;
		}
	}

	const auto src = g::local->origin ( ) + vec3_t ( 0.0f, 0.0f, 64.0f ) + g::local->vel ( ) * cs::ticks2time ( 1 );
	const auto eyes_max = ent->origin ( ) + vec3_t ( 0.0f, 0.0f, 64.0f ) + ent->vel ( ) * cs::ticks2time ( 1 );
	auto fwd = ( eyes_max - src ).normalized();

	if ( !fwd.is_valid ( ) ) {
		last_recorded_resolve [ idx ] = rdata::resolved_side [ idx ];
		return false;
	}

	/* try to hide head */
	const auto right_dir = fwd.cross_product ( vec3_t ( 0.0f, 0.0f, 1.0f ) );
	const auto left_dir = -right_dir;
	const auto dmg_left = autowall::dmg ( g::local, ent, src + left_dir * 35.0f, eyes_max + fwd * 30.0f + left_dir * 10.0f, 0 /* pretend player would be there */ ) + autowall::dmg ( g::local, ent, src - left_dir * 10.0f, eyes_max + fwd * 30.0f + left_dir * 10.0f, 0 /* pretend player would be there */ );
	const auto dmg_right = autowall::dmg ( g::local, ent, src + right_dir * 35.0f, eyes_max + fwd * 30.0f + right_dir * 10.0f, 0 /* pretend player would be there */ ) + autowall::dmg ( g::local, ent, src - right_dir * 10.0f, eyes_max + fwd * 30.0f + right_dir * 10.0f, 0 /* pretend player would be there */ );
	const auto one_side_hittable = ( dmg_left && !dmg_right ) || ( !dmg_left && dmg_right );
	const auto occluded_side = ( dmg_left > dmg_right ) ? anims::desync_side_t::desync_left_max : anims::desync_side_t::desync_right_max;
	auto target_side_tmp = rdata::resolved_side [ idx ];

	/*
	*	- Put head towards wall if only one side is hittable and we don't have enough information
	*/
	//if ( rdata::prefer_edge [ idx ] && one_side_hittable ) {
	//	if ( occluded_side < anims::desync_side_t::desync_middle && target_side_tmp < anims::desync_side_t::desync_middle )
	//		target_side_tmp = rdata::resolved_side [ idx ];
	//	else if ( occluded_side > anims::desync_side_t::desync_middle && target_side_tmp > anims::desync_side_t::desync_middle )
	//		target_side_tmp = rdata::resolved_side [ idx ];
	//	else
	//		target_side_tmp = occluded_side;
	//}

	/* bruteforce from Onetap, except a bit more agressive, with different order depending on weapon (skeet does this as well) */
	const auto has_accurate_weapon = g::local && g::local->weapon ( ) && g::local->weapon ( )->item_definition_index ( ) == weapons_t::ssg08;
	const auto has_bad_desync = /*ent->desync_amount ( ) <= 34.0f*/ false;

	auto bruteforce = [&](int brute_mode /* 0 = none, 1 = opposite, 2 = close, 3 = fast, 4 = fast opposite*/) {
		switch (brute_mode) {
		case 0: {
			rec.m_side = target_side_tmp;
		} break;
		case 1: {
			switch (target_side_tmp) {
			case anims::desync_side_t::desync_left_max: rec.m_side = anims::desync_side_t::desync_right_max; break;
			case anims::desync_side_t::desync_left_half: rec.m_side = anims::desync_side_t::desync_middle; break;
			case anims::desync_side_t::desync_right_max: rec.m_side = anims::desync_side_t::desync_left_max; break;
			case anims::desync_side_t::desync_right_half: rec.m_side = anims::desync_side_t::desync_left_half; break;
			case anims::desync_side_t::desync_middle: rec.m_side = anims::desync_side_t::desync_left_half; break;
			}
		} break;
		case 2: {
			switch (target_side_tmp) {
			case anims::desync_side_t::desync_left_max: rec.m_side = anims::desync_side_t::desync_left_half; break;
			case anims::desync_side_t::desync_left_half: rec.m_side = anims::desync_side_t::desync_left_max; break;
			case anims::desync_side_t::desync_right_max: rec.m_side = anims::desync_side_t::desync_middle; break;
			case anims::desync_side_t::desync_right_half: rec.m_side = anims::desync_side_t::desync_right_max; break;
			case anims::desync_side_t::desync_middle: rec.m_side = anims::desync_side_t::desync_right_max; break;
			}
		} break;
		case 3: {
			switch (target_side_tmp) {
			case anims::desync_side_t::desync_left_max: rec.m_side = anims::desync_side_t::desync_left_max; break;
			case anims::desync_side_t::desync_left_half: rec.m_side = anims::desync_side_t::desync_middle; break;
			case anims::desync_side_t::desync_right_max: rec.m_side = anims::desync_side_t::desync_middle; break;
			case anims::desync_side_t::desync_right_half: rec.m_side = anims::desync_side_t::desync_middle; break;
			case anims::desync_side_t::desync_middle: rec.m_side = anims::desync_side_t::desync_middle; break;
			}
		} break;
		case 4: {
			switch (target_side_tmp) {
			case anims::desync_side_t::desync_left_max: rec.m_side = anims::desync_side_t::desync_middle; break;
			case anims::desync_side_t::desync_left_half: rec.m_side = anims::desync_side_t::desync_left_max; break;
			case anims::desync_side_t::desync_right_max: rec.m_side = anims::desync_side_t::desync_left_max; break;
			case anims::desync_side_t::desync_right_half: rec.m_side = anims::desync_side_t::desync_left_max; break;
			case anims::desync_side_t::desync_middle: rec.m_side = anims::desync_side_t::desync_left_max; break;
			}
		} break;
		default: rec.m_side = target_side_tmp; break;
		}
	};

	switch (features::ragebot::get_misses(idx).bad_resolve % (has_bad_desync ? 2 : 3)) {
	case 0: bruteforce(has_bad_desync ? 3 : 0); break;
	case 1: bruteforce(has_bad_desync ? 4 : (has_accurate_weapon ? 1 : 2)); break;
	case 2: bruteforce(has_bad_desync ? 3 : (has_accurate_weapon ? 2 : 1)); break;
	default: bruteforce(has_bad_desync ? 3 : 0); break;
	}

	last_recorded_resolve [ idx ] = rec.m_side;
	rdata::queued_hit [ idx ] = false;

	const auto offset = -ent->desync_amount ( ) + static_cast< float >( rec.m_side ) * ( ent->desync_amount ( ) * 0.5f );
	last_abs_yaw [ idx ] = cs::normalize ( cs::normalize ( rec.m_anim_state [ anims::desync_side_t::desync_max ].m_eye_yaw ) + offset );

	return true;
}