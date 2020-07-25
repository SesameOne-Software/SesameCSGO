#include <array>
#include "animations.hpp"
#include "resolver.hpp"
#include "../features/ragebot.hpp"
#include "../globals.hpp"
#include "../security/security_handler.hpp"
#include "../features/lagcomp.hpp"
#include "../features/autowall.hpp"
#include "../renderer/d3d9.hpp"
#include "../menu/menu.hpp"
#include "../features/esp.hpp"
#include "../features/autowall.hpp"
#include "../hitsounds.h"
#include "../features/other_visuals.hpp"

template < typename ...args_t >
void print_console ( const oxui::color& clr, const char* fmt, args_t ...args ) {
	if ( !fmt )
		return;

	struct {
		uint8_t r, g, b, a;
	} s_clr;

	s_clr = { static_cast < uint8_t > ( clr.r ), static_cast < uint8_t > ( clr.g ), static_cast < uint8_t > ( clr.b ), static_cast < uint8_t > ( clr.a ) };

	static auto con_color_msg = reinterpret_cast< void ( * )( const decltype( s_clr )&, const char*, ... ) >( LI_FN ( GetProcAddress ) ( LI_FN ( GetModuleHandleA ) ( _ ( "tier0.dll" ) ), _ ( "?ConColorMsg@@YAXABVColor@@PBDZZ" ) ) );

	con_color_msg ( s_clr, fmt, args... );
}

struct cached_resolve_t {
	vec3_t m_origin;
	float m_correction, m_radius;
	int m_failed;

	bool within ( int pl ) {
		return csgo::i::ent_list->get < player_t* > ( pl ) && csgo::i::ent_list->get < player_t* > ( pl )->origin ( ).dist_to ( m_origin ) <= m_radius;
	}
};

struct impact_rec_t {
	vec3_t m_src, m_dst;
	std::wstring m_msg;
	float m_time;
	bool m_hurt;
	uint32_t m_clr;
};

std::array< float, 65 > resolver_confidence { 0.0f };
std::array< bool, 65 > received_new_particles { false };
std::array< float, 65 > particle_predicted_desync_offset { FLT_MAX };
std::deque< animations::resolver::hit_matrix_rec_t > hit_matrix_rec { };
std::deque< impact_rec_t > impact_recs { };
std::array< std::deque< cached_resolve_t >, 65 > cached_resolves { };

std::array< vec3_t, 65 > last_origin { };
std::array< vec3_t, 65 > last_angle { };

bool resolve_cached ( int pl_idx, float& yaw ) {
	for ( auto& cached_resolve : cached_resolves [ pl_idx ] ) {
		if ( cached_resolve.within ( pl_idx ) && !cached_resolve.m_failed ) {
			resolver_confidence [ pl_idx ] = 65.0f + 25.0f * ( ( cached_resolve.m_radius - std::clamp( cached_resolve.m_origin.dist_to ( csgo::i::ent_list->get < player_t* > ( pl_idx )->origin ( ) ), 0.0f, cached_resolve.m_radius ) ) / cached_resolve.m_radius );
			yaw += cached_resolve.m_correction;
			return true;
		}
	}

	return false;
}

/* if we miss while using a cached resolve, don't shoot at it again until we have it corrected again */
void fix_cached_resolves ( int pl_idx ) {
	for ( auto& cached_resolve : cached_resolves [ pl_idx ] )
		if ( cached_resolve.within ( pl_idx ) )
			cached_resolve.m_failed++;
}

float animations::resolver::get_confidence ( int pl_idx ) {
	return resolver_confidence [ pl_idx ];
}

void animations::resolver::process_blood ( const effect_data_t& effect_data ) {
	const auto idx = ( effect_data.m_ent_handle & 0xfff ) - 1;

	if ( effect_data.m_ent_handle == -1
		|| !idx
		|| idx > csgo::i::globals->m_max_clients
		/* make sure we only scan headshots; usually effect radius will be <= 1 (easiest and most accurate way to tell usually) */
		|| effect_data.m_radius > 1.0f
		/* make sure others or us shot at CURRENT record, so we have an much more accurate estimate of when the particle came from... ghetto af check, but works */
		/*|| ( effect_data.m_origin - ent->origin ( ) ).length_2d ( ) > 16.0f*/ )
		return;

	player_t* pl = nullptr;
	vec3_t origin, angle;

	if ( !features::lagcomp::data::all_records [ idx ].empty ( ) && ( pl = csgo::i::ent_list->get < player_t* > ( idx ) ) ) {
		auto found = false;

		/* find the record that they backtracked (if it was backtrack) */
		for ( auto& rec : features::lagcomp::data::all_records [ idx ] ) {
			const auto delta_2d = ( effect_data.m_origin - rec.m_origin ).length_2d ( );

			if ( delta_2d > 16.0f && delta_2d < 64.0f ) {
				origin = rec.m_origin;
				angle = rec.m_ang;
				found = true;
				break;
			}
		}

		if ( !found )
			return;
	}
	else {
		origin = last_origin [ idx ];
		angle = last_angle [ idx ];
	}

	/* predict desync without having to reverse everything (i'm too fucking lazy and dumb) */
	const auto angle_to_particle = csgo::calc_angle ( origin, effect_data.m_origin ).y;
	const auto angle_delta = csgo::normalize ( csgo::normalize ( angle_to_particle ) - csgo::normalize ( angle.y ) );

	//if ( std::fabsf ( angle_delta ) < 25.0f )
	//	return;

	print_console ( { 0, 255, 0, 255 }, "shot at valid record\n", effect_data.m_scale, effect_data.m_magnitude, effect_data.m_radius, effect_data.m_hitbox );

	particle_predicted_desync_offset [ idx ] = std::copysignf ( 1.0f, angle_delta );

	received_new_particles [ idx ] = true;
}

void animations::resolver::process_impact( event_t* event ) {
	auto shooter = csgo::i::ent_list->get< player_t* >( csgo::i::engine->get_player_for_userid( event->get_int( _( "userid" ) ) ) );
	auto& target = features::ragebot::get_target ( );

	if ( !g::local || !g::local->alive( ) || !target || target->dormant( ) || target->idx ( ) <= 0 || target->idx ( ) > 64 || !shooter || shooter != g::local || features::ragebot::get_shot_pos ( target->idx ( ) ) == vec3_t( 0.0f, 0.0f, 0.0f ) || !g::local->weapon ( ) || !g::local->weapon ( )->data ( ) )
		return;

	auto target_rec = features::ragebot::get_lag_rec ( target->idx ( ) );
	const auto impact_pos = vec3_t ( event->get_float ( _ ( "x" ) ), event->get_float ( _ ( "y" ) ), event->get_float ( _ ( "z" ) ) );
	const auto towards = csgo::angle_vec ( csgo::calc_angle( features::ragebot::get_shot_pos ( target->idx ( ) ), impact_pos ) );
	const auto vec_max = features::ragebot::get_shot_pos ( target->idx ( ) ) + towards * g::local->weapon ( )->data ( )->m_range;

	auto backup_abs_origin = target->abs_origin ( );
	const auto backup_origin = target->origin ( );
	const auto backup_min = target->mins ( );
	const auto backup_max = target->maxs ( );
	matrix3x4_t backup_bones [ 128 ];
	std::memcpy ( backup_bones, target->bone_cache ( ), sizeof matrix3x4_t * target->bone_count ( ) );

	target->mins ( ) = target_rec.m_min;
	target->maxs ( ) = target_rec.m_max;
	target->origin ( ) = target_rec.m_origin;
	target->set_abs_origin ( target_rec.m_origin );

	if ( features::ragebot::get_misses ( target->idx ( ) ).bad_resolve % 3 == 0 )
		std::memcpy ( target->bone_cache ( ), target_rec.m_bones1, sizeof matrix3x4_t * target->bone_count ( ) );
	else if ( features::ragebot::get_misses ( target->idx ( ) ).bad_resolve % 3 == 1 )
		std::memcpy ( target->bone_cache ( ), target_rec.m_bones2, sizeof matrix3x4_t * target->bone_count ( ) );
	else
		std::memcpy ( target->bone_cache ( ), target_rec.m_bones3, sizeof matrix3x4_t * target->bone_count ( ) );

	auto hit_hitgroup = -1;
	vec3_t impact_out = impact_pos;

	const auto dmg = autowall::dmg ( g::local, target, features::ragebot::get_shot_pos ( target->idx ( ) ), vec_max, -1, &impact_out );

	target->mins ( ) = backup_min;
	target->maxs ( ) = backup_max;
	target->origin ( ) = backup_origin;
	target->set_abs_origin ( backup_abs_origin );
	std::memcpy ( target->bone_cache ( ), backup_bones, sizeof matrix3x4_t * target->bone_count ( ) );

	rdata::impact_dmg [ target->idx ( ) ] = dmg;
	rdata::impacts [ target->idx ( ) ] = impact_pos;
}

std::array< bool, 65 > animations::resolver::flags::has_slow_walk { false };
std::array< bool, 65 > animations::resolver::flags::has_micro_movements { false };
std::array< bool, 65 > animations::resolver::flags::has_desync { false };
std::array< bool, 65 > animations::resolver::flags::has_jitter { false };
std::array< float, 65 > animations::resolver::flags::eye_delta { 0.0f };
std::array< float, 65 > animations::resolver::flags::last_pitch { 0.0f };
std::array< animlayer_t [ 15 ], 65 > animations::resolver::flags::anim_layers { { } };
std::array< bool, 65 > animations::resolver::flags::force_pitch_flick_yaw { false };

std::array< float, 65 > last_recorded_resolve1 { FLT_MAX };
std::array< float, 65 > last_recorded_resolve2 { FLT_MAX };
std::array< float, 65 > last_recorded_resolve3 { FLT_MAX };
std::array< int, 65 > last_recorded_tick { 0 };

std::array< int, 65 > hit_hitgroup { 0 };

void animations::resolver::process_hurt( event_t* event ) {
	OPTION ( int, hit_sound, "Sesame->C->Other->World->Hit Sound", oxui::object_dropdown );

	// ambient\atmosphere\firewerks_burst_02
	// buttons\weapon_confirm
	// buttons\light_power_on_switch_01
	// common\beep
	// doors\door_latch3
	// doors\door_metal_gate_close1
	// doors\heavy_metal_stop1
	// items\smallmedkit1
	// physics\concrete\concrete_break3
	// physics\glass\glass_impact_bullet4
	// physics\glass\glass_largesheet_break3
	// player\neck_snap_01
	// ui\counter_beep
	// weapons\c4\c4_beep1
	// weapons\awp\awp1
	// weapons\ak47\ak47-1
	// physics\metal\metal_canister_impact_hard1
	// buttons\arena_switch_press_02
	// player\pl_fallpain3
	// weapons\flashbang\flashbang_explode1

	// weapons\awp\awp_boltback
	// weapons\awp\awp_boltforward

	auto dmg = event->get_int ( _ ( "dmg_health" ) );
	auto attacker = csgo::i::ent_list->get< player_t* >( csgo::i::engine->get_player_for_userid( event->get_int( _( "attacker" ) ) ) );
	auto victim = csgo::i::ent_list->get< player_t* >( csgo::i::engine->get_player_for_userid( event->get_int( _( "userid" ) ) ) );
	auto hitgroup = event->get_int ( _ ( "hitgroup" ) );

	if ( !attacker || !victim || attacker != g::local || victim->team( ) == g::local->team( ) || victim->idx ( ) <= 0 || victim->idx ( ) > 64 )
		return;
	
	switch ( hit_sound ) {
	case 0: break;
	case 1: csgo::i::engine->client_cmd_unrestricted ( _ ( "play buttons\\arena_switch_press_02" ) ); break;
	case 2: csgo::i::engine->client_cmd_unrestricted ( _ ( "play player\\pl_fallpain3" ) ); break;
	case 3: csgo::i::engine->client_cmd_unrestricted ( _ ( "play weapons\\awp\\awp_boltback" ) ); break;
	case 4: csgo::i::engine->client_cmd_unrestricted ( _ ( "play player\\neck_snap_01" ) ); break;
	case 5: csgo::i::engine->client_cmd_unrestricted ( _ ( "play buttons\\light_power_on_switch_01" ) ); break;
	case 6: csgo::i::engine->client_cmd_unrestricted ( _ ( "play physics\\glass\\glass_impact_bullet4" ) ); break;
	case 7: LI_FN( PlaySoundA ) ( reinterpret_cast < const char* > ( hitsound_bell ), nullptr, SND_MEMORY | SND_ASYNC ); break;
	case 8: LI_FN ( PlaySoundA ) ( reinterpret_cast < const char* > ( hitsound_cod ), nullptr, SND_MEMORY | SND_ASYNC ); break;
	case 9: LI_FN ( PlaySoundA ) ( reinterpret_cast < const char* > ( hitsound_rattle ), nullptr, SND_MEMORY | SND_ASYNC ); break;
	case 10: LI_FN ( PlaySoundA ) ( reinterpret_cast < const char* > ( hitsound_ramune ), nullptr, SND_MEMORY | SND_ASYNC ); break;
	default: break;
	}

	rdata::player_dmg [ victim->idx ( ) ] = dmg;
	rdata::player_hurt [ victim->idx( ) ] = true;
	features::ragebot::get_hits( victim->idx( ) )++;

	hit_hitgroup [ victim->idx ( ) ] = hitgroup;

	/* hit wrong hitbox, probably due to resolver */
	//if ( hitgroup != autowall::hitbox_to_hitgroup ( features::ragebot::get_hitbox ( victim->idx ( ) ) ) ) {
	//	rdata::player_hurt [ victim->idx ( ) ] = false;
	//}
}

void animations::resolver::process_event_buffer( int pl_idx ) {
	OPTION ( bool, shot_logs, "Sesame->C->Other->World->Shot Logs", oxui::object_checkbox );
	OPTION ( bool, crash_when_miss, "Sesame->A->Default->Accuracy->Crash If Miss Due To Resolve", oxui::object_checkbox );

	if ( !g::local || !g::local->weapon( ) || !g::local->weapon( )->data( ) )
		return;

	auto ray_intersects_sphere = [ ] ( const vec3_t& from, const vec3_t& to, const vec3_t& sphere, float rad ) -> bool {
		auto q = sphere - from;
		auto v = q.dot_product ( csgo::angle_vec ( csgo::calc_angle ( from, to ) ).normalized ( ) );
		auto d = rad - ( q.length_sqr ( ) - v * v );

		return d >= FLT_EPSILON;
	};

	if ( rdata::last_shots [ pl_idx ] != features::ragebot::get_shots ( pl_idx ) && rdata::impacts [ pl_idx ] != vec3_t ( 0.0f, 0.0f, 0.0f ) && !rdata::player_hurt [ pl_idx ] ) {
		//const auto vec_to_extended = csgo::angle_vec ( csgo::calc_angle ( features::ragebot::get_shot_pos ( pl_idx ), rdata::impacts [ pl_idx ] ) ) * g::local->weapon ( )->data ( )->m_range;
		//const auto reached_without_obstruction = ray_intersects_sphere ( features::ragebot::get_shot_pos ( pl_idx ), features::ragebot::get_shot_pos ( pl_idx ) + vec_to_extended, features::ragebot::get_target_pos ( pl_idx ), 8.0f );

		/*if ( rdata::impact_dmg [ pl_idx ] <= 0.0f && reached_without_obstruction ) {
			features::ragebot::get_misses ( pl_idx ).occlusion++;
			impact_recs.push_back ( impact_rec_t { features::ragebot::get_shot_pos ( pl_idx ), rdata::impacts [ pl_idx ], _ ( L"occlusion ( " ) + std::to_wstring ( features::ragebot::get_misses ( pl_idx ).occlusion ) + _ ( L" )" ), csgo::i::globals->m_curtime, false, D3DCOLOR_RGBA ( 161, 66, 245, 150 ) } );
		}
		else*/ if ( rdata::impact_dmg [ pl_idx ] <= 0.0f ) {
			features::ragebot::get_misses ( pl_idx ).spread++;
			impact_recs.push_back ( impact_rec_t { features::ragebot::get_shot_pos ( pl_idx ), rdata::impacts [ pl_idx ], _ ( L"spread ( " ) + std::to_wstring ( features::ragebot::get_misses ( pl_idx ).spread ) + _ ( L" )" ), csgo::i::globals->m_curtime, false, D3DCOLOR_RGBA ( 161, 66, 245, 150 ) } );
			
			if ( shot_logs ) {
				print_console ( oxui::color ( 0xd8, 0x50, 0xd4, 255 ), _ ( "[ sesame ] " ) );
				print_console ( oxui::color ( 255, 255, 255, 255 ), _ ( "Missed shot due to spread ( hitchance: %d%, confidence: %d% )\n" ), static_cast< int >( features::ragebot::hitchances [ pl_idx ] ), static_cast< int >( get_confidence ( pl_idx ) ) );
				//print_console ( oxui::color ( 0xd8, 0x50, 0xd4, 255 ), ( char* ) _ ( u8"【ごま】" ) );
				//print_console ( oxui::color ( 255, 255, 255, 255 ), ( char* ) _ ( u8"不正確によるショットの失敗（%d）\n" ), features::ragebot::get_misses ( pl_idx ).spread );
			}
		}
		else {
			features::ragebot::get_misses ( pl_idx ).bad_resolve++;
			impact_recs.push_back ( impact_rec_t { features::ragebot::get_shot_pos ( pl_idx ), rdata::impacts [ pl_idx ], _ ( L"resolve ( " ) + std::to_wstring ( features::ragebot::get_misses ( pl_idx ).bad_resolve ) + _ ( L" )" ), csgo::i::globals->m_curtime, false, D3DCOLOR_RGBA ( 161, 66, 245, 150 ) } );
			
			/* missed the cached resolve? don't shoot at it again until we have it fixed */
			fix_cached_resolves ( pl_idx );

			if ( shot_logs ) {
				print_console ( oxui::color ( 0xd8, 0x50, 0xd4, 255 ), _ ( "[ sesame ] " ) );
				print_console ( oxui::color ( 255, 255, 255, 255 ), _ ( "Missed shot due to bad resolve ( hitchance: %d%, confidence: %d% )\n" ), static_cast< int >( features::ragebot::hitchances [ pl_idx ] ), static_cast< int >( get_confidence ( pl_idx ) ) );
				//print_console ( oxui::color ( 0xd8, 0x50, 0xd4, 255 ), (char*)_ ( u8"【ごま】" ) );
				//print_console ( oxui::color ( 255, 255, 255, 255 ), (char*)_ ( u8"悪いresolveによるショットの失敗（%d）\n" ), features::ragebot::get_misses ( pl_idx ).bad_resolve );
			}

			if ( crash_when_miss )
				abort ( );
		}

		rdata::impacts [ pl_idx ] = vec3_t ( 0.0f, 0.0f, 0.0f );
		rdata::player_hurt [ pl_idx ] = false;
		rdata::last_shots [ pl_idx ] = features::ragebot::get_shots ( pl_idx );
		features::ragebot::get_shot_pos ( pl_idx ) = vec3_t ( 0.0f, 0.0f, 0.0f );
		rdata::player_dmg [ pl_idx ] = 0;
		rdata::impact_dmg [ pl_idx ] = 0.0f;
		features::ragebot::get_target_idx ( ) = 0;
		features::ragebot::get_target ( ) = nullptr;
	}
	else if ( rdata::player_hurt [ pl_idx ] ) {
		hit_matrix_rec.push_back ( { uint32_t( pl_idx ), { }, csgo::i::globals->m_curtime, D3DCOLOR_RGBA ( 202, 252, 3, 255 ) } );

		if ( features::ragebot::get_misses ( pl_idx ).bad_resolve % 3 == 0 )
			std::memcpy ( &hit_matrix_rec.back ( ).m_bones, features::ragebot::get_lag_rec ( pl_idx ).m_bones1, sizeof matrix3x4_t * 128 );
		else if ( features::ragebot::get_misses ( pl_idx ).bad_resolve % 3 == 1 )
			std::memcpy ( &hit_matrix_rec.back ( ).m_bones, features::ragebot::get_lag_rec ( pl_idx ).m_bones2, sizeof matrix3x4_t * 128 );
		else
			std::memcpy ( &hit_matrix_rec.back ( ).m_bones, features::ragebot::get_lag_rec ( pl_idx ).m_bones3, sizeof matrix3x4_t * 128 );

		impact_recs.push_back ( impact_rec_t { features::ragebot::get_shot_pos ( pl_idx ), features::ragebot::get_target_pos( pl_idx ), std::to_wstring ( rdata::player_dmg [ pl_idx ] ), csgo::i::globals->m_curtime, true, D3DCOLOR_RGBA ( 161, 66, 245, 150 ) } );
		
		player_info_t pl_info;
		csgo::i::engine->get_player_info ( pl_idx, &pl_info );

		if ( shot_logs ) {
			print_console ( oxui::color ( 0xd8, 0x50, 0xd4, 255 ), _ ( "[ sesame ] " ) );
			print_console ( oxui::color ( 255, 255, 255, 255 ), _ ( "Hit %s in the " ), pl_info.m_name );
			
			switch ( hit_hitgroup [ pl_idx ] ) {
			case 0: print_console ( oxui::color ( 117, 255, 48, 255 ), _ ( "generic" ) ); break;
			case 1: print_console ( oxui::color ( 117, 255, 48, 255 ), _ ( "head" ) ); break;
			case 2: print_console ( oxui::color ( 117, 255, 48, 255 ), _ ( "chest" ) ); break;
			case 3: print_console ( oxui::color ( 117, 255, 48, 255 ), _ ( "stomach" ) ); break;
			case 4: print_console ( oxui::color ( 117, 255, 48, 255 ), _ ( "left arm" ) ); break;
			case 5: print_console ( oxui::color ( 117, 255, 48, 255 ), _ ( "right arm" ) ); break;
			case 6: print_console ( oxui::color ( 117, 255, 48, 255 ), _ ( "left leg" ) ); break;
			case 7: print_console ( oxui::color ( 117, 255, 48, 255 ), _ ( "right leg" ) ); break;
			case 10: print_console ( oxui::color ( 117, 255, 48, 255 ), _ ( "gear" ) ); break;
			default: print_console ( oxui::color ( 117, 255, 48, 255 ), _ ( "unknown" ) ); break;
			}
			
			print_console ( oxui::color ( 255, 255, 255, 255 ), _ ( " for " ) );
			print_console ( oxui::color ( 255, 86, 86, 255 ), _ ( "%d\n" ), rdata::player_dmg [ pl_idx ] );

			//print_console ( oxui::color ( 0xd8, 0x50, 0xd4, 255 ), (char*)_ ( u8"【ごま】" ) );
			//
			//switch ( hit_hitgroup [ pl_idx ] ) {
			//case 0: print_console ( oxui::color ( 117, 255, 48, 255 ), (char*) _ ( u8"ジェネリック" ) ); break;
			//case 1: print_console ( oxui::color ( 117, 255, 48, 255 ), (char*) _ ( u8"頭" ) ); break;
			//case 2: print_console ( oxui::color ( 117, 255, 48, 255 ), (char*) _ ( u8"胸" ) ); break;
			//case 3: print_console ( oxui::color ( 117, 255, 48, 255 ), (char*) _ ( u8"胃" ) ); break;
			//case 4: print_console ( oxui::color ( 117, 255, 48, 255 ), (char*) _ ( u8"左腕" ) ); break;
			//case 5: print_console ( oxui::color ( 117, 255, 48, 255 ), (char*) _ ( u8"右腕" ) ); break;
			//case 6: print_console ( oxui::color ( 117, 255, 48, 255 ), (char*) _ ( u8"左脚" ) ); break;
			//case 7: print_console ( oxui::color ( 117, 255, 48, 255 ), (char*) _ ( u8"右脚" ) ); break;
			//case 10: print_console ( oxui::color ( 117, 255, 48, 255 ), (char*)_ ( u8"装置" ) ); break;
			//default: print_console ( oxui::color ( 117, 255, 48, 255 ), (char*)_ ( u8"不明" ) ); break;
			//}
			//
			//print_console ( oxui::color ( 255, 255, 255, 255 ), ( char* ) _ ( u8"に%を打ちます（" ), pl_info.m_name );
			//print_console ( oxui::color ( 255, 86, 86, 255 ), ( char* ) _ ( u8"%d" ), rdata::player_dmg [ pl_idx ] );
			//print_console ( oxui::color ( 255, 255, 255, 255 ), ( char* ) _ ( u8"）\n" ) );
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
	}
}

bool animations::resolver::jittering( player_t* pl ) {
	return rdata::has_jitter [ pl->idx( ) ];
}

float animations::resolver::get_dmg ( player_t* pl, int side ) {
	if ( !side )
		return 0.0f;

	if ( side == 1 )
		return rdata::l_dmg [ pl->idx ( ) ];
	else if ( side == 2 )
		return rdata::r_dmg [ pl->idx ( ) ];

	return 0.0f;
}

void animations::resolver::update( ucmd_t* ucmd ) {
	if ( !g::local || !g::local->alive( ) )
		return;

	csgo::for_each_player( [ & ] ( player_t* pl ) {
		if ( pl == g::local || pl->team ( ) == g::local->team ( ) )
			return;

		const auto cross = csgo::angle_vec( csgo::calc_angle( pl->origin( ) + vec3_t( 0.0f, 0.0f, 64.0f ), g::local->origin( ) + vec3_t( 0.0f, 0.0f, 64.0f ) ) ).cross_product( vec3_t( 0.0f, 0.0f, 1.0f ) );

		const auto src = g::local->origin ( ) + vec3_t ( 0.0f, 0.0f, 64.0f );
		const auto dst = pl->origin ( ) + vec3_t ( 0.0f, 0.0f, 64.0f );
		const auto l_dmg = autowall::dmg ( g::local, pl, src + cross * 48.0f, dst + cross * 48.0f, 0 );
		const auto r_dmg = autowall::dmg ( g::local, pl, src - cross * 48.0f, dst - cross * 48.0f, 0 );

		if ( l_dmg != r_dmg ) {
			rdata::l_dmg [ pl->idx ( ) ] = l_dmg;
			rdata::r_dmg [ pl->idx ( ) ] = r_dmg;
		}
	} );
}

float calc_avg_yaw ( player_t* pl, float& delta_out ) {
	const auto recs = features::lagcomp::get_all ( pl );

	if ( recs.first.size( ) <= 2 )
		return pl->angles ( ).y;

	auto avg_delta = 0.0f;

	for ( auto& rec : recs.first )
		avg_delta += std::fabsf ( csgo::normalize( csgo::normalize( pl->angles ( ).y ) - csgo::normalize( rec.m_ang.y ) ) );

	avg_delta /= recs.first.size ( );
	delta_out = avg_delta;

	const auto sign = csgo::normalize ( csgo::normalize ( recs.first [ 0 ].m_ang.y ) - csgo::normalize ( recs.first [ 1 ].m_ang.y ) );

	return csgo::normalize( pl->angles ( ).y + std::copysignf( avg_delta * 0.5f, sign ) );
}

float approached_angle [ 65 ] { 0.0f };
float last_approached_angle [ 65 ] { 0.0f };
float last_approached_check [ 65 ] { 0.0f };
float last_vel_rate2 [ 65 ] { 0.0f };
int initial_fake_side [ 65 ] { 0 };
float last_correction_amount [ 65 ] { 0.0f };
float last_freestand_correction_amount [ 65 ] { 0.0f };
float last_predicted_particle_yaw [ 65 ] { FLT_MAX };
int last_missed_shots [ 65 ] { 0 };

float correction_stage_1 [ 65 ] { 0.0f };
float correction_stage_2 [ 65 ] { 0.0f };
float correction_stage_3 [ 65 ] { 0.0f };

void resolve_simple ( player_t* pl, float& yaw1, float& yaw2, float& yaw3 ) {
	auto correction_amount = 0.0f;
	auto desync_amount = pl->desync_amount ( );
	auto record_correction = true;
	auto freestanding_dir = desync_amount;

	auto avg_yaw_delta = 0.0f;
	const auto eye_feet_delta = csgo::normalize ( csgo::normalize ( pl->angles ( ).y ) - csgo::normalize ( pl->lby ( ) ) );
	const auto avg_yaw = calc_avg_yaw ( pl, avg_yaw_delta );
	const auto jitter_delta = csgo::normalize ( csgo::normalize ( pl->angles ( ).y ) - csgo::normalize ( avg_yaw ) );

	if ( animations::resolver::rdata::r_dmg [ pl->idx ( ) ] > animations::resolver::rdata::l_dmg [ pl->idx ( ) ] )
		freestanding_dir = desync_amount;
	else if ( animations::resolver::rdata::l_dmg [ pl->idx ( ) ] > animations::resolver::rdata::r_dmg [ pl->idx ( ) ] )
		freestanding_dir = -desync_amount;
	else
		freestanding_dir = -desync_amount;

	if ( pl->simtime ( ) != pl->old_simtime ( ) ) {
		if ( std::fabsf ( csgo::i::globals->m_curtime - animations::resolver::rdata::last_vel_check [ pl->idx ( ) ] ) > 0.5f ) {
			animations::resolver::rdata::last_vel_rate [ pl->idx ( ) ] = pl->vel ( ).length_2d ( );
			animations::resolver::rdata::last_vel_check [ pl->idx ( ) ] = csgo::i::globals->m_curtime;
		}
	}

	auto has_slow_walk = false;

	if ( pl->weapon ( ) && pl->weapon ( )->data ( ) )
		has_slow_walk = std::fabsf ( pl->vel ( ).length_2d ( ) - animations::resolver::rdata::last_vel_rate [ pl->idx ( ) ] ) < 10.0f && animations::resolver::rdata::last_vel_rate [ pl->idx ( ) ] > 20.0f && pl->vel ( ).length_2d ( ) > 20.0f && animations::resolver::rdata::last_vel_rate [ pl->idx ( ) ] < pl->weapon ( )->data ( )->m_max_speed * 0.33f && pl->vel ( ).length_2d ( ) < pl->weapon ( )->data ( )->m_max_speed * 0.33f;

	if ( std::fabsf ( avg_yaw_delta ) > pl->desync_amount() ) {
		auto switch_delta = ( ( jitter_delta > 0.0f ) ? desync_amount : -desync_amount ) * 2.0f;
		switch_delta = ( features::ragebot::get_misses ( pl->idx ( ) ).bad_resolve % 2 ) ? -switch_delta : switch_delta;
		correction_amount = switch_delta;

		yaw1 = csgo::normalize ( pl->angles ( ).y + switch_delta );
		yaw2 = csgo::normalize ( pl->angles ( ).y - switch_delta );
		yaw3 = csgo::normalize ( pl->angles ( ).y + switch_delta );

		record_correction = false;
	}
	else {
		//if ( has_slow_walk || std::fabsf( eye_feet_delta ) <= 35.0f ) {
		//	yaw1 = csgo::normalize ( pl->angles ( ).y + desync_amount * ( freestanding_dir > 0.0f ? 0.333f : -1.0f ) );
		//	yaw2 = csgo::normalize ( pl->angles ( ).y + desync_amount * ( freestanding_dir > 0.0f ? -1.0f : 0.33f ) );
		//	yaw3 = csgo::normalize ( pl->angles ( ).y - desync_amount * 0.5f );
		//}
		//else {
		//	yaw1 = csgo::normalize ( pl->angles ( ).y + std::copysignf( desync_amount, eye_feet_delta ) );
		//	yaw2 = csgo::normalize ( pl->angles ( ).y - desync_amount * 0.5f );
		//	yaw3 = csgo::normalize ( pl->angles ( ).y );
		//}

		/*if ( pl->crouch_amount ( ) > 0.66f ) {
			yaw1 = csgo::normalize ( pl->angles ( ).y + desync_amount * 1.66f );
			yaw2 = csgo::normalize ( pl->angles ( ).y - desync_amount * 1.66f );
			yaw3 = csgo::normalize ( pl->angles ( ).y );
		}
		else*/ {
			yaw1 = csgo::normalize ( pl->angles ( ).y + desync_amount * ( freestanding_dir > 0.0f ? 1.0f : -1.0f ) );
			yaw2 = csgo::normalize ( pl->angles ( ).y + desync_amount * ( freestanding_dir > 0.0f ? -1.0f : 1.0f ) );
			yaw3 = csgo::normalize ( pl->angles ( ).y - desync_amount * 0.5f );
		}
	}

	if ( last_missed_shots [ pl->idx ( ) ] == features::ragebot::get_misses ( pl->idx ( ) ).bad_resolve && last_predicted_particle_yaw [ pl->idx ( ) ] != FLT_MAX && !( std::fabsf ( avg_yaw_delta ) > pl->desync_amount ( ) ) ) {
		switch ( features::ragebot::get_misses ( pl->idx ( ) ).bad_resolve % 3 ) {
		case 0: yaw1 = csgo::normalize ( pl->angles ( ).y + last_predicted_particle_yaw [ pl->idx ( ) ] * desync_amount ); break;
		case 1: yaw2 = csgo::normalize ( pl->angles ( ).y + last_predicted_particle_yaw [ pl->idx ( ) ] * desync_amount ); break;
		case 2: yaw3 = csgo::normalize ( pl->angles ( ).y + last_predicted_particle_yaw [ pl->idx ( ) ] * desync_amount ); break;
		}
	
		record_correction = false;
	}

	///* if we hit them with the current resolve, cache it at current location so we can use it later on */
	//if ( animations::resolver::rdata::queued_hit [ pl->idx ( ) ] && record_correction ) {
	//	auto cached_resolve_exists = false;
	//
	//	/* replace cached resolve if current one exists */
	//	for ( auto& cached_resolve : cached_resolves [ pl->idx ( ) ] ) {
	//		if ( cached_resolve.within ( pl->idx ( ) ) ) {
	//			cached_resolve = { pl->origin ( ), correction_amount, 25.0f, 0 };
	//			cached_resolve_exists = true;
	//			break;
	//		}
	//	}
	//
	//	/* else add a new entry */
	//	if ( !cached_resolve_exists )
	//		cached_resolves [ pl->idx ( ) ].push_back ( { pl->origin ( ), correction_amount, 45.0f, 0 } );
	//}

	/* replace predicted data if we recieved new information from particle system shit */
	if ( received_new_particles [ pl->idx ( ) ] ) {
		last_predicted_particle_yaw [ pl->idx ( ) ] = particle_predicted_desync_offset [ pl->idx ( ) ];
		last_missed_shots [ pl->idx ( ) ] = features::ragebot::get_misses ( pl->idx ( ) ).bad_resolve;

		//for ( auto& cached_resolve : cached_resolves [ pl->idx ( ) ] ) {
		//	if ( cached_resolve.within ( pl->idx ( ) ) ) {
		//		cached_resolve = { pl->origin ( ), particle_predicted_desync_offset [ pl->idx ( ) ], 45.0f, 0 };
		//		break;
		//	}
		//}

		received_new_particles [ pl->idx ( ) ] = false;
	}

	//yaw1 = csgo::normalize( pl->angles ( ).y + correction_amount );
}

std::array< float, 65 > initial_pitch_time { 0.0f };

void animations::resolver::resolve( player_t* pl, float& yaw1, float& yaw2, float& yaw3 ) {
	OPTION ( bool, resolver, "Sesame->A->Default->Accuracy->Resolve Desync", oxui::object_checkbox );
	//OPTION ( bool, beta_resolver, "Sesame->A->Rage Aimbot->Accuracy->Beta Resolver", oxui::object_checkbox );

	security_handler::update ( );

	auto state = pl->animstate( );

	if ( !state || !pl->layers ( ) || !resolver || !pl->weapon ( ) /*|| !animations::data::choke[pl->idx()]*/ ) {
		yaw1 = pl->angles ( ).y;
		yaw2 = pl->angles ( ).y;
		yaw3 = pl->angles ( ).y;

		return;
	}

	player_info_t pl_info;
	csgo::i::engine->get_player_info ( pl->idx ( ), &pl_info );
	
	if ( pl_info.m_fake_player ) {
		yaw1 = pl->angles().y;
		yaw2 = pl->angles ( ).y;
		yaw3 = pl->angles ( ).y;
	
		resolver_confidence [ pl->idx ( ) ] = 100.0f;
		return;
	}

	last_origin [ pl->idx ( ) ] = pl->origin ( );
	last_angle [ pl->idx ( ) ] = pl->angles ( );

	//if ( rdata::spawn_times [ pl->idx ( ) ] != pl->spawn_time ( ) ) {
	//	features::ragebot::get_misses ( pl->idx ( ) ).bad_resolve = 0;
	//	rdata::tried_side [ pl->idx ( ) ] = 0;
	//	rdata::forced_side [ pl->idx ( ) ] = 0;
	//	rdata::spawn_times [ pl->idx ( ) ] = pl->spawn_time ( );
	//}

	//if ( features::ragebot::get_misses ( pl->idx ( ) ).bad_resolve > 4 ) {
	//	rdata::tried_side [ pl->idx ( ) ] = 0;
	//	rdata::forced_side [ pl->idx ( ) ] = 0;
	//	features::ragebot::get_misses ( pl->idx ( ) ).bad_resolve = 0;
	//}

	//if ( std::fabsf ( pl->angles ( ).x ) >= 60.0f )
	//	initial_pitch_time [ pl->idx ( ) ] = csgo::i::globals->m_curtime;
	
	if ( std::fabsf ( pl->angles ( ).x ) < 60.0f && std::fabsf( csgo::i::globals->m_curtime - initial_pitch_time [ pl->idx ( ) ] ) < 0.25f ) {
		yaw1 = last_recorded_resolve1 [ pl->idx ( ) ];
		yaw2 = last_recorded_resolve2 [ pl->idx ( ) ];
		yaw3 = last_recorded_resolve3 [ pl->idx ( ) ];
		return;
	}

	//if ( animations::data::choke [ pl->idx ( ) ] ) {
		/* attempt to resolve player with the data we have */
	//resolve_smart_v2 ( pl, yaw );
		resolve_simple ( pl, yaw1, yaw2, yaw3 );
	//}

		last_recorded_resolve1 [ pl->idx ( ) ] = yaw1;
		last_recorded_resolve2 [ pl->idx ( ) ] = yaw2;
		last_recorded_resolve3 [ pl->idx ( ) ] = yaw3;

	//last_recorded_resolve [ pl->idx ( ) ] = yaw;
	last_recorded_tick [ pl->idx ( ) ] = csgo::i::globals->m_tickcount;
	rdata::queued_hit [ pl->idx ( ) ] = false;
}

void animations::resolver::render_impacts ( ) {
	OPTION ( bool, bullet_tracers, "Sesame->C->Other->World->Bullet Tracers", oxui::object_checkbox );
	OPTION ( bool, bullet_impacts, "Sesame->C->Other->World->Bullet Impacts", oxui::object_checkbox );
	OPTION ( oxui::color, clr_bullet_tracer, "Sesame->C->Other->World->Bullet Tracer", oxui::object_colorpicker );
	OPTION ( oxui::color, clr_bullet_impact, "Sesame->C->Other->World->Bullet Impact", oxui::object_colorpicker );

	/* erase resolver data if we exit the game or connect to another server */
	if ( !csgo::i::engine->is_connected ( ) || !csgo::i::engine->is_in_game ( ) ) {
		for ( auto i = 0; i < 65; i++ ) {
			particle_predicted_desync_offset [ i ] = FLT_MAX;
			rdata::tried_side [ i ] = 0;
			rdata::forced_side [ i ] = 0;
			rdata::l_dmg [ i ] = 0.0f;
			rdata::r_dmg [ i ] = 0.0f;
			cached_resolves [ i ].clear ( );

			if ( !csgo::i::ent_list->get< player_t* > ( i ) || !csgo::i::ent_list->get< player_t* > ( i )->alive( ) )
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

	//if ( !hit_matrix_rec.empty ( ) && !hit_matrix )
	//	hit_matrix_rec.clear ( );
	//
	//if ( !impact_recs.empty ( ) && ( !bullet_tracers && !bullet_impacts ) )
	//	impact_recs.clear ( );

	//for ( auto i = 0; i < 65; i++ ) {
	//	const auto ent = csgo::i::ent_list->get< player_t* > ( i );
	//
	//	if ( !ent->valid ( ) )
	//		continue;
	//
	//	for ( auto& cached_resolve : cached_resolves [ i ] ) {
	//		if ( !cached_resolve.within ( i ) )
	//			continue;
	//
	//		vec3_t screen;
	//
	//		if ( csgo::render::world_to_screen ( screen, cached_resolve.m_origin ) ) {
	//			const auto correction_amount = std::to_wstring ( static_cast< int > ( cached_resolve.m_correction ) );
	//			render::dim dim;
	//			render::text_size ( features::esp::esp_font, correction_amount, dim );
	//			render::text ( screen.x - dim.w / 2, screen.y - dim.h / 2, D3DCOLOR_RGBA ( 255, 255, 255, 255 ), features::esp::esp_font, correction_amount, true );
	//			render::circle3d ( cached_resolve.m_origin, 90, 26, D3DCOLOR_RGBA ( 193, 108, 230, 255 ), true );
	//		}
	//	}
	//}

	auto calc_alpha = [ & ] ( float time, float fade_time, float base_alpha, bool add = false ) {
		const auto dormant_time = 10.0f;
		return static_cast< int >( ( std::clamp< float > ( dormant_time - ( std::clamp< float > ( add ? ( dormant_time - std::clamp< float > ( std::fabsf ( csgo::i::globals->m_curtime - time ), 0.0f, dormant_time ) ) : std::fabsf ( csgo::i::globals->m_curtime - time ), std::max< float > ( dormant_time - fade_time, 0.0f ), dormant_time ) ), 0.0f, fade_time ) / fade_time ) * base_alpha );
	};

	if ( !hit_matrix_rec.empty( ) ) {
		int cur_matrix = 0;

		for ( auto& hit_matrix : hit_matrix_rec ) {
			const auto pl = csgo::i::ent_list->get< player_t* > ( hit_matrix.m_pl );

			if ( !pl || !pl->alive( ) ) {
				hit_matrix_rec.erase ( hit_matrix_rec.begin ( ) + cur_matrix );
				continue;
			}

			oxui::visual_editor::settings_t* visuals;
			if ( !features::get_visuals ( pl, &visuals ) )
				continue;

			const auto alpha = calc_alpha ( hit_matrix.m_time, 2.0f, visuals->hit_matrix_picker->clr.a );

			if ( !alpha ) {
				hit_matrix_rec.erase ( hit_matrix_rec.begin ( ) + cur_matrix );
				continue;
			}

			hit_matrix.m_clr = D3DCOLOR_RGBA ( visuals->hit_matrix_picker->clr.r , visuals->hit_matrix_picker->clr.g, visuals->hit_matrix_picker->clr.b, alpha );

			cur_matrix++;
		}
	}

	if ( impact_recs.empty ( ) )
		return;

	int cur_ray = 0;

	for ( auto& impact : impact_recs ) {
		vec3_t scrn_src;
		vec3_t scrn_dst;

		const auto alpha = calc_alpha ( impact.m_time, 2.0f, clr_bullet_tracer.a );
		const auto alpha1 = calc_alpha ( impact.m_time, 2.0f, clr_bullet_impact.a );
		const auto alpha2 = calc_alpha ( impact.m_time, 2.0f, 255 );

		if ( !alpha && !alpha1 && !alpha2 ) {
			impact_recs.erase ( impact_recs.begin ( ) + cur_ray );
			continue;
		}

		csgo::render::world_to_screen ( scrn_src, impact.m_src );
		csgo::render::world_to_screen ( scrn_dst, impact.m_dst );

		//if ( !csgo::render::world_to_screen ( scrn_src, impact.m_src ) || !csgo::render::world_to_screen ( scrn_dst, impact.m_dst ) ) {
		//	cur_ray++;
		//	continue;
		//}

		impact.m_clr = D3DCOLOR_RGBA ( clr_bullet_tracer.r, clr_bullet_tracer.g, clr_bullet_tracer.b, alpha );

		if ( bullet_tracers ) {
			render::line ( scrn_src.x - 1, scrn_src.y, scrn_dst.x - 1, scrn_dst.y, impact.m_clr );
			render::line ( scrn_src.x + 1, scrn_src.y, scrn_dst.x + 1, scrn_dst.y, impact.m_clr );
			render::line ( scrn_src.x, scrn_src.y, scrn_dst.x, scrn_dst.y, impact.m_clr );
		}

		render::dim dim;
		render::text_size ( features::esp::esp_font, impact.m_msg, dim );

		if ( impact.m_hurt && bullet_impacts ) {
			render::text ( scrn_dst.x - dim.w / 2, scrn_dst.y - 16, D3DCOLOR_RGBA ( 145, 255, 0, alpha2 ), features::esp::esp_font, impact.m_msg, true, false );
			//render::cube ( impact.m_dst, 4, D3DCOLOR_RGBA ( clr_bullet_impact.r, clr_bullet_impact.g, clr_bullet_impact.b, alpha1 ) );

			render::line ( scrn_dst.x + 3, scrn_dst.y + 3, scrn_dst.x + 8, scrn_dst.y + 8, D3DCOLOR_RGBA ( clr_bullet_impact.r , clr_bullet_impact.g, clr_bullet_impact.b, alpha1 ) );
			render::line ( scrn_dst.x + 3, scrn_dst.y - 3, scrn_dst.x + 8, scrn_dst.y - 8, D3DCOLOR_RGBA ( clr_bullet_impact.r , clr_bullet_impact.g, clr_bullet_impact.b, alpha1 ) );
			render::line ( scrn_dst.x - 3, scrn_dst.y + 3, scrn_dst.x - 8, scrn_dst.y + 8, D3DCOLOR_RGBA ( clr_bullet_impact.r , clr_bullet_impact.g, clr_bullet_impact.b, alpha1 ) );
			render::line ( scrn_dst.x - 3, scrn_dst.y - 3, scrn_dst.x - 8, scrn_dst.y - 8, D3DCOLOR_RGBA ( clr_bullet_impact.r , clr_bullet_impact.g, clr_bullet_impact.b, alpha1 ) );

			render::line ( scrn_dst.x + 3, scrn_dst.y + 3 + 1, scrn_dst.x + 8, scrn_dst.y + 8 + 1, D3DCOLOR_RGBA ( 0, 0, 0, alpha1 ) );
			render::line ( scrn_dst.x + 3, scrn_dst.y - 3 + 1, scrn_dst.x + 8, scrn_dst.y - 8 + 1, D3DCOLOR_RGBA ( 0, 0, 0, alpha1 ) );
			render::line ( scrn_dst.x - 3, scrn_dst.y + 3 + 1, scrn_dst.x - 8, scrn_dst.y + 8 + 1, D3DCOLOR_RGBA ( 0, 0, 0, alpha1 ) );
			render::line ( scrn_dst.x - 3, scrn_dst.y - 3 + 1, scrn_dst.x - 8, scrn_dst.y - 8 + 1, D3DCOLOR_RGBA ( 0, 0, 0, alpha1 ) );
		}
		else {
			//render::text ( scrn_dst.x - dim.w / 2, scrn_dst.y - 16, D3DCOLOR_RGBA ( 255, 62, 59, alpha ), features::esp::esp_font, impact.m_msg, true, false );
		}

		cur_ray++;
	}
}