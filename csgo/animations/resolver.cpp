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
		auto backup_abs_origin = target->abs_origin ( );
		const auto backup_bone_cache = target->bone_cache ( );
		const auto backup_mins = target->mins ( );
		const auto backup_maxs = target->maxs ( );

		target->origin ( ) = target_rec.m_origin;
		target->set_abs_origin ( target_rec.m_origin );
		target->bone_cache ( ) = target_rec.m_aim_bones[ target_rec.m_side ].data();
		target->mins ( ) = target_rec.m_mins;
		target->maxs ( ) = target_rec.m_maxs;

		auto hit_hitgroup = -1;
		vec3_t impact_out = impact_pos;

		const auto dmg = autowall::dmg ( g::local, target, shot_pos, vec_max, -1, &impact_out );

		target->origin ( ) = backup_origin;
		target->set_abs_origin ( backup_abs_origin );
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

__forceinline float angle_diff ( float dst, float src ) {
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

std::array < bool , 65 > found_extended { false };
std::array < float , 65 > pitch_timer { 0.0f };
std::array < float , 65 > last_abs_yaw { 0.0f };
std::array < anims::desync_side_t , 65 > extended_side { anims::desync_side_t::desync_middle };
std::array < anims::desync_side_t , 65 > movement_side { anims::desync_side_t::desync_middle };
std::array < anims::desync_side_t , 65 > initial_resolve_side { anims::desync_side_t::desync_middle };

bool anims::resolver::resolve_desync( player_t* ent , anim_info_t& rec ) {
	/* set default desync direction to middle (0 deg range) */
	rec.m_side = anims::desync_side_t::desync_middle;

	static auto& resolver = options::vars[ _( "ragebot.resolve_desync" ) ].val.b;

	security_handler::update( );

	const auto speed = ent->vel( ).length_2d( );
	const auto anim_state = ent->animstate( );
	const auto anim_layers = ent->layers( );
	const auto idx = ent->idx( );

	if ( !resolver || !g::local || !g::local->alive( ) || !anim_state || !anim_layers || ent->team( ) == g::local->team( ) ) {
		last_recorded_resolve[ idx ] = anims::desync_side_t::desync_middle;
		extended_side[ idx ] = anims::desync_side_t::desync_middle;
		movement_side[ idx ] = anims::desync_side_t::desync_middle;
		found_extended[ idx ] = false;
		return false;
	}

	player_info_t pl_info;
	cs::i::engine->get_player_info( idx , &pl_info );

	/* bot check */
	if ( pl_info.m_fake_player ) {
		last_recorded_resolve[ idx ] = anims::desync_side_t::desync_middle;
		extended_side[ idx ] = anims::desync_side_t::desync_middle;
		movement_side[ idx ] = anims::desync_side_t::desync_middle;
		found_extended[ idx ] = false;
		return false;
	}

	const auto has_legit_aa = ( !anim_layers[ 6 ].m_weight && abs( cs::normalize( cs::normalize( anim_state->m_eye_yaw ) - cs::normalize( ent->lby( ) ) ) ) > 60.0f ) || cs::time2ticks( abs( ent->simtime( ) - ent->old_simtime( ) ) ) >= 2;

	if ( abs( anim_state->m_pitch ) < 70.0f && !has_legit_aa ) {
		auto closest_delta_yaw = std::numeric_limits<float>::max( );

		/* resolve onshot */
		if ( abs( pitch_timer[ idx ] - cs::i::globals->m_curtime ) < 0.8f ) {
			const auto delta = cs::normalize( cs::normalize( last_abs_yaw[ idx ] ) - cs::normalize( anim_state->m_eye_yaw ) );

			for ( auto side = 0; side < anims::desync_side_t::desync_max; side++ ) {
				const auto offset = -60.0f + static_cast< float >( side ) * 30.0f;
				const auto delta_yaw = abs( delta - offset );

				if ( delta_yaw < closest_delta_yaw ) {
					rec.m_side = static_cast< anims::desync_side_t >( side );
					closest_delta_yaw = delta_yaw;
				}
			}

			return true;
		}

		/* resolve legit */
		rec.m_side = anims::desync_side_t::desync_middle;
		return true;
	}
	else {
		pitch_timer[ idx ] = cs::i::globals->m_curtime;
	}

	/* start of resolver */
	if ( !anim_layers[ 6 ].m_weight ) {
		const auto delta_yaw = cs::normalize( cs::normalize( anim_state->m_eye_yaw ) - cs::normalize( ent->lby( ) ) );

		if ( abs( delta_yaw ) > 35.0f ) {
			extended_side[ idx ] = ( delta_yaw < 0.0f ) ? anims::desync_side_t::desync_left_max : anims::desync_side_t::desync_right_max;
			found_extended[ idx ] = true;
		}
	}
	else if ( !anim_info[ idx ].empty() ) {
		const auto has_static = abs( anim_layers[ 6 ].m_weight - anim_info[ idx ][ 0 ].m_anim_layers[ anims::desync_side_t::desync_middle ][ 6 ].m_weight ) < 0.003f;
		const auto has_jitter_side_1 = anim_info[ idx ].size( ) >= 3
			&& anim_layers[ 6 ].m_weight > anim_info[ idx ][ 0 ].m_anim_layers[ anims::desync_side_t::desync_middle ][ 6 ].m_playback_rate
			&& anim_layers[ 6 ].m_playback_rate < anim_info[ idx ][ 1 ].m_anim_layers[ anims::desync_side_t::desync_middle ][ 6 ].m_playback_rate
			&& anim_layers[ 6 ].m_playback_rate > anim_info[ idx ][ 2 ].m_anim_layers[ anims::desync_side_t::desync_middle ][ 6 ].m_playback_rate;
		const auto has_jitter_side_2 = anim_info[ idx ].size( ) >= 3
			&& anim_layers[ 6 ].m_playback_rate < anim_info[ idx ][ 0 ].m_anim_layers[ anims::desync_side_t::desync_middle ][ 6 ].m_playback_rate
			&& anim_layers[ 6 ].m_playback_rate > anim_info[ idx ][ 1 ].m_anim_layers[ anims::desync_side_t::desync_middle ][ 6 ].m_playback_rate
			&& anim_layers[ 6 ].m_playback_rate < anim_info[ idx ][ 2 ].m_anim_layers[ anims::desync_side_t::desync_middle ][ 6 ].m_playback_rate;
		const auto has_jitter = has_jitter_side_1 || has_jitter_side_2;
		
		if ( anim_layers[ 12 ].m_weight < 0.006f ) {
			/* reset resolver if we recieved new, reliable data */
			auto nearest_playback_rate_delta = std::numeric_limits<float>::max( );
			auto nearest_side = anims::desync_side_t::desync_middle;

			for ( auto side = 0; side < anims::desync_side_t::desync_max; side++ ) {
				const auto playback_rate_delta = abs( anim_layers[ 6 ].m_playback_rate - rec.m_anim_layers[ side ][ 6 ].m_playback_rate );

				if ( playback_rate_delta < nearest_playback_rate_delta ) {
					nearest_side = static_cast< anims::desync_side_t >( side );
					nearest_playback_rate_delta = playback_rate_delta;
				}
			}

			if ( nearest_playback_rate_delta != std::numeric_limits<float>::max( ) ) {
				movement_side[ idx ] = nearest_side;
				features::ragebot::get_misses( idx ).bad_resolve = 0;
			}

			extended_side[ idx ] = anims::desync_side_t::desync_middle;
			found_extended[ idx ] = false;
		}
	}

	const auto src = g::local->eyes ( );
	auto eyes_max = ent->origin() + vec3_t ( 0.0f, 0.0f, 64.0f );
	auto fwd = eyes_max - src;
	fwd.normalize ( );

	if ( !fwd.is_valid ( ) ) {
		last_recorded_resolve[ idx ] = anims::desync_side_t::desync_middle;
		extended_side[ idx ] = anims::desync_side_t::desync_middle;
		movement_side[ idx ] = anims::desync_side_t::desync_middle;
		found_extended[ idx ] = false;
		return false;
	}

	auto right_dir = fwd.cross_product ( vec3_t ( 0.0f, 0.0f, 1.0f ) );
	auto left_dir = -right_dir;
	auto dmg_left = autowall::dmg ( g::local, ent , src + left_dir * 10.0f, eyes_max + fwd * 30.0f + left_dir * 10.0f, 0 /* pretend player would be there */ ) + autowall::dmg( g::local , ent , src - left_dir * 10.0f , eyes_max + fwd * 30.0f + left_dir * 10.0f , 0 /* pretend player would be there */ );
	auto dmg_right = autowall::dmg ( g::local, ent , src + right_dir * 10.0f, eyes_max + fwd * 30.0f + right_dir * 10.0f, 0 /* pretend player would be there */ ) + autowall::dmg( g::local , ent , src - right_dir * 10.0f , eyes_max + fwd * 30.0f + right_dir * 10.0f , 0 /* pretend player would be there */ );

	if ( !features::ragebot::get_misses ( idx ).bad_resolve ) {
		if ( found_extended[ idx ] )
			initial_resolve_side[ idx ] = extended_side[ idx ];
		else if ( ( ( dmg_left && !dmg_right ) || ( !dmg_left && dmg_right ) ) && !anim_layers[ 6 ].m_weight )
			initial_resolve_side[ idx ] = ( dmg_left > dmg_right ) ? anims::desync_side_t::desync_left_max : anims::desync_side_t::desync_right_max;
		else
			initial_resolve_side[ idx ] = movement_side[ idx ];
	}

	/* bruteforce */
	/* low max desync, bruteforce faster */
	if ( ent->desync_amount( ) < 38.0f ) {
		switch ( features::ragebot::get_misses( idx ).bad_resolve % 2 ) {
		case 0: /* first side max */ {
			switch ( initial_resolve_side[ idx ] ) {
			case anims::desync_side_t::desync_left_max:
			case anims::desync_side_t::desync_left_half:
				rec.m_side = anims::desync_side_t::desync_left_max;
				break;
			case anims::desync_side_t::desync_middle:
			case anims::desync_side_t::desync_right_max:
			case anims::desync_side_t::desync_right_half:
				rec.m_side = anims::desync_side_t::desync_middle;
				break;
			}
		} break;
		case 1: /* opposite side max */ {
			switch ( initial_resolve_side[ idx ] ) {
			case anims::desync_side_t::desync_left_max:
			case anims::desync_side_t::desync_left_half:
				rec.m_side = anims::desync_side_t::desync_middle;
				break;
			case anims::desync_side_t::desync_middle:
			case anims::desync_side_t::desync_right_max:
			case anims::desync_side_t::desync_right_half:
				rec.m_side = anims::desync_side_t::desync_left_max;
				break;
			}
		} break;
		}
	}
	/* usual desync range */
	else {
		switch ( features::ragebot::get_misses( idx ).bad_resolve % 4 ) {
		case 0: /* first side */ {
			rec.m_side = initial_resolve_side[ idx ];
		} break;
		case 1: /* near side if moving, max opposite side if standing */ {
			/* near side */
			if ( anim_layers[ 6 ].m_weight ) {
				switch ( initial_resolve_side[ idx ] ) {
				case anims::desync_side_t::desync_left_max: rec.m_side = anims::desync_side_t::desync_left_half; break;
				case anims::desync_side_t::desync_left_half: rec.m_side = anims::desync_side_t::desync_left_max; break;
				case anims::desync_side_t::desync_middle:
					rec.m_side = anims::desync_side_t::desync_left_half;
					break;
				case anims::desync_side_t::desync_right_max:
				case anims::desync_side_t::desync_right_half:
					rec.m_side = anims::desync_side_t::desync_middle;
					break;
				}
			}
			/* max opposite side */
			else {
				switch ( initial_resolve_side[ idx ] ) {
				case anims::desync_side_t::desync_left_max:
				case anims::desync_side_t::desync_left_half:
					rec.m_side = anims::desync_side_t::desync_right_max;
					break;
				case anims::desync_side_t::desync_middle:
					rec.m_side = anims::desync_side_t::desync_left_max;
					break;
				case anims::desync_side_t::desync_right_max:
				case anims::desync_side_t::desync_right_half:
					rec.m_side = anims::desync_side_t::desync_left_max;
					break;
				}
			}
		} break;
		case 2: /* max opposite if moving, near side side if standing */ {
			/* max opposite side */
			if ( anim_layers[ 6 ].m_weight ) {
				switch ( initial_resolve_side[ idx ] ) {
				case anims::desync_side_t::desync_left_max:
				case anims::desync_side_t::desync_left_half:
					rec.m_side = anims::desync_side_t::desync_right_max;
					break;
				case anims::desync_side_t::desync_middle:
					rec.m_side = anims::desync_side_t::desync_left_max;
					break;
				case anims::desync_side_t::desync_right_max:
				case anims::desync_side_t::desync_right_half:
					rec.m_side = anims::desync_side_t::desync_left_max;
					break;
				}
			}
			/* near side */
			else {
				switch ( initial_resolve_side[ idx ] ) {
				case anims::desync_side_t::desync_left_max: rec.m_side = anims::desync_side_t::desync_left_half; break;
				case anims::desync_side_t::desync_left_half: rec.m_side = anims::desync_side_t::desync_left_max; break;
				case anims::desync_side_t::desync_middle:
					rec.m_side = anims::desync_side_t::desync_left_half;
					break;
				case anims::desync_side_t::desync_right_max:
				case anims::desync_side_t::desync_right_half:
					rec.m_side = anims::desync_side_t::desync_middle;
					break;
				}
			}
		} break;
		case 3: {
			rec.m_side = initial_resolve_side[ idx ];
			features::ragebot::get_misses( idx ).bad_resolve = 0;
		} break;
		}
	}

	last_recorded_resolve [ idx ] = rec.m_side;
	rdata::queued_hit [ idx ] = false;

	const auto offset = -60.0f + static_cast< float >( rec.m_side ) * 30.0f;
	last_abs_yaw[ idx ] = cs::normalize( cs::normalize( anim_state->m_eye_yaw ) + offset );
	
	return true;
}