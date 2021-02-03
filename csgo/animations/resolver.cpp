﻿#include <array>
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

void anims::resolver::process_impact ( event_t* event ) {
	auto shooter = cs::i::ent_list->get< player_t* > ( cs::i::engine->get_player_for_userid ( event->get_int ( _ ( "userid" ) ) ) );
	auto& target = features::ragebot::get_target ( );

	if ( !g::local || !g::local->alive ( ) || !shooter || shooter != g::local )
		return;

	const auto impact_pos = vec3_t ( event->get_float ( _ ( "x" ) ), event->get_float ( _ ( "y" ) ), event->get_float ( _ ( "z" ) ) );

	if ( target ) {
		const auto shot_pos = features::ragebot::get_shot_pos ( target->idx ( ) );

		if ( !target || target->dormant ( ) || target->idx ( ) <= 0 || target->idx ( ) > 64 || shot_pos == vec3_t ( 0.0f, 0.0f, 0.0f ) || !g::local->weapon ( ) || !g::local->weapon ( )->data ( ) )
			return;

		auto target_rec = features::ragebot::get_lag_rec ( target->idx ( ) );
		const auto towards = cs::angle_vec ( cs::calc_angle ( shot_pos, impact_pos ) );
		const auto vec_max = shot_pos + towards * g::local->weapon ( )->data ( )->m_range;

		const auto backup_origin = target->origin ( );
		//auto backup_abs_origin = target->abs_origin ( );
		const auto backup_bone_cache = target->bone_cache ( );
		const auto backup_mins = target->mins ( );
		const auto backup_maxs = target->maxs ( );

		target->origin ( ) = target_rec.m_origin;
		//target->set_abs_origin ( target_rec.m_origin );
		target->bone_cache ( ) = target_rec.m_aim_bones [ features::ragebot::get_misses ( target->idx ( ) ).bad_resolve % 3 ].data ( );
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
std::array< float, 65 > last_recorded_resolve { std::numeric_limits<float>::max ( ) };
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
	case 4: cs::i::engine->client_cmd_unrestricted ( _ ( "play player\\neck_snap_01" ) ); break;
	case 5: cs::i::engine->client_cmd_unrestricted ( _ ( "play weapons\\flashbang\\grenade_hit1" ) ); break;
	case 6: cs::i::engine->client_cmd_unrestricted ( _ ( "play physics\\glass\\glass_impact_bullet4" ) ); break;
	case 7: LI_FN ( PlaySoundA ) ( reinterpret_cast < const char* > ( hitsound_bell ), nullptr, SND_MEMORY | SND_ASYNC ); break;
	case 8: LI_FN ( PlaySoundA ) ( reinterpret_cast < const char* > ( hitsound_cod ), nullptr, SND_MEMORY | SND_ASYNC ); break;
	case 9: LI_FN ( PlaySoundA ) ( reinterpret_cast < const char* > ( hitsound_rattle ), nullptr, SND_MEMORY | SND_ASYNC ); break;
	case 10: LI_FN ( PlaySoundA ) ( reinterpret_cast < const char* > ( hitsound_ramune ), nullptr, SND_MEMORY | SND_ASYNC ); break;
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
				print_console ( { 1.0f, 1.0f, 1.0f, 1.0f }, _ ( " ～ missed shot due to bad resolve at %.1f°" ), (rec.m_poses[11] - 0.5f) * 120.0f );

				if ( rec.m_shot ) {
					print_console ( { 1.0f, 1.0f, 1.0f, 1.0f }, _ ( ", " ) );
					print_console ( { 0.678f, 1.0f, 0.996f, 1.0f }, _ ( "onshot" ) );
				}

				if ( rec.m_predicted ) {
					print_console ( { 1.0f, 1.0f, 1.0f, 1.0f }, _ ( ", " ) );
					print_console ( { 0.678f, 1.0f, 0.682f, 1.0f }, _ ( "predicted" ) );
				}

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

		memcpy ( hit_matrix_rec.back ( ).m_bones.data ( ), features::ragebot::get_lag_rec ( pl_idx ).m_aim_bones [ features::ragebot::get_misses ( pl_idx ).bad_resolve % 3 ].data ( ), sizeof matrix3x4_t * 128 );

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

			if ( rec.m_predicted ) {
				print_console ( { 1.0f, 1.0f, 1.0f, 1.0f }, _ ( ", " ) );
				print_console ( { 0.678f, 1.0f, 0.682f, 1.0f }, _ ( "predicted" ) );
			}

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

		memcpy ( hit_matrix_rec.back ( ).m_bones.data ( ), features::ragebot::get_lag_rec ( pl_idx ).m_aim_bones [ features::ragebot::get_misses ( pl_idx ).bad_resolve % 3 ].data ( ), sizeof matrix3x4_t * 128 );

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

			if ( rec.m_predicted ) {
				print_console ( { 1.0f, 1.0f, 1.0f, 1.0f }, _ ( ", " ) );
				print_console ( { 0.678f, 1.0f, 0.682f, 1.0f }, _ ( "predicted" ) );
			}

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
	static auto& bullet_impacts = options::vars [ _ ( "visuals.other.bullet_impacts" ) ].val.b;
	static auto& bullet_tracer_color = options::vars [ _ ( "visuals.other.bullet_tracer_color" ) ].val.c;
	static auto& bullet_impact_color = options::vars [ _ ( "visuals.other.bullet_impact_color" ) ].val.c;

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

		const auto alpha = calc_alpha ( impact.m_time, 2.0f, bullet_tracer_color.a * 255.0f );
		const auto alpha1 = calc_alpha ( impact.m_time, 2.0f, bullet_impact_color.a * 255.0f );
		const auto alpha2 = calc_alpha ( impact.m_time, 2.0f, 255 );

		if ( !alpha && !alpha1 && !alpha2 ) {
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
	static auto& bullet_impacts = options::vars [ _ ( "visuals.other.bullet_impacts" ) ].val.b;
	static auto& bullet_tracer_color = options::vars [ _ ( "visuals.other.bullet_tracer_color" ) ].val.c;
	static auto& bullet_impact_color = options::vars [ _ ( "visuals.other.bullet_impact_color" ) ].val.c;

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

		const auto alpha = calc_alpha ( impact.m_time, 2.0f, bullet_tracer_color.a * 255.0f );
		const auto alpha1 = calc_alpha ( impact.m_time, 2.0f, bullet_impact_color.a * 255.0f );
		const auto alpha2 = calc_alpha ( impact.m_time, 2.0f, 255 );

		if ( !alpha && !alpha1 && !alpha2 ) {
			impact_recs.erase ( impact_recs.begin ( ) + cur_ray );
			continue;
		}

		cs::render::world_to_screen ( scrn_src, impact.m_src );
		cs::render::world_to_screen ( scrn_dst, impact.m_dst );

		impact.m_clr = rgba ( static_cast< int >( bullet_tracer_color.r * 255.0f ), static_cast< int >( bullet_tracer_color.g * 255.0f ), static_cast< int >( bullet_tracer_color.b * 255.0f ), alpha );

		vec3_t dim;
		render::text_size ( impact.m_msg, _ ( "esp_font" ), dim );

		if ( impact.m_hurt && bullet_impacts ) {
			render::text ( scrn_dst.x - dim.x / 2, scrn_dst.y - 26, impact.m_msg, ( "esp_font" ), rgba ( 145, 255, 0, alpha2 ), true );
			render::cube ( impact.m_dst, 4, rgba ( static_cast< int >( bullet_impact_color.r * 255.0f ), static_cast< int >( bullet_impact_color.g * 255.0f ), static_cast< int >( bullet_impact_color.b * 255.0f ), alpha1 ) );
		}

		cur_ray++;
	}
}

float anims::resolver::resolve_yaw ( player_t* pl, bool update ) {
	VM_TIGER_BLACK_START
	static auto& resolver = options::vars [ _ ( "ragebot.resolve_desync" ) ].val.b;

	security_handler::update ( );

	static std::array<std::array<animlayer_t, 13>, 65> old_anim_layers { {} };
	static std::array<animstate_t, 65> old_anim_state { };
	static std::array< float, 65 > old_simtime { 0.0f };
	static std::array< vec3_t, 65 > old_vel { vec3_t ( 0.0f, 0.0f ,0.0f ) };
	static std::array< bool, 65 > old_not_walking { false };
	static std::array < float, 65 > fake_ground_fraction { 0.0f };

	const auto anim_state = pl->animstate ( );
	const auto anim_layers = pl->layers ( );

	if ( !anim_state || !anim_layers ) {
		return std::numeric_limits<float>::max ( );
	}

	const auto idx = pl->idx ( );

	if ( !resolver || g::local->team ( ) == pl->team ( ) ) {
		if ( update ) {
			memcpy ( old_anim_layers [ idx ].data ( ), anim_layers, sizeof ( old_anim_layers [ idx ] ) );
			old_anim_state [ idx ] = *anim_state;
			old_simtime [ idx ] = pl->simtime ( );
			old_vel [ idx ] = pl->vel ( );
		}

		return std::numeric_limits<float>::max ( );
	}

	player_info_t pl_info;
	cs::i::engine->get_player_info ( idx, &pl_info );

	/* bot check */
	if ( pl_info.m_fake_player ) {
		if ( update ) {
			memcpy ( old_anim_layers [ idx ].data ( ), anim_layers, sizeof ( old_anim_layers [ idx ] ) );
			old_anim_state [ idx ] = *anim_state;
			old_simtime [ idx ] = pl->simtime ( );
			old_vel [ idx ] = pl->vel ( );
		}

		return std::numeric_limits<float>::max ( );
	}

	if ( pl->desync_amount ( ) <= 38.0f )
		features::ragebot::get_misses ( idx ).bad_resolve = 0;

	static std::array< float, 65 > last_shot_time { 0.0f };
	const auto shooting = pl->weapon ( ) && ( pl->weapon ( )->last_shot_time ( ) > pl->old_simtime ( ) && pl->weapon ( )->last_shot_time ( ) <= pl->simtime ( ) );

	if ( shooting )
		last_shot_time [ idx ] = cs::i::globals->m_curtime;

	if ( update ) {
		auto calc_ground_fraction = [ & ] ( float speed_2d ) {
			constexpr auto CS_PLAYER_SPEED_DUCK_MODIFIER = 0.340f;
			constexpr auto CS_PLAYER_SPEED_WALK_MODIFIER = 0.520f;
			constexpr auto CS_PLAYER_SPEED_RUN = 260.0f;

			if ( old_not_walking [ idx ] )
				fake_ground_fraction [ idx ] -= cs::ticks2time ( 1 ) * 2.0f;
			else
				fake_ground_fraction [ idx ] += cs::ticks2time ( 1 ) * 2.0f;

			if ( speed_2d > ( CS_PLAYER_SPEED_RUN * CS_PLAYER_SPEED_WALK_MODIFIER ) && old_not_walking [ idx ] )
				old_not_walking [ idx ] = false;
			else if ( speed_2d < ( CS_PLAYER_SPEED_RUN * CS_PLAYER_SPEED_WALK_MODIFIER ) && !old_not_walking [ idx ] )
				old_not_walking [ idx ] = true;

			fake_ground_fraction [ idx ] = std::clamp< float > ( fake_ground_fraction [ idx ], 0.0f, 1.0f );

			return fake_ground_fraction [ idx ];
		};

		calc_ground_fraction ( pl->vel ( ).length_2d ( ) );

		// TEMP
		//const auto delta_ticks = cs::time2ticks ( pl->simtime ( ) - pl->old_simtime ( ) );
		//
		//for ( auto i = 0; i < delta_ticks; i++ ) {
		//	const auto simulated_vel = old_vel [ idx ] + ( pl->vel ( ) - old_vel [ idx ] ) * ( static_cast< float >( i + 1 ) / static_cast< float >( delta_ticks ) );
		//	calc_ground_fraction ( simulated_vel.length_2d ( ) );
		//}
	}

	const auto predicted_ground_fraction = fake_ground_fraction [ idx ];

	/* simulate movement and predict ground fraction during choked commands */
	if ( abs ( anim_state->m_pitch ) < 60.0f /*shooting*/ ) {
		if ( shooting ) {
			if ( update ) {
				memcpy ( old_anim_layers [ idx ].data ( ), anim_layers, sizeof ( old_anim_layers [ idx ] ) );
				old_anim_state [ idx ] = *anim_state;
				old_simtime [ idx ] = pl->simtime ( );
				old_vel [ idx ] = pl->vel ( );
			}

			if ( last_recorded_resolve [ idx ] == std::numeric_limits<float>::max ( ) )
					return 0.0f;
				
			return copysign ( pl->desync_amount(), cs::normalize ( last_recorded_resolve [ idx ] ) - cs::normalize ( anim_state->m_eye_yaw ) );
		}
		else {
			return 0.0f;
		}
	}

	/*
	*	OneTap::Features::AnimationSystem::SetAngles
	*	OneTap::Features::RageBot::Resolver::AngleMode
	*/
	static std::array< float, 65 > desync_offset { 0.0f };
	static std::array< float, 65 > desync_offset_lby { 0.0f };

	const auto speed = pl->vel ( ).length_2d ( );

	if ( !!( pl->flags ( ) & flags_t::on_ground ) && update ) {
		/* predict angle from animlayers */
		const auto max_desync = pl->desync_amount ( );

		if ( anim_layers [ 6 ].m_weight == 0.0f ) {
			const auto delta_yaw = cs::normalize ( cs::normalize ( anim_state->m_eye_yaw ) - cs::normalize ( anim_state->m_abs_yaw ) );

			desync_offset_lby [ idx ] = copysign (
				max_desync,
				( abs ( delta_yaw ) > 35.0f ) ? delta_yaw : desync_offset [ idx ]
			);
		}
		else if ( abs( anim_layers [ 6 ].m_weight * 1000.0f - old_anim_layers [ idx ][ 6 ].m_weight * 1000.0f ) <= 1.0f ) {
			auto calc_playback_rate = [ & ] (
				float ground_fraction,
				float offset
				) -> float {
				static std::array<animlayer_t, 13> backup_anim_layers {};
				static animstate_t backup_anim_state {};
				static std::array<float, 24> backup_poses {};

				memcpy ( backup_anim_layers.data ( ), anim_layers, sizeof ( backup_anim_layers ) );
				backup_anim_state = *anim_state;
				backup_poses = pl->poses ( );

				anim_state->m_feet_yaw
					= anim_state->m_abs_yaw
					= cs::normalize ( cs::normalize ( anim_state->m_eye_yaw ) + offset );

				anim_state->m_move_yaw_pose.set_value ( pl, std::clamp ( cs::normalize ( cs::normalize ( cs::rad2deg ( atan2 ( -pl->vel ( ).y, -pl->vel ( ).x ) ) ) - cs::normalize ( anim_state->m_abs_yaw ) ), -180.0f, 180.0f ) / 360.0f + 0.5f );
				anim_state->m_body_yaw_pose.set_value ( pl, std::clamp ( cs::normalize ( cs::normalize ( anim_state->m_eye_yaw ) - cs::normalize ( anim_state->m_abs_yaw ) ), -60.0f, 60.0f ) / 120.0f + 0.5f );

				//    get the sequence used in the calc
				char dest [ 64 ] { '\0' };
				sprintf_s ( dest, "move_%s", pl->animstate ( )->get_weapon_move_animation ( ) );

				int seq = pl->lookup_sequence ( dest );

				if ( seq == -1 )
					seq = pl->lookup_sequence ( _ ( "move" ) );

				//    cycle rate
				float flMoveCycleRate = 0;

				if ( speed ) {
					flMoveCycleRate = pl->get_sequence_cycle_rate_server ( seq );
					float flSequenceGroundSpeed = std::max ( pl->get_sequence_move_distance ( *reinterpret_cast< studiohdr_t** >( reinterpret_cast< uintptr_t > ( pl ) + 0x294C ), seq ) / ( 1.0f / flMoveCycleRate ), 0.001f );
					flMoveCycleRate *= speed / flSequenceGroundSpeed;
					flMoveCycleRate *= std::lerp ( 1.0f, 0.85f, ground_fraction );
				}

				float flLocalCycleIncrement = flMoveCycleRate * cs::ticks2time ( 1 );

				*anim_state = backup_anim_state;
				memcpy ( anim_layers, backup_anim_layers.data ( ), sizeof ( backup_anim_layers ) );
				pl->poses ( ) = backup_poses;

				return flLocalCycleIncrement;
			};

			static std::array<float, 5> simulated_playback_rates { 0.0f };

			simulated_playback_rates [ 0 ] = calc_playback_rate ( predicted_ground_fraction, -1.0f * max_desync );
			simulated_playback_rates [ 1 ] = calc_playback_rate ( predicted_ground_fraction, -0.5f * max_desync );
			simulated_playback_rates [ 2 ] = calc_playback_rate ( predicted_ground_fraction, 0.0f * max_desync );
			simulated_playback_rates [ 3 ] = calc_playback_rate ( predicted_ground_fraction, 0.5f * max_desync );
			simulated_playback_rates [ 4 ] = calc_playback_rate ( predicted_ground_fraction, 1.0f * max_desync );

			float base_offset = -1.0f;
			float closest_playback_rate = std::numeric_limits<float>::max ( );

			for ( auto& playback_rate : simulated_playback_rates ) {
				const auto playback_delta = abs ( anim_layers [ 6 ].m_playback_rate - playback_rate );

				if ( playback_delta < closest_playback_rate && base_offset != 0.5f ) {
					desync_offset [ idx ] = base_offset * max_desync;
					closest_playback_rate = playback_delta;
				}

				base_offset += 0.5f;
			}
		}
	}

	float yaw_offset = 0.0f;

	if ( anim_layers [ 6 ].m_weight == 0.0f || !( pl->flags ( ) & flags_t::on_ground ) )
		yaw_offset = desync_offset_lby [ idx ];
	else
		yaw_offset = desync_offset [ idx ];

	last_recorded_resolve [ idx ] = cs::normalize ( anim_state->m_eye_yaw + yaw_offset );

	if ( update ) {
		memcpy ( old_anim_layers [ idx ].data ( ), anim_layers, sizeof ( old_anim_layers [ idx ] ) );
		old_anim_state [ idx ] = *anim_state;
		old_simtime [ idx ] = pl->simtime ( );
		old_vel [ idx ] = pl->vel ( );
	}

	rdata::queued_hit [ idx ] = false;
	
	VM_TIGER_BLACK_END
	return yaw_offset;
}