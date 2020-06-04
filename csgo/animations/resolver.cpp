#include <array>
#include "animations.hpp"
#include "resolver.hpp"
#include "../features/ragebot.hpp"
#include "../globals.hpp"
#include "../hooks.hpp"
#include "../security/security_handler.hpp"
#include "../features/lagcomp.hpp"
#include "../features/autowall.hpp"
#include "../renderer/d3d9.hpp"
#include "../menu/menu.hpp"
#include "../features/esp.hpp"
#include "../features/autowall.hpp"

struct impact_rec_t {
	vec3_t m_src, m_dst;
	std::wstring m_msg;
	float m_time;
	bool m_hurt;
	uint32_t m_clr;
};

std::deque< animations::resolver::hit_matrix_rec_t > hit_matrix_rec { };
std::deque< impact_rec_t > impact_recs { };

void animations::resolver::process_impact( event_t* event ) {
	auto shooter = csgo::i::ent_list->get< player_t* >( csgo::i::engine->get_player_for_userid( event->get_int( _( "userid" ) ) ) );
	auto& target = features::ragebot::get_target ( );

	if ( !g::local || !g::local->alive( ) || !target || target->dormant( ) || !shooter || shooter != g::local || features::ragebot::get_shot_pos ( target->idx ( ) ) == vec3_t( 0.0f, 0.0f, 0.0f ) || !g::local->weapon ( ) || !g::local->weapon ( )->data ( ) )
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
	std::memcpy ( target->bone_cache ( ), target_rec.m_bones, sizeof matrix3x4_t * target->bone_count ( ) );

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

std::array< float, 65 > last_recorded_resolve { FLT_MAX };
std::array< int, 65 > last_recorded_tick { 0 };

void animations::resolver::process_hurt( event_t* event ) {
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

	if ( !attacker || !victim || attacker != g::local || victim->team( ) == g::local->team( ) )
		return;

	csgo::i::engine->client_cmd_unrestricted( _( "play physics\\metal\\paintcan_impact_hard3" ) );

	rdata::player_dmg [ victim->idx ( ) ] = dmg;
	rdata::player_hurt [ victim->idx( ) ] = true;
	features::ragebot::get_hits( victim->idx( ) )++;

	/* hit wrong hitbox, probably due to resolver */
	//if ( hitgroup != autowall::hitbox_to_hitgroup ( features::ragebot::get_hitbox ( victim->idx ( ) ) ) ) {
	//	rdata::player_hurt [ victim->idx ( ) ] = false;
	//}
}

void animations::resolver::process_event_buffer( player_t* pl ) {
	if ( !g::local || !g::local->weapon( ) || !g::local->weapon( )->data( ) )
		return;

	auto ray_intersects_sphere = [ ] ( const vec3_t& from, const vec3_t& to, const vec3_t& sphere, float rad ) -> bool {
		auto q = sphere - from;
		auto v = q.dot_product ( csgo::angle_vec ( csgo::calc_angle ( from, to ) ).normalized ( ) );
		auto d = rad - ( q.length_sqr ( ) - v * v );

		return d >= FLT_EPSILON;
	};

	if ( rdata::last_shots [ pl->idx ( ) ] != features::ragebot::get_shots ( pl->idx ( ) ) && rdata::impacts [ pl->idx ( ) ] != vec3_t ( 0.0f, 0.0f, 0.0f ) && !rdata::player_hurt [ pl->idx ( ) ] ) {
		const auto vec_to_extended = csgo::angle_vec ( csgo::calc_angle ( features::ragebot::get_shot_pos ( pl->idx ( ) ), rdata::impacts [ pl->idx ( ) ] ) ) * g::local->weapon ( )->data ( )->m_range;
		const auto reached_without_obstruction = ray_intersects_sphere ( features::ragebot::get_shot_pos ( pl->idx ( ) ), features::ragebot::get_shot_pos ( pl->idx ( ) ) + vec_to_extended, features::ragebot::get_target_pos ( pl->idx ( ) ), 8.0f );

		/*if ( rdata::impact_dmg [ pl->idx ( ) ] <= 0.0f && reached_without_obstruction ) {
			features::ragebot::get_misses ( pl->idx ( ) ).occlusion++;
			impact_recs.push_back ( impact_rec_t { features::ragebot::get_shot_pos ( pl->idx ( ) ), rdata::impacts [ pl->idx ( ) ], _ ( L"occlusion ( " ) + std::to_wstring ( features::ragebot::get_misses ( pl->idx ( ) ).occlusion ) + _ ( L" )" ), csgo::i::globals->m_curtime, false, D3DCOLOR_RGBA ( 161, 66, 245, 150 ) } );
		}
		else*/ if ( rdata::impact_dmg [ pl->idx ( ) ] <= 0.0f ) {
			features::ragebot::get_misses ( pl->idx ( ) ).spread++;
			impact_recs.push_back ( impact_rec_t { features::ragebot::get_shot_pos ( pl->idx ( ) ), rdata::impacts [ pl->idx ( ) ], _ ( L"spread ( " ) + std::to_wstring ( features::ragebot::get_misses ( pl->idx ( ) ).spread ) + _ ( L" )" ), csgo::i::globals->m_curtime, false, D3DCOLOR_RGBA ( 161, 66, 245, 150 ) } );
		}
		else {
			features::ragebot::get_misses ( pl->idx ( ) ).bad_resolve++;
			impact_recs.push_back ( impact_rec_t { features::ragebot::get_shot_pos ( pl->idx ( ) ), rdata::impacts [ pl->idx ( ) ], _ ( L"resolve ( " ) + std::to_wstring ( features::ragebot::get_misses ( pl->idx ( ) ).bad_resolve ) + _ ( L" )" ), csgo::i::globals->m_curtime, false, D3DCOLOR_RGBA ( 161, 66, 245, 150 ) } );
		}

		rdata::impacts [ pl->idx ( ) ] = vec3_t ( 0.0f, 0.0f, 0.0f );
		rdata::player_hurt [ pl->idx( ) ] = false;
		rdata::last_shots [ pl->idx ( ) ] = features::ragebot::get_shots ( pl->idx ( ) );
		features::ragebot::get_shot_pos ( pl->idx ( ) ) = vec3_t ( 0.0f, 0.0f, 0.0f );
		rdata::player_dmg [ pl->idx ( ) ] = 0;
		rdata::impact_dmg [ pl->idx ( ) ] = 0.0f;
		features::ragebot::get_target_idx ( ) = 0;
		features::ragebot::get_target ( ) = nullptr;
	}
	else if ( rdata::player_hurt [ pl->idx ( ) ] ) {
		hit_matrix_rec.push_back ( { pl->idx ( ), { }, csgo::i::globals->m_curtime, D3DCOLOR_RGBA ( 202, 252, 3, 255 ) } );
		std::memcpy ( &hit_matrix_rec.back ( ).m_bones, features::ragebot::get_lag_rec ( pl->idx ( ) ).m_bones, sizeof features::ragebot::get_lag_rec ( pl->idx ( ) ).m_bones );

		impact_recs.push_back ( impact_rec_t { features::ragebot::get_shot_pos ( pl->idx ( ) ), features::ragebot::get_target_pos( pl->idx( ) ), std::to_wstring ( rdata::player_dmg [ pl->idx ( ) ] ), csgo::i::globals->m_curtime, true, D3DCOLOR_RGBA ( 161, 66, 245, 150 ) } );

		rdata::impacts [ pl->idx ( ) ] = vec3_t ( 0.0f, 0.0f, 0.0f );
		rdata::queued_hit [ pl->idx ( ) ] = true;
		rdata::player_hurt [ pl->idx( ) ] = false;
		rdata::last_shots [ pl->idx ( ) ] = features::ragebot::get_shots ( pl->idx ( ) );
		features::ragebot::get_shot_pos ( pl->idx ( ) ) = vec3_t ( 0.0f, 0.0f, 0.0f );
		rdata::player_dmg [ pl->idx ( ) ] = 0;
		rdata::impact_dmg [ pl->idx ( ) ] = 0.0f;
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
		const auto dst = pl->origin ( ) + pl->vel ( ) * ( csgo::i::globals->m_curtime - pl->simtime ( ) ) + vec3_t ( 0.0f, 0.0f, 64.0f );

		rdata::l_dmg [ pl->idx ( ) ] = autowall::dmg ( g::local, pl, src + cross * 32.0f, dst + cross * 32.0f, 0 );
		rdata::r_dmg [ pl->idx ( ) ] = autowall::dmg ( g::local, pl, src - cross * 32.0f, dst - cross * 32.0f, 0 );

		if ( ( rdata::l_dmg [ pl->idx ( ) ] == rdata::r_dmg [ pl->idx ( ) ] ) || ( rdata::l_dmg [ pl->idx ( ) ] == 0.0f && rdata::r_dmg [ pl->idx ( ) ] == 0.0f ) ) {
			rdata::l_dmg [ pl->idx ( ) ] = 0.0f;
			rdata::r_dmg [ pl->idx ( ) ] = 100.0f;
		}
	} );
}

void animations::resolver::resolve_edge ( player_t* pl, float& yaw ) {
	auto fake = yaw;

	auto head_visible = false;

	if ( pl->bone_cache ( ) )
		head_visible = csgo::is_visible ( pl->bone_cache ( ) [ 8 ].origin ( ) );

	//const auto dmg_left = csgo::is_visible( pl->origin( ) + vec3_t( 0.0f, 0.0f, 64.0f ) + cross * 45.0f );
	//const auto dmg_right = csgo::is_visible( pl->origin( ) + vec3_t( 0.0f, 0.0f, 64.0f ) - cross * 45.0f );

	if ( /*rdata::has_jitter [ pl->idx ( ) ]*/false ) {
		//if ( rdata::r_dmg [ pl->idx( ) ] > rdata::l_dmg [ pl->idx( ) ] || rdata::forced_side [ pl->idx( ) ] == 2 ) {
		//	if ( rdata::player_hurt [ pl->idx( ) ] )
		//		rdata::forced_side [ pl->idx( ) ] = 2;
		//
		//	fake += ( head_visible ? ( 180.0f + pl->desync_amount( ) ) : -pl->desync_amount( ) );
		//}
		//else if ( rdata::l_dmg [ pl->idx( ) ] > rdata::r_dmg [ pl->idx( ) ] || rdata::forced_side [ pl->idx( ) ] == 1 ) {
		//	if ( rdata::player_hurt [ pl->idx( ) ] )
		//		rdata::forced_side [ pl->idx( ) ] = 1;
		//
		//	fake += ( head_visible ? ( 180.0f - pl->desync_amount( ) ) : pl->desync_amount( ) );
		//}
		//else {
		//	rdata::forced_side [ pl->idx( ) ] = 0;
		//	fake += ( ( features::ragebot::get_misses( pl->idx( ) ).bad_resolve + 1 ) % 2 ) ? 180.0f : 0.0f;
		//}


		fake += ( ( rdata::flip_jitter [ pl->idx ( ) ] ) ? -pl->desync_amount ( ) : pl->desync_amount ( ) ) * 1.7f;

		if ( rdata::old_tc [ pl->idx ( ) ] != csgo::time2ticks ( pl->simtime ( ) ) )
			rdata::flip_jitter [ pl->idx ( ) ] = !rdata::flip_jitter [ pl->idx ( ) ];

		rdata::old_tc [ pl->idx ( ) ] = csgo::time2ticks ( pl->simtime ( ) );
	}
	else {
		if ( ( ( rdata::r_dmg [ pl->idx ( ) ] > rdata::l_dmg [ pl->idx ( ) ] && rdata::forced_side [ pl->idx ( ) ] != 2 ) || rdata::forced_side [ pl->idx ( ) ] == 2 ) && !features::ragebot::get_misses ( pl->idx ( ) ).bad_resolve ) {
			if ( rdata::queued_hit [ pl->idx ( ) ] )
				rdata::forced_side [ pl->idx ( ) ] = 2;

			fake += pl->desync_amount ( );
		}
		else if ( ( ( rdata::l_dmg [ pl->idx ( ) ] > rdata::r_dmg [ pl->idx ( ) ] && rdata::forced_side [ pl->idx ( ) ] != 1 ) || rdata::forced_side [ pl->idx ( ) ] == 1 ) && !features::ragebot::get_misses ( pl->idx ( ) ).bad_resolve ) {
			if ( rdata::queued_hit [ pl->idx ( ) ] )
				rdata::forced_side [ pl->idx ( ) ] = 1;

			fake -= pl->desync_amount ( );
		}
		else {
			fake += ( ( features::ragebot::get_misses ( pl->idx ( ) ).bad_resolve + rdata::forced_side [ pl->idx ( ) ] ) % 2 ) ? -pl->desync_amount ( ) : pl->desync_amount ( );
		}
	}

	yaw = csgo::normalize ( fake );
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

void animations::resolver::resolve_smart ( player_t* pl, float& yaw ) {
	auto state = pl->animstate ( );

	if ( !state || !pl->layers ( ) || !pl->weapon ( ) || !pl->weapon ( )->data ( ) )
		return;

	auto max_delta1 = csgo::rad2deg ( std::atan2f ( std::sinf ( csgo::deg2rad ( csgo::normalize( pl->angles ( ).y - pl->lby ( ) ) ) ), std::cosf ( csgo::deg2rad ( csgo::normalize ( pl->angles ( ).y - pl->lby ( ) ) ) ) ) );

	auto ang_body = yaw;
	auto max_delta = csgo::rad2deg ( std::atan2f ( std::sinf ( csgo::deg2rad ( csgo::normalize ( pl->angles ( ).y - pl->lby ( ) ) ) ), std::cosf ( csgo::deg2rad ( csgo::normalize ( pl->angles ( ).y - pl->lby ( ) ) ) ) ) );
	auto pitch_flick = false;

	if ( pl->simtime( ) != pl->old_simtime( ) ) {
		auto avg_yaw_delta = 0.0f;
		const auto avg_yaw = calc_avg_yaw ( pl, avg_yaw_delta );
		const auto jitter_delta = csgo::rad2deg ( std::atan2f ( std::sinf ( csgo::deg2rad ( pl->angles ( ).y - avg_yaw ) ), std::cosf ( csgo::deg2rad ( pl->angles ( ).y - avg_yaw ) ) ) );

		flags::eye_delta [ pl->idx ( ) ] = max_delta;

		flags::has_slow_walk [ pl->idx ( ) ] = pl->flags ( ) & 1 && pl->vel ( ).length_2d ( ) > 10.0f && std::fabsf ( pl->vel ( ).length_2d ( ) - rdata::last_vel_rate [ pl->idx ( ) ] ) < 10.0f && pl->vel ( ).length_2d ( ) <= pl->weapon ( )->data ( )->m_max_speed * 0.34f && rdata::last_vel_rate [ pl->idx ( ) ] <= pl->weapon ( )->data ( )->m_max_speed * 0.34f;
		flags::has_micro_movements [ pl->idx ( ) ] = pl->vel ( ).length_2d ( ) > 0.1f && pl->flags ( ) & 1 && pl->vel ( ).length_2d ( ) < 3.0f;
		const auto has_air_desync = !( pl->flags ( ) & 1 );
		flags::has_desync [ pl->idx ( ) ] = pl->vel ( ).length_2d ( ) > 0.1f ? has_air_desync || flags::has_micro_movements [ pl->idx ( ) ] || flags::has_slow_walk [ pl->idx ( ) ] : std::fabsf ( max_delta ) >= 60;
		
		flags::has_jitter [ pl->idx ( ) ] = std::fabsf( jitter_delta ) > 60.0f;

		rdata::last_abs_delta [ pl->idx ( ) ] = jitter_delta;

		pitch_flick = std::fabsf ( flags::last_pitch [ pl->idx ( ) ] ) < 45.0f && std::fabsf ( pl->angles ( ).x ) > 70.0f;

		flags::last_pitch [ pl->idx ( ) ] = pl->angles ( ).x;

		if ( std::fabsf ( csgo::i::globals->m_curtime - rdata::last_vel_check [ pl->idx ( ) ] ) > 0.5f ) {
			rdata::last_vel_rate [ pl->idx ( ) ] = pl->vel ( ).length_2d ( );
			rdata::last_vel_check [ pl->idx ( ) ] = csgo::i::globals->m_curtime;
		}
	}

	auto brute_center = [ & ] ( float& ang_body ) {
		const auto yaw1 = csgo::calc_angle ( g::local->eyes ( ), pl->eyes ( ) ).y;
		const auto max_delta5 = csgo::rad2deg ( std::atan2f ( std::sinf ( csgo::deg2rad ( csgo::normalize ( yaw1 - pl->abs_angles ( ).y ) ) ), std::cosf ( csgo::deg2rad ( csgo::normalize ( yaw1 - pl->abs_angles ( ).y ) ) ) ) );

		//const auto yaw = csgo::calc_angle (  ).y;
		//
		if ( std::fabsf( max_delta5 ) > 20.0f && std::fabsf ( max_delta5 ) < 60.0f && !features::ragebot::get_misses ( pl->idx ( ) ).bad_resolve ) {
			if ( max_delta5 > 0.0f ) {
				rdata::tried_side [ pl->idx ( ) ] = 2;
				ang_body += pl->desync_amount ( );
			}
			else {
				rdata::tried_side [ pl->idx ( ) ] = 1;
				ang_body -= pl->desync_amount ( );
			}
		}
		/*else */if ( ( rdata::r_dmg [ pl->idx ( ) ] > rdata::l_dmg [ pl->idx ( ) ] || rdata::forced_side [ pl->idx ( ) ] == 2 ) && !features::ragebot::get_misses ( pl->idx ( ) ).bad_resolve ) {
			rdata::tried_side [ pl->idx ( ) ] = 2;
			ang_body += pl->desync_amount ( );
		}
		else if ( ( rdata::l_dmg [ pl->idx ( ) ] > rdata::r_dmg [ pl->idx ( ) ] || rdata::forced_side [ pl->idx ( ) ] == 1 ) && !features::ragebot::get_misses ( pl->idx ( ) ).bad_resolve ) {
			rdata::tried_side [ pl->idx ( ) ] = 1;
			ang_body -= pl->desync_amount ( );
		}
		else {
			if ( rdata::tried_side [ pl->idx ( ) ] == 1 || !rdata::tried_side [ pl->idx ( ) ] )
				ang_body += ( ( features::ragebot::get_misses ( pl->idx ( ) ).bad_resolve % 2 ) ? pl->desync_amount ( ) : -pl->desync_amount ( ) );
			else if ( rdata::tried_side [ pl->idx ( ) ] == 2 )
				ang_body += ( ( features::ragebot::get_misses ( pl->idx ( ) ).bad_resolve % 2 ) ? -pl->desync_amount ( ) : pl->desync_amount ( ) );
		}
	};

	auto avg_yaw_delta = 0.0f;
	const auto avg_yaw = calc_avg_yaw ( pl, avg_yaw_delta );
	const auto avg_delta = csgo::rad2deg ( std::atan2f ( std::sinf ( csgo::deg2rad ( csgo::normalize( pl->angles ( ).y - avg_yaw ) ) ), std::cosf ( csgo::deg2rad ( csgo::normalize( pl->angles ( ).y - avg_yaw ) ) ) ) );

	//max_delta1 = csgo::normalize( csgo::normalize ( pl->angles ( ).y ) - csgo::normalize ( pl->lby ( ) ) );

	/* using jitter aa */
	/*if ( std::fabsf( avg_delta ) > 50.0f ) {
		//auto change_delta = ( std::fabsf ( max_delta1 ) > pl->desync_amount ( ) ) ? pl->desync_amount ( ) : ( pl->desync_amount ( ) * 0.5f );
		//
		//if ( features::ragebot::get_misses ( pl->idx ( ) ).bad_resolve % 2 )
		//	change_delta = ( std::fabsf ( max_delta1 ) > pl->desync_amount ( ) ) ? ( pl->desync_amount ( ) * 0.5f ) : pl->desync_amount ( );

		ang_body += ( avg_delta > 0.0f ) ? pl->desync_amount ( ) : -pl->desync_amount ( );
	}*/
	/* resolve slow walk and no visible abs delta */
	//dbg_print ( "%f\n", ( pl->poses ( ) [ 11 ] - 0.5f ) * 60.0f );

	const auto delta = ( pl->poses ( ) [ 11 ] - 0.5f ) * 60.0f;
	
	if ( std::fabsf ( max_delta1 ) <= 60.0f || flags::has_slow_walk [ pl->idx ( ) ] || features::ragebot::get_misses ( pl->idx ( ) ).bad_resolve > 2 ) {
		brute_center ( ang_body );
	}
	/* resolve positive abs-eye delta */
	else if ( max_delta1 > 0.0f ) {
		/* save hit yaw if they start slowwalking, but fake abs yaw will aproach eye yaw */
		if ( rdata::queued_hit [ pl->idx ( ) ] )
			rdata::forced_side [ pl->idx ( ) ] = 2;
	
		rdata::tried_side [ pl->idx ( ) ] = 2;
		ang_body += ( features::ragebot::get_misses ( pl->idx ( ) ).bad_resolve % 2 ) ? pl->desync_amount ( ) : ( pl->desync_amount ( ) * 0.5f );
	}
	/* resolve negative abs-eye delta */
	else {
		/* save hit yaw if they start slowwalking, but fake abs yaw will aproach eye yaw */
		if ( rdata::queued_hit [ pl->idx ( ) ] ) 
			rdata::forced_side [ pl->idx ( ) ] = 1;
	
		rdata::tried_side [ pl->idx ( ) ] = 1;
		ang_body -= ( features::ragebot::get_misses ( pl->idx ( ) ).bad_resolve % 2 ) ? pl->desync_amount ( ) : ( pl->desync_amount ( ) * 0.5f );
	}

	yaw = csgo::normalize( ang_body );
}

float approached_angle [ 65 ] { 0.0f };
float last_approached_angle [ 65 ] { 0.0f };
float last_approached_check [ 65 ] { 0.0f };
float last_vel_rate2 [ 65 ] { 0.0f };
int initial_fake_side [ 65 ] { 0 };

void animations::resolver::resolve_smart_v2 ( player_t* pl, float& yaw ) {
	auto avg_yaw_delta = 0.0f;

	const auto in_shot = animations::data::overlays [ pl->idx ( ) ][ 1 ].m_weight < 0.1f && pl->weapon ( )->last_shot_time ( ) > pl->old_simtime ( );
	const auto using_micro_movements = animations::data::overlays [ pl->idx ( ) ][ 6 ].m_weight < 0.1f && pl->vel ( ).length_2d ( ) > 0.0f && pl->vel ( ).length_2d ( ) < 5.0f;
	const auto eye_feet_delta = csgo::normalize ( csgo::normalize ( pl->angles ( ).y ) - csgo::normalize ( pl->abs_angles ( ).y ) );
	const auto avg_yaw = calc_avg_yaw ( pl, avg_yaw_delta );
	const auto jitter_delta = csgo::normalize ( csgo::normalize ( pl->angles ( ).y ) - csgo::normalize ( avg_yaw ) );
	const auto is_legit = std::fabsf ( pl->angles ( ).x ) < 45.0f;
	const auto has_legit_aa = is_legit && std::fabsf ( eye_feet_delta ) < 35.0f;

	if ( pl->simtime ( ) != pl->old_simtime ( ) ) {
		if ( std::fabsf ( csgo::i::globals->m_curtime - rdata::last_vel_check [ pl->idx ( ) ] ) > 0.5f ) {
			rdata::last_vel_rate [ pl->idx ( ) ] = pl->vel ( ).length_2d ( );
			rdata::last_vel_check [ pl->idx ( ) ] = csgo::i::globals->m_curtime;
		}

		if ( std::fabsf ( csgo::i::globals->m_curtime - last_approached_check [ pl->idx ( ) ] ) > 0.2f ) {
			if ( last_vel_rate2 [ pl->idx ( ) ] > 30.0f && pl->vel ( ).length_2d ( ) > 5.0f ) {
				last_approached_angle [ pl->idx ( ) ] = pl->angles ( ).y;
				initial_fake_side [ pl->idx ( ) ] = -1;
			}

			if ( last_vel_rate2 [ pl->idx ( ) ] > 10.0f && pl->vel ( ).length_2d ( ) < 5.0f && std::fabsf ( eye_feet_delta ) > 45.0f ) {
				approached_angle [ pl->idx ( ) ] = pl->angles ( ).y;
				initial_fake_side [ pl->idx ( ) ] = ( csgo::normalize ( csgo::normalize ( last_approached_angle [ pl->idx ( ) ] ) - csgo::normalize ( approached_angle [ pl->idx ( ) ] ) ) < 0.0f ) ? 0 : 1;
			}

			last_vel_rate2 [ pl->idx ( ) ] = pl->vel ( ).length_2d ( );
			last_approached_check [ pl->idx ( ) ] = csgo::i::globals->m_curtime;
		}
	}

	auto has_slow_walk = false;

	if ( pl->weapon ( ) && pl->weapon ( )->data ( ) )
		has_slow_walk = std::fabsf ( pl->vel ( ).length_2d ( ) - rdata::last_vel_rate [ pl->idx ( ) ] ) < 10.0f && rdata::last_vel_rate [ pl->idx ( ) ] > 20.0f && pl->vel ( ).length_2d ( ) > 20.0f && rdata::last_vel_rate [ pl->idx ( ) ] < pl->weapon ( )->data ( )->m_max_speed * 0.33f && pl->vel ( ).length_2d ( ) < pl->weapon ( )->data ( )->m_max_speed * 0.33f;

	/* jitter resolver */
	//if ( avg_yaw_delta > 75.0f /*&& !is_legit*/ ) {
	//	auto switch_delta = ( ( jitter_delta > 0.0f ) ? pl->desync_amount ( ) : -pl->desync_amount ( ) ) * 2.0f;
	//	switch_delta = ( features::ragebot::get_misses ( pl->idx ( ) ).bad_resolve % 2 ) ? -switch_delta : switch_delta;
	//	yaw += switch_delta;
	//}
	/*else*/ {
		const auto at_target = csgo::calc_angle ( g::local->origin ( ), pl->origin ( ) ).y;
		const auto is_sideways = std::fabsf( csgo::normalize ( csgo::normalize ( at_target ) - csgo::normalize ( pl->angles ( ).y ) ) ) > 75.0f;

		if ( is_sideways ) {
			/* most people use a positive body yaw when they use a sideways or manual antiaim */
			yaw += ( features::ragebot::get_misses ( pl->idx ( ) ).bad_resolve % 2 ) ? -pl->desync_amount ( ) : pl->desync_amount ( );
		}
		else {
			if ( ( /*using_micro_movements || has_slow_walk ||*/ std::fabsf ( eye_feet_delta ) < 45.0f ) /*&& !is_legit*/ ) {
				if ( features::ragebot::get_misses ( pl->idx ( ) ).bad_resolve < 1 && initial_fake_side [ pl->idx ( ) ] != -1 ) {
					rdata::tried_side [ pl->idx ( ) ] = initial_fake_side [ pl->idx ( ) ] + 1;
					yaw += initial_fake_side [ pl->idx ( ) ] ? pl->desync_amount ( ) : -pl->desync_amount ( );
				}
				else if ( ( rdata::r_dmg [ pl->idx ( ) ] > rdata::l_dmg [ pl->idx ( ) ] || rdata::forced_side [ pl->idx ( ) ] == 2 ) && features::ragebot::get_misses ( pl->idx ( ) ).bad_resolve < 1 ) {
					rdata::tried_side [ pl->idx ( ) ] = 2;
					yaw += pl->desync_amount ( );
				}
				else if ( ( rdata::l_dmg [ pl->idx ( ) ] > rdata::r_dmg [ pl->idx ( ) ] || rdata::forced_side [ pl->idx ( ) ] == 1 ) && features::ragebot::get_misses ( pl->idx ( ) ).bad_resolve < 1 ) {
					rdata::tried_side [ pl->idx ( ) ] = 1;
					yaw -= pl->desync_amount ( );
				}
				else {
					if ( rdata::tried_side [ pl->idx ( ) ] == 1 || !rdata::tried_side [ pl->idx ( ) ] )
						yaw += ( features::ragebot::get_misses ( pl->idx ( ) ).bad_resolve % 2 ) ? pl->desync_amount ( ) : -pl->desync_amount ( );
					else if ( rdata::tried_side [ pl->idx ( ) ] == 2 )
						yaw += ( features::ragebot::get_misses ( pl->idx ( ) ).bad_resolve % 2 ) ? -pl->desync_amount ( ) : pl->desync_amount ( );
				}
			}
			/*else if ( has_legit_aa ) {
				yaw += ( eye_feet_delta > 0.0f ) ? pl->desync_amount ( ) : -pl->desync_amount ( );
			}*/
			else /*if ( !is_legit )*/ {
				const auto yaw1 = csgo::calc_angle ( g::local->origin ( ), pl->origin ( ) ).y;
				const auto max_delta5 = std::fabsf ( csgo::normalize ( csgo::normalize ( yaw1 ) - csgo::normalize ( pl->abs_angles ( ).y ) ) );

				if ( features::ragebot::get_misses ( pl->idx ( ) ).bad_resolve < 1 && initial_fake_side [ pl->idx ( ) ] != -1 ) {
					rdata::tried_side [ pl->idx ( ) ] = initial_fake_side [ pl->idx ( ) ] + 1;
					yaw += initial_fake_side [ pl->idx ( ) ] ? pl->desync_amount ( ) : -pl->desync_amount ( );
				}
				else if ( max_delta5 > 0.0f && features::ragebot::get_misses ( pl->idx ( ) ).bad_resolve < 2 ) {
					rdata::tried_side [ pl->idx ( ) ] = 2;
					yaw += ( features::ragebot::get_misses ( pl->idx ( ) ).bad_resolve < 1 ) ? pl->desync_amount ( ) : ( pl->desync_amount ( ) * 0.5f );
				}
				else if ( max_delta5 < 0.0f && features::ragebot::get_misses ( pl->idx ( ) ).bad_resolve < 2 ) {
					rdata::tried_side [ pl->idx ( ) ] = 1;
					yaw -= ( features::ragebot::get_misses ( pl->idx ( ) ).bad_resolve < 1 ) ? pl->desync_amount ( ) : ( pl->desync_amount ( ) * 0.5f );
				}
				else {
					if ( rdata::tried_side [ pl->idx ( ) ] == 2 )
						yaw += ( features::ragebot::get_misses ( pl->idx ( ) ).bad_resolve % 2 ) ? pl->desync_amount ( ) : -pl->desync_amount ( );
					else if ( rdata::tried_side [ pl->idx ( ) ] == 1 || !rdata::tried_side [ pl->idx ( ) ] )
						yaw += ( features::ragebot::get_misses ( pl->idx ( ) ).bad_resolve % 2 ) ? -pl->desync_amount ( ) : pl->desync_amount ( );
				}
			}
		}
	}
}

void animations::resolver::resolve( player_t* pl, float& yaw ) {
	OPTION ( bool, resolver, "Sesame->A->Rage Aimbot->Accuracy->Resolve Desync", oxui::object_checkbox );
	OPTION ( bool, beta_resolver, "Sesame->A->Rage Aimbot->Accuracy->Beta Resolver", oxui::object_checkbox );

	security_handler::update ( );

	auto state = pl->animstate( );

	if ( !state || !pl->layers ( ) || !resolver || !pl->weapon ( ) ) {
		last_recorded_resolve [ pl->idx ( ) ] = FLT_MAX;
		rdata::player_hurt [ pl->idx( ) ] = false;
		rdata::queued_hit [ pl->idx ( ) ] = false;
		rdata::tried_side [ pl->idx ( ) ] = 0;
		rdata::forced_side [ pl->idx ( ) ] = 0;
		features::ragebot::get_misses ( pl->idx ( ) ).bad_resolve = 0;
		return;
	}

	player_info_t pl_info;
	csgo::i::engine->get_player_info ( pl->idx ( ), &pl_info );
	
	if ( pl_info.m_fake_player )
		return;

	if ( last_recorded_resolve [ pl->idx ( ) ] == FLT_MAX )
		last_recorded_resolve [ pl->idx ( ) ] = pl->angles ( ).y;

	//if ( rdata::spawn_times [ pl->idx ( ) ] != pl->spawn_time ( ) ) {
	//	features::ragebot::get_misses ( pl->idx ( ) ).bad_resolve = 0;
	//	rdata::tried_side [ pl->idx ( ) ] = 0;
	//	rdata::forced_side [ pl->idx ( ) ] = 0;
	//	rdata::spawn_times [ pl->idx ( ) ] = pl->spawn_time ( );
	//}

	if ( features::ragebot::get_misses ( pl->idx ( ) ).bad_resolve > 4 ) {
		rdata::tried_side [ pl->idx ( ) ] = 0;
		rdata::forced_side [ pl->idx ( ) ] = 0;
		features::ragebot::get_misses ( pl->idx ( ) ).bad_resolve = 0;
	}

	if ( last_recorded_tick [ pl->idx ( ) ] == csgo::i::globals->m_tickcount || std::fabsf ( pl->angles ( ).x ) < 60.0f ) {
		yaw = last_recorded_resolve [ pl->idx ( ) ];
		return;
	}

	if ( beta_resolver ) {
		resolve_smart_v2 ( pl, yaw );
	}
	else {
		resolve_smart ( pl, yaw );
	}

	last_recorded_resolve [ pl->idx ( ) ] = yaw;
	last_recorded_tick [ pl->idx ( ) ] = csgo::i::globals->m_tickcount;
	rdata::queued_hit [ pl->idx ( ) ] = false;
}

void animations::resolver::render_impacts ( ) {
	if ( !g::local || !g::local->alive ( ) ) {
		if ( !impact_recs.empty ( ) )
			impact_recs.clear ( );

		if ( !hit_matrix_rec.empty ( ) )
			hit_matrix_rec.clear ( );

		return;
	}

	auto calc_alpha = [ & ] ( float time, float fade_time, bool add = false ) {
		const auto dormant_time = 10.0f;
		return static_cast< int >( ( std::clamp< float > ( dormant_time - ( std::clamp< float > ( add ? ( dormant_time - std::clamp< float > ( std::fabsf ( csgo::i::globals->m_curtime - time ), 0.0f, dormant_time ) ) : std::fabsf ( csgo::i::globals->m_curtime - time ), std::max< float > ( dormant_time - fade_time, 0.0f ), dormant_time ) ), 0.0f, fade_time ) / fade_time ) * 255.0f );
	};

	if ( !hit_matrix_rec.empty( ) ) {
		int cur_matrix = 0;

		for ( auto& hit_matrix : hit_matrix_rec ) {
			const auto pl = csgo::i::ent_list->get< player_t* > ( hit_matrix.m_pl );

			if ( !pl || !pl->alive( ) ) {
				hit_matrix_rec.erase ( hit_matrix_rec.begin ( ) + cur_matrix );
				continue;
			}

			const auto alpha = calc_alpha ( hit_matrix.m_time, 2.0f );

			if ( !alpha ) {
				hit_matrix_rec.erase ( hit_matrix_rec.begin ( ) + cur_matrix );
				continue;
			}

			hit_matrix.m_clr = D3DCOLOR_RGBA ( ( hit_matrix.m_clr >> 16 ) & 0xff, ( hit_matrix.m_clr >> 8 ) & 0xff, ( hit_matrix.m_clr ) & 0xff, alpha );

			cur_matrix++;
		}
	}

	if ( impact_recs.empty ( ) )
		return;

	int cur_ray = 0;

	for ( auto& impact : impact_recs ) {
		vec3_t scrn_src;
		vec3_t scrn_dst;

		const auto alpha = calc_alpha ( impact.m_time, 2.0f );

		if ( !alpha ) {
			impact_recs.erase ( impact_recs.begin ( ) + cur_ray );
			continue;
		}

		if ( !csgo::render::world_to_screen ( scrn_src, impact.m_src ) || !csgo::render::world_to_screen ( scrn_dst, impact.m_dst ) ) {
			cur_ray++;
			continue;
		}

		impact.m_clr = D3DCOLOR_RGBA ( ( impact.m_clr >> 16 ) & 0xff,  ( impact.m_clr >> 8 ) & 0xff, ( impact.m_clr ) & 0xff, alpha );

		render::line ( scrn_src.x - 1, scrn_src.y, scrn_dst.x, scrn_dst.y, impact.m_clr );
		render::line ( scrn_src.x + 1, scrn_src.y, scrn_dst.x, scrn_dst.y, impact.m_clr );
		render::line ( scrn_src.x, scrn_src.y, scrn_dst.x, scrn_dst.y, impact.m_clr );

		render::dim dim;
		render::text_size ( features::esp::esp_font, impact.m_msg, dim );

		if ( impact.m_hurt ) {
			render::text ( scrn_dst.x - dim.w / 2, scrn_dst.y - 16, D3DCOLOR_RGBA ( 145, 255, 0, alpha ), features::esp::esp_font, impact.m_msg, true, false );
			render::cube ( impact.m_dst, 4, D3DCOLOR_RGBA ( 201, 145, 250, alpha ) );
		}
		else {
			render::text ( scrn_dst.x - dim.w / 2, scrn_dst.y - 16, D3DCOLOR_RGBA ( 255, 62, 59, alpha ), features::esp::esp_font, impact.m_msg, true, false );
		}

		cur_ray++;
	}
}