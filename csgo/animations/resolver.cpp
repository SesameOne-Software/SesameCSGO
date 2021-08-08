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
#include "../features/chams.hpp"

#include "anims.hpp"
#include "rebuilt.hpp"

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

#pragma optimize( "2", off )

bool anims::resolver::process_blood ( const effect_data_t& effect_data ) {
	OBF_BEGIN;

	auto hit_player = cs::i::ent_list->get_by_handle< player_t* > ( effect_data.m_ent_handle );
	
	IF ( !hit_player || !hit_player->is_player ( ) )
		RETURN ( false ); ENDIF;
	
	auto idx = hit_player->idx ( );

	std::vector<const features::ragebot::shot_t*> shots { };
	features::ragebot::get_shots ( idx, shots );

	IF ( shots.empty( ) )
		RETURN ( false ); ENDIF;

	const auto last_shot = shots.front ( );

	IF ( last_shot->hitbox == hitbox_t::r_foot
		|| last_shot->hitbox == hitbox_t::l_foot
		|| last_shot->hitbox == hitbox_t::r_calf
		|| last_shot->hitbox == hitbox_t::l_calf )
		RETURN ( false ) ENDIF;

	auto mdl = hit_player->mdl ( );

	IF ( !mdl )
		RETURN ( false ); ENDIF;

	auto studio_mdl = cs::i::mdl_info->studio_mdl ( mdl );

	IF ( !studio_mdl )
		RETURN ( false ); ENDIF;

	auto set = studio_mdl->hitbox_set ( N( 0 ) );

	IF ( !set )
		RETURN ( false ); ENDIF;

	auto hitbox = set->hitbox ( static_cast<int>( last_shot->hitbox ) );

	IF ( !hitbox )
		RETURN ( false ); ENDIF;

	vec3_t hitbox_pos_hit;
	
	IF ( hitbox->m_radius > N(0) )
		hitbox_pos_hit = effect_data.m_origin - effect_data.m_normal * hitbox->m_radius;
	ELSE
		hitbox_pos_hit = effect_data.m_origin;
	ENDIF;

	const auto angle_to_real_hitbox = cs::calc_angle ( g::local->eyes ( ), hitbox_pos_hit );
	const auto angle_to_player = cs::calc_angle ( g::local->eyes ( ), last_shot->rec.m_origin );

	const auto delta_hitbox_angle = cs::normalize ( angle_to_real_hitbox.y - angle_to_player.y );

	const auto real_hitbox_side = delta_hitbox_angle > N ( 0 ) ? desync_side_t::desync_right_max : desync_side_t::desync_left_max;

	IF ( ( rdata::resolved_side_run [ V ( idx ) ] >= desync_side_t::desync_middle && real_hitbox_side < desync_side_t::desync_middle )
		|| ( rdata::resolved_side_run [ V ( idx ) ] < desync_side_t::desync_middle && real_hitbox_side >= desync_side_t::desync_middle )) {
		features::ragebot::get_misses ( V ( idx ) ).bad_resolve = N ( 0 );

		rdata::resolved_side_run [ V ( idx ) ] = real_hitbox_side;
		rdata::resolved_jitter [ V ( idx ) ] = false;
		rdata::new_resolve [ V ( idx ) ] = true;
		rdata::prefer_edge [ V ( idx ) ] = false;

		//dbg_print ( _("-- BLOOD RESOLVED ACTIVE!! --") );
	} ENDIF;

	RETURN ( true );
	OBF_END;
}

#pragma optimize( "2", on )

void anims::resolver::process_impact ( event_t* event ) {
	static auto& bullet_impacts_server = options::vars [ _ ( "visuals.other.bullet_impacts_server" ) ].val.b;
	static auto& bullet_impacts_server_color = options::vars [ _ ( "visuals.other.bullet_impacts_server_color" ) ].val.c;

	auto shooter = cs::i::ent_list->get< player_t* > ( cs::i::engine->get_player_for_userid ( event->get_int ( _ ( "userid" ) ) ) );

	if ( !g::local || !g::local->alive ( ) || !shooter || shooter != g::local )
		return;

	const auto impact_pos = vec3_t ( event->get_float ( _ ( "x" ) ), event->get_float ( _ ( "y" ) ), event->get_float ( _ ( "z" ) ) );

	if ( impact_pos == vec3_t ( 0.0f, 0.0f, 0.0f ) )
		return;

	/* bullet impacts */
	if ( bullet_impacts_server )
		cs::add_box_overlay ( impact_pos, vec3_t ( -1.5f, -1.5f, -1.5f ), vec3_t ( 1.5f, 1.5f, 1.5f ), cs::calc_angle ( g::local->eyes( ), impact_pos ), bullet_impacts_server_color.r * 255.0f, bullet_impacts_server_color.g * 255.0f, bullet_impacts_server_color.b * 255.0f, bullet_impacts_server_color.a * 255.0f, 7.0f );

	impact_recs.push_back ( impact_rec_t { g::local->eyes ( ), impact_pos, _(""), cs::i::globals->m_curtime, false, rgba ( 255, 255, 255, 255 ), false } );

	/* process ragebot shots */
	const auto shot = features::ragebot::get_unprocessed_shot ( );

	if ( !shot )
		return;
	
	const auto player = cs::i::ent_list->get<player_t*> ( shot->idx );

	if ( !player || player->idx ( ) <= 0 || player->idx ( ) > 64 )
		return;

	const auto vec_max = shot->src + ( impact_pos - shot->src ).normalized ( ) * 4096.0f;

	const auto backup_origin = player->origin ( );
	auto backup_abs_origin = player->abs_origin ( );
	const auto backup_bone_cache = player->bone_cache ( );
	const auto backup_mins = player->mins ( );
	const auto backup_maxs = player->maxs ( );

	player->origin ( ) = shot->rec.m_origin;
	player->bone_cache ( ) = shot->rec.m_aim_bones [ shot->rec.m_side ].data ( );
	player->mins ( ) = shot->rec.m_mins;
	player->maxs ( ) = shot->rec.m_maxs;

	const auto dmg = autowall::dmg ( g::local, player, shot->src, vec_max, hitbox_t::invalid );

	player->origin ( ) = backup_origin;
	player->bone_cache ( ) = backup_bone_cache;
	player->mins ( ) = backup_mins;
	player->maxs ( ) = backup_maxs;

	shot->processed_impact_pos = impact_pos;
	shot->processed_impact_dmg = static_cast< int >( dmg + 0.5f );
	shot->processed_impact = true;
	shot->processed_tick = g::server_tick;
}

/* missed shot detection stuff */
std::array< anims::desync_side_t , 65 > last_recorded_resolve { anims::desync_side_t::desync_middle };

void anims::resolver::process_hurt ( event_t* event ) {
	static auto& hit_sound = options::vars [ _ ( "visuals.other.hit_sound" ) ].val.i;

	auto dmg = event->get_int ( _ ( "dmg_health" ) );
	auto attacker = cs::i::ent_list->get< player_t* > ( cs::i::engine->get_player_for_userid ( event->get_int ( _ ( "attacker" ) ) ) );
	auto victim = cs::i::ent_list->get< player_t* > ( cs::i::engine->get_player_for_userid ( event->get_int ( _ ( "userid" ) ) ) );
	auto hitgroup = event->get_int ( _ ( "hitgroup" ) );

	if ( !attacker || !victim || attacker != g::local || victim->team ( ) == g::local->team ( ) || victim->idx ( ) <= 0 || victim->idx ( ) > 64 )
		return;

	/* hitsound */
	switch ( hit_sound ) {
	case 0: break;
	case 1: cs::i::engine->client_cmd_unrestricted ( _ ( "play buttons\\arena_switch_press_02" ) ); break;
	case 2: cs::i::engine->client_cmd_unrestricted ( _ ( "play player\\pl_fallpain3" ) ); break;
	case 3: cs::i::engine->client_cmd_unrestricted ( _ ( "play weapons\\awp\\awp_boltback" ) ); break;
	case 4: cs::i::engine->client_cmd_unrestricted ( _ ( "play weapons\\flashbang\\grenade_hit1" ) ); break;
	case 5: LI_FN ( PlaySoundA ) ( reinterpret_cast < const char* > ( hitsound_sesame ), nullptr, SND_MEMORY | SND_ASYNC ); break;
	default: break;
	}

	features::ragebot::get_hits ( victim->idx ( ) )++;

	/* process ragebot hits */
	const auto shot = features::ragebot::get_unprocessed_shot ( );

	if ( shot )
		features::chams::add_shot ( victim, shot->rec );
	else if ( !anim_info [ victim->idx ( ) ].empty( ) )
		features::chams::add_shot ( victim, anim_info [ victim->idx ( ) ].front ( ) );

	if ( !shot )
		return;

	const auto player = cs::i::ent_list->get<player_t*> ( shot->idx );

	if ( !player || player->idx ( ) <= 0 || player->idx ( ) > 64 || player != victim )
		return;

	shot->processed_dmg = dmg;
	shot->processed_hitgroup = hitgroup;
	shot->processed_hurt = true;
	shot->processed_tick = g::server_tick;
}

void anims::resolver::process_event_buffer ( ) {
	static auto& logs = options::vars [ _ ( "visuals.other.logs" ) ].val.l;

	if ( !g::local ) {
		if ( !features::ragebot::shots.empty ( ) )
			features::ragebot::shots.clear ( );

		return;
	}

	auto get_hitgroup_name = [ ] ( int hitgroup ) -> std::string {
		switch ( hitgroup ) {
		case 0: return _ ( "generic" );
		case 1: return _ ( "head" );
		case 2: return _ ( "chest" );
		case 3: return _ ( "stomach" );
		case 4: return _ ( "left arm" );
		case 5: return _ ( "right arm" );
		case 6: return _ ( "left leg" );
		case 7: return _ ( "right leg" );
		case 8: return _ ( "neck" );
		case 10: return _ ( "gear" );
		default: return _ ( "unknown" );
		}

		return _ ( "unknown" );
	};

	/*
	*	POSSIBILITIES
	* -- LOCAL DEAD --
	*	1. shot clientsided
	* -- PLAYER HURT --
	*	1. hit shot
	* -- ENEMY DEAD --
	*	2. miss due to enemy death
	* -- NO HURT --
	*	1. bad anims/resolve
	*	2. miss due to spread
	*	3. pred error (VEL MODIFIER < 1 (this is so ghetto, lol))
	*/

	/* shot logger */
	for ( auto& shot : features::ragebot::shots ) {
		if ( ( !shot.processed_tick || g::server_tick == shot.processed_tick ) && g::local->alive ( ) )
			continue;

		auto player = cs::i::ent_list->get<player_t*> ( shot.idx );

		if ( !player || player->idx ( ) <= 0 || player->idx ( ) > 64 )
			player = nullptr;

		std::string reason = _ ( "?" );
		
		if ( !g::local->alive ( ) ) {
			reason = _ ( "local death" );
		}
		else if ( player ) {
			if ( shot.processed_hurt ) {
				reason = _ ( "shot" );
				impact_recs.push_back ( impact_rec_t { g::local->eyes ( ), shot.processed_impact_pos, std::to_string( shot.processed_dmg ), cs::i::globals->m_curtime, true, rgba ( 255, 255, 255, 255 ), false } );
			}
			else {
				if ( !player->alive ( ) ) {
					reason = _ ( "enemy death" );
				}
				else {
					auto recalc_hitchance = 0.0f;
					features::ragebot::hitchance ( cs::calc_angle( shot.src, shot.dst ), player, shot.dst, 256, shot.hitbox, shot.rec, recalc_hitchance );

					auto found_hitbox = false;
					auto studio_mdl = cs::i::mdl_info->studio_mdl ( player->mdl ( ) );

					if ( studio_mdl ) {
						auto set = studio_mdl->hitbox_set ( 0 );

						if ( set ) {
							auto hhitbox = set->hitbox ( static_cast< int >( shot.hitbox ) );

							if ( hhitbox ) {
								vec3_t vmin, vmax;
								VEC_TRANSFORM ( hhitbox->m_bbmin, shot.rec.m_aim_bones [ shot.rec.m_side ][ hhitbox->m_bone ], vmin );
								VEC_TRANSFORM ( hhitbox->m_bbmax, shot.rec.m_aim_bones [ shot.rec.m_side ][ hhitbox->m_bone ], vmax );

								if ( autowall::trace_ray ( vmin, vmax, shot.rec.m_aim_bones [ shot.rec.m_side ][ hhitbox->m_bone ], hhitbox->m_radius, shot.src, shot.processed_impact_pos ) )
									found_hitbox = true;
							}
						}
					}

					if ( shot.processed_impact_dmg ) {
						reason = _ ( "bad anims" );
						features::ragebot::get_misses ( player->idx ( ) ).bad_resolve++;
					}
					else if ( /*static_cast< int >( recalc_hitchance + 0.5f ) >= 100 || shot.vel_modifier < 1.0f*/ g::local->velocity_modifier( ) < 1.0f ) {
						reason = _ ( "pred error" );
						features::ragebot::get_misses ( player->idx ( ) ).pred_error++;
					}
					else if ( found_hitbox ) {
						reason = _ ( "occlusion" );
						features::ragebot::get_misses ( player->idx ( ) ).occlusion++;
					}
					else if ( !g::cvars::weapon_accuracy_nospread->get_bool ( ) ) {
						reason = _ ( "spread" );
						features::ragebot::get_misses ( player->idx ( ) ).spread++;
					}
				}
			}
		}

		if ( logs [ 0 ] ) {
			player_info_t info { };
			const auto player_info_valid = cs::i::engine->get_player_info ( shot.idx, &info );

			print_console ( { 0.85f, 0.31f, 0.83f, 1.0f }, _ ( "sesame" ) );
			
			if ( player_info_valid )
				print_console ( { 0.96f, 0.8f, 0.95f, 1.0f }, shot.processed_hurt ? _ ( " ~ hit %s" ) : _ ( " ~ missed %s" ), info.m_name );
			else
				print_console ( { 0.96f, 0.8f, 0.95f, 1.0f }, shot.processed_hurt ? _ ( " ~ hit" ) : _ ( " ~ miss" ) );

			/* shot info */
			const options::option::colorf info_color = { 0.8f, 0.95f, 0.95f, 1.0f };

			print_console ( info_color, _ ( " [r: %s" ), reason.c_str ( ) );

			if ( shot.processed_hurt && g::local->alive ( ) )
				print_console ( info_color, _ ( ", hb: %s" ), get_hitgroup_name ( shot.processed_hitgroup ).c_str ( ) );

			print_console ( info_color, _ ( ", thb: %s" ), get_hitgroup_name ( autowall::hitbox_to_hitgroup ( shot.hitbox ) ).c_str ( ) );
			print_console ( info_color, _ ( ", bt: %dt" ), shot.backtrack );

			if ( shot.hitchance != -1.0f )
				print_console ( info_color, _ ( ", hc: %d%%" ), static_cast< int >( shot.hitchance + 0.5f ) );

			if ( shot.processed_hurt && g::local->alive ( ) )
				print_console ( info_color, _ ( ", dmg: %dhp" ), shot.processed_dmg );

			if ( shot.dmg != -1 )
				print_console ( info_color, _ ( ", tdmg: %dhp" ), shot.dmg );

			print_console ( info_color, _ ( ", desync: %d°" ), static_cast< int >( shot.body_yaw + 0.5f ) );

			print_console ( info_color, _ ( ", fl: " ) );

			if ( !( shot.rec.m_flags & flags_t::on_ground ) )
				print_console ( info_color, _ ( "air" ) );
			else if ( shot.rec.m_anim_layers [ desync_side_t::desync_max ][ 6 ].m_weight <= 0.01f )
				print_console ( info_color, _ ( "stand" ) );
			else if ( shot.rec.m_anim_layers [ desync_side_t::desync_max ][ 6 ].m_weight < 0.95f )
				print_console ( info_color, _ ( "walk" ) );
			else
				print_console ( info_color, _ ( "run" ) );

			if ( shot.rec.m_shot )
				print_console ( info_color, _ ( " & onshot" ) );

			if ( player_info_valid && info.m_fake_player )
				print_console ( info_color, _ ( " & bot" ) );

			print_console ( info_color, _ ( "]\n" ) );
		}
	}

	/* erase shot records that have already been processed */
	features::ragebot::shots.erase ( std::remove_if( features::ragebot::shots.begin(), features::ragebot::shots.end(), [ ] ( const features::ragebot::shot_t& shot ) {
		return shot.processed_tick > 0;
	} ), features::ragebot::shots.end ( ) );
	
	if ( !g::local->alive ( ) && !features::ragebot::shots.empty ( ) )
		features::ragebot::shots.clear ( );
}

void anims::resolver::create_beams ( ) {
	static auto& bullet_tracers = options::vars [ _ ( "visuals.other.bullet_tracers" ) ].val.b;
	static auto& bullet_tracer_color = options::vars [ _ ( "visuals.other.bullet_tracer_color" ) ].val.c;

	if ( !g::local || !g::local->alive ( ) ) {
		if ( !impact_recs.empty ( ) )
			impact_recs.clear ( );

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

		if ( impact.m_hurt )
			continue;

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
	if ( !g::local || !g::local->alive ( ) ) {
		if ( !impact_recs.empty ( ) )
			impact_recs.clear ( );

		return;
	}

	auto calc_alpha = [ & ] ( float time, float fade_time, float base_alpha, bool add = false ) {
		const auto dormant_time = 4.0f;
		return static_cast< int >( ( std::clamp< float > ( dormant_time - ( std::clamp< float > ( add ? ( dormant_time - std::clamp< float > ( abs ( cs::i::globals->m_curtime - time ), 0.0f, dormant_time ) ) : abs ( cs::i::globals->m_curtime - time ), std::max< float > ( dormant_time - fade_time, 0.0f ), dormant_time ) ), 0.0f, fade_time ) / fade_time ) * base_alpha );
	};

	if ( impact_recs.empty ( ) )
		return;

	int cur_ray = 0;

	for ( auto& impact : impact_recs ) {
		if ( !impact.m_hurt )
			continue;

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

		if ( damage_indicator ) {
			vec3_t dim;
			render::text_size ( impact.m_msg, _ ( "watermark_font" ), dim );
			render::text ( scrn_dst.x - dim.x / 2, scrn_dst.y - 26.0f - ( std::clamp ( cs::i::globals->m_curtime - impact.m_time, 0.0f, 6.0f ) / 6.0f ) * 100.0f, impact.m_msg, ( "watermark_font" ), rgba<int> ( damage_indicator_color.r * 255.0f, damage_indicator_color.g * 255.0f, damage_indicator_color.b * 255.0f, calc_alpha ( impact.m_time, 2.0f, damage_indicator_color.a * 255.0f ) ), true );
		}
		
		/* hit dot */
		if ( player_hits ) {
			const auto radius = ( std::clamp ( cs::i::globals->m_curtime - impact.m_time, 0.0f, 6.0f ) / 6.0f ) * 50.0f;
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
			circle [ 0 ].color = D3DCOLOR_RGBA ( static_cast< int >( player_hits_color.r * 255.0f ), static_cast< int >( player_hits_color.g * 255.0f ), static_cast< int >( player_hits_color.b * 255.0f ), static_cast< int >( ( 1.0f - std::clamp ( cs::i::globals->m_curtime - impact.m_time, 0.0f, 6.0f ) / 6.0f ) * ( bullet_tracer_color.a * 255.0f )) );

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

		cur_ray++;
	}
}

#pragma optimize( "2", off )
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

bool anims::resolver::bruteforce ( int brute_mode /* 0 = none, 1 = opposite, 2 = close, 3 = fast, 4 = fast opposite*/, int target_side, anim_info_t& rec ) {
	//OBF_BEGIN;
	//CASE ( brute_mode )
	//	WHEN ( N ( 0 ) ) DO rec.m_side = static_cast< anims::desync_side_t >( target_side ); BREAK; DONE
	//	WHEN ( N ( 1 ) ) DO {
	//	CASE ( static_cast< anims::desync_side_t >( target_side ) )
	//		WHEN ( anims::desync_side_t::desync_left_max ) DO rec.m_side = anims::desync_side_t::desync_right_max; BREAK; DONE
	//		WHEN ( anims::desync_side_t::desync_left_half ) DO rec.m_side = anims::desync_side_t::desync_middle; BREAK; DONE
	//		WHEN ( anims::desync_side_t::desync_right_max ) DO rec.m_side = anims::desync_side_t::desync_left_max; BREAK; DONE
	//		WHEN ( anims::desync_side_t::desync_right_half ) DO rec.m_side = anims::desync_side_t::desync_left_half; BREAK; DONE
	//		WHEN ( anims::desync_side_t::desync_middle ) DO rec.m_side = anims::desync_side_t::desync_left_half; BREAK; DONE
	//	ENDCASE;
	//} BREAK; DONE
	//	WHEN ( N ( 2 ) ) DO {
	//	CASE ( static_cast< anims::desync_side_t >( target_side ) )
	//		WHEN ( anims::desync_side_t::desync_left_max ) DO rec.m_side = anims::desync_side_t::desync_left_half; BREAK; DONE
	//		WHEN ( anims::desync_side_t::desync_left_half ) DO rec.m_side = anims::desync_side_t::desync_left_max; BREAK; DONE
	//		WHEN ( anims::desync_side_t::desync_right_max ) DO rec.m_side = anims::desync_side_t::desync_middle; BREAK; DONE
	//		WHEN ( anims::desync_side_t::desync_right_half ) DO rec.m_side = anims::desync_side_t::desync_right_max; BREAK; DONE
	//		WHEN ( anims::desync_side_t::desync_middle ) DO rec.m_side = anims::desync_side_t::desync_right_max; BREAK; DONE
	//		ENDCASE;
	//} BREAK; DONE
	//	WHEN ( N ( 3 ) ) DO {
	//	CASE ( static_cast< anims::desync_side_t >( target_side ) )
	//		WHEN ( anims::desync_side_t::desync_left_max ) DO rec.m_side = anims::desync_side_t::desync_left_max; BREAK; DONE
	//		WHEN ( anims::desync_side_t::desync_left_half ) DO rec.m_side = anims::desync_side_t::desync_middle; BREAK; DONE
	//		WHEN ( anims::desync_side_t::desync_right_max ) DO rec.m_side = anims::desync_side_t::desync_middle; BREAK; DONE
	//		WHEN ( anims::desync_side_t::desync_right_half ) DO rec.m_side = anims::desync_side_t::desync_middle; BREAK; DONE
	//		WHEN ( anims::desync_side_t::desync_middle ) DO rec.m_side = anims::desync_side_t::desync_middle; BREAK; DONE
	//		ENDCASE;
	//} BREAK; DONE
	//	WHEN ( N ( 4 ) ) DO {
	//	CASE ( static_cast< anims::desync_side_t >( target_side ) )
	//		WHEN ( anims::desync_side_t::desync_left_max ) DO rec.m_side = anims::desync_side_t::desync_middle; BREAK; DONE
	//		WHEN ( anims::desync_side_t::desync_left_half ) DO rec.m_side = anims::desync_side_t::desync_left_max; BREAK; DONE
	//		WHEN ( anims::desync_side_t::desync_right_max ) DO rec.m_side = anims::desync_side_t::desync_left_max; BREAK; DONE
	//		WHEN ( anims::desync_side_t::desync_right_half ) DO rec.m_side = anims::desync_side_t::desync_left_max; BREAK; DONE
	//		WHEN ( anims::desync_side_t::desync_middle ) DO rec.m_side = anims::desync_side_t::desync_left_max; BREAK; DONE
	//		ENDCASE;
	//} BREAK; DONE
	//	DEFAULT rec.m_side = static_cast< anims::desync_side_t >( target_side ); DONE
	//	ENDCASE;
	//OBF_END;

	switch ( brute_mode ) {
	case 0: {
		rec.m_side = static_cast< anims::desync_side_t >( target_side );
	} break;
	case 1: {
		switch ( target_side ) {
		case anims::desync_side_t::desync_left_max: rec.m_side = anims::desync_side_t::desync_right_max; break;
		case anims::desync_side_t::desync_left_half: rec.m_side = anims::desync_side_t::desync_middle; break;
		case anims::desync_side_t::desync_right_max: rec.m_side = anims::desync_side_t::desync_left_max; break;
		case anims::desync_side_t::desync_right_half: rec.m_side = anims::desync_side_t::desync_left_half; break;
		case anims::desync_side_t::desync_middle: rec.m_side = anims::desync_side_t::desync_left_half; break;
		}
	} break;
	case 2: {
		switch ( target_side ) {
		case anims::desync_side_t::desync_left_max: rec.m_side = anims::desync_side_t::desync_left_half; break;
		case anims::desync_side_t::desync_left_half: rec.m_side = anims::desync_side_t::desync_left_max; break;
		case anims::desync_side_t::desync_right_max: rec.m_side = anims::desync_side_t::desync_middle; break;
		case anims::desync_side_t::desync_right_half: rec.m_side = anims::desync_side_t::desync_right_max; break;
		case anims::desync_side_t::desync_middle: rec.m_side = anims::desync_side_t::desync_right_max; break;
		}
	} break;
	case 3: {
		switch ( target_side ) {
		case anims::desync_side_t::desync_left_max: rec.m_side = anims::desync_side_t::desync_left_max; break;
		case anims::desync_side_t::desync_left_half: rec.m_side = anims::desync_side_t::desync_middle; break;
		case anims::desync_side_t::desync_right_max: rec.m_side = anims::desync_side_t::desync_middle; break;
		case anims::desync_side_t::desync_right_half: rec.m_side = anims::desync_side_t::desync_middle; break;
		case anims::desync_side_t::desync_middle: rec.m_side = anims::desync_side_t::desync_middle; break;
		}
	} break;
	case 4: {
		switch ( target_side ) {
		case anims::desync_side_t::desync_left_max: rec.m_side = anims::desync_side_t::desync_middle; break;
		case anims::desync_side_t::desync_left_half: rec.m_side = anims::desync_side_t::desync_left_max; break;
		case anims::desync_side_t::desync_right_max: rec.m_side = anims::desync_side_t::desync_left_max; break;
		case anims::desync_side_t::desync_right_half: rec.m_side = anims::desync_side_t::desync_left_max; break;
		case anims::desync_side_t::desync_middle: rec.m_side = anims::desync_side_t::desync_left_max; break;
		}
	} break;
	default: rec.m_side = static_cast< anims::desync_side_t >( target_side ); break;
	}

	return true;
}

anims::desync_side_t anims::resolver::apply_antiaim ( player_t* player, const anim_info_t& rec, float speed_2d, float max_speed ) {
	OBF_BEGIN;
	int idx = player->idx ( );

	const bool antiaim_disabled = anim_info [ idx ].size ( ) >= N( 1 ) && abs ( rec.m_angles.x ) < 70.0f && abs ( anim_info [ idx ][ 0 ].m_angles.x ) < 70.0f && !rec.m_shot && !anim_info [ idx ][ 0 ].m_shot;
	
	IF ( antiaim_disabled ) {
		RETURN ( rdata::resolved_side_run [ idx ] );
	}
	ELSE {
		IF ( rdata::resolved_jitter [ idx ] && !features::ragebot::get_misses ( idx ).bad_resolve ) {
			IF ( rdata::jitter_sync [ idx ] == cs::time2ticks ( rec.m_simtime ) % 2 )
				RETURN ( rdata::resolved_side_jitter1 [ idx ] );
			ELSE
				RETURN ( rdata::resolved_side_jitter2 [ idx ] );
			ENDIF;
		}
		ELSE {
			RETURN ( rdata::resolved_side_run [ idx ] );
		} ENDIF;
	} ENDIF;

	RETURN ( desync_side_t::desync_max );
	OBF_END;
}

bool anims::resolver::is_spotted ( player_t* src, player_t* dst ) {
	OBF_BEGIN;
	static const auto spotted_by_mask_off = netvars::get_offset ( _ ( "DT_BaseEntity->m_bSpottedByMask" ) );

	auto idx = dst->idx ( );

	IF ( idx > N( 0 ) && idx <= V( cs::i::globals->m_max_clients ) )
		RETURN ( *reinterpret_cast< uint32_t* >( reinterpret_cast< uintptr_t >( dst ) + spotted_by_mask_off ) & ( N( 1 ) << ( src->idx ( ) - 1 ) ) ); ENDIF
	
	RETURN ( false );
	OBF_END;
}

std::array< int, 65 > last_resolved_sides { };
std::array< int, 65 > last_anti_freestand_sides { };

bool anims::resolver::resolve_desync_skeet ( player_t* ent, anim_info_t& rec, bool shot ) {
	static auto& resolver = options::vars [ _ ( "ragebot.resolve_desync" ) ].val.b;
	
	const auto idx = ent->idx ( );

	const auto resolver_mode = 1; /* CHANGE LATER */
	const auto& missed_shots = features::ragebot::get_misses ( idx );
	const auto total_misses = missed_shots.bad_resolve + missed_shots.occlusion + missed_shots.pred_error + missed_shots.spread;
	const auto hits = features::ragebot::get_hits ( idx );

	const auto& default_animstate = rec.m_anim_state [ desync_side_t::desync_max ];
	
	const auto server_layers = ent->layers ( );

	/* change this in resolver */
	int desync_side = last_resolved_sides [ idx ];

	std::array< desync_side_t, 3 > desync_side_skeet {
		desync_side_t::desync_right_max,
		desync_side_t::desync_middle,
		desync_side_t::desync_left_max
	};

	bool jitter_resolver = false;

	auto& cur_anim_info = anim_info [ idx ];

	if ( !cur_anim_info.empty ( ) ) {
		const auto& prev_anim_info = anim_info [ idx ].front ( );
		const auto& prev_server_layers = prev_anim_info.m_anim_layers [ desync_side_t::desync_max ];

		const auto delta_yaw = angle_diff1 ( rec.m_anim_state [ desync_side_t::desync_max ].m_eye_yaw, rec.m_anim_state [ desync_side_t::desync_max ].m_abs_yaw );
		const auto delta_yaw_old = angle_diff1 ( prev_anim_info.m_anim_state [ desync_side_t::desync_max ].m_eye_yaw, prev_anim_info.m_anim_state [ desync_side_t::desync_max ].m_abs_yaw );
		const auto delta_eye_yaw = angle_diff1 ( rec.m_anim_state [ desync_side_t::desync_max ].m_eye_yaw, prev_anim_info.m_anim_state [ desync_side_t::desync_max ].m_eye_yaw );
		const auto delta_at_target = angle_diff1 ( rec.m_anim_state [ desync_side_t::desync_max ].m_eye_yaw, cs::calc_angle ( g::local->eyes ( ), rec.m_origin + vec3_t ( N ( 0 ), N ( 0 ), N ( 64 ) ) ).y );

		if ( abs ( delta_at_target ) < 45.0f ) {
			const auto eyes_max = ent->eyes ( );
			const auto eyes_fwd = cs::angle_vec ( vec3_t ( 0.0f, ent->angles ( ).y, 0.0f ) ).normalized ( );
			const auto eyes_right = eyes_fwd.cross_product ( vec3_t ( N ( 0 ), N ( 0 ), N ( 1 ) ) );
			const auto src = g::local->eyes ( );

			auto left_head = eyes_max + eyes_right * N ( 35 );
			auto right_head = eyes_max - eyes_right * N ( 35 );

			const auto dmg_left = autowall::dmg ( g::local, ent, src + eyes_right * N ( 35 ), left_head, hitbox_t::head );
			const auto dmg_right = autowall::dmg ( g::local, ent, src - eyes_right * N ( 35 ), right_head, hitbox_t::head );
			const auto one_side_hittable = ( dmg_left && !dmg_right ) || ( !dmg_left && dmg_right );

			if ( one_side_hittable )
				last_anti_freestand_sides [ idx ] = dmg_left > 0.0f ? 0 : 2;
		}

		const auto lby_delta_yaw = angle_diff1 ( default_animstate.m_eye_yaw, rec.m_lby );

		if ( delta_yaw > N ( 1 ) && delta_yaw_old > N ( 1 )
			&& abs ( delta_yaw - delta_yaw_old ) > abs ( delta_eye_yaw )
			&& abs ( delta_yaw ) > N ( 45 ) && abs ( delta_yaw_old ) > N ( 45 ) && delta_yaw * delta_yaw_old < N ( 0 ) ) {
			float some_yaw = N ( 0 );

			if ( delta_yaw - delta_yaw_old <= N ( 0 ) )
				some_yaw = default_animstate.m_eye_yaw - N ( 90 );
			else
				some_yaw = default_animstate.m_eye_yaw + N ( 90 );

			some_yaw = cs::normalize ( some_yaw );

			float v41 = angle_diff1 ( some_yaw, default_animstate.m_abs_yaw );
			float v42 = angle_diff1 ( some_yaw, prev_anim_info.m_anim_state [ desync_side_t::desync_max ].m_abs_yaw );

			desync_side = abs ( v41 ) <= abs ( v42 ) ? 0 : 1;
			jitter_resolver = true;
		}
		else if ( server_layers [ 3 ].m_weight <= 0.0f
			&& server_layers [ 3 ].m_cycle <= 0.0f
			&& server_layers [ 6 ].m_weight <= 0.0f
			&& abs ( lby_delta_yaw ) > 35.0f ) {
			desync_side = lby_delta_yaw <= 0.0f ? 0 : 2;
			rec.m_resolved = true;
		}
		else if ( server_layers [ 6 ].m_playback_rate > 0.0005f && !!( rec.m_flags & flags_t::on_ground ) ) {
			int choked_ticks = rec.m_choked_commands - 1;

			if ( choked_ticks > 0 ) {
				if ( server_layers [ 6 ].m_sequence == prev_server_layers [ 6 ].m_sequence &&
					abs ( server_layers [ 6 ].m_weight - prev_server_layers [ 6 ].m_weight ) <= 0.05f
					&& rec.m_duck_amount == prev_anim_info.m_duck_amount
					&& ( rec.m_vel.length ( ) - prev_anim_info.m_vel.length ( ) ) <= 5.0f
					&& rec.m_vel.length ( ) >= 1.0f ) {
					const auto backup_poses = ent->poses ( );

					float velocity_delta = 3.0f;

					if ( rec.m_choked_commands >= 2 )
						velocity_delta = ( rec.m_vel - prev_anim_info.m_vel ).length ( );

					int side = -1;
					int best_side = 1;
					float best_weight = 0.0001f;
					float best_value = velocity_delta;
					float weight_delta = 0.0001f;

					for ( int i = 0; i < 3; i++ ) {
						desync_side_t mapped_side = desync_side_t::desync_middle;

						if ( i == 0 )
							mapped_side = desync_side_t::desync_right_max;
						else if ( i == 2 )
							mapped_side = desync_side_t::desync_left_max;

						ent->poses ( ) = rec.m_poses [ mapped_side ];

						float velocity_2d = rec.m_anim_state [ mapped_side ].m_speed2d;
						float groundfraction = rec.m_anim_state [ mapped_side ].m_ground_fraction;

						float pb = velocity_2d;
						float v827 = server_layers [ 6 ].m_playback_rate;

						if ( v827 > 0.0f ) {
							float v834 = ent->get_sequence_cycle_rate_server ( server_layers [ 6 ].m_sequence );
							float v442 = v827 / cs::ticks2time ( 1 );

							pb = ( pb / ( ( v442 / ( 1.0f - ( groundfraction * 0.15f ) ) ) / v834 ) ) * ( 1.0f / v834 );
						}
						else {
							pb = 0.0f;
						}

						float move_dist = ent->get_sequence_move_distance ( anims::rebuilt::get_model_ptr( ent ), server_layers [ 6 ].m_sequence );
						float delta = abs ( move_dist - pb );

						float weight_clamped = std::clamp ( rec.m_anim_layers [ mapped_side ][ 6 ].m_weight, 0.0f, 1.0f );
						float _weight_delta = abs ( weight_clamped - server_layers [ 6 ].m_weight );

						float compare_value = best_value;

						if ( mapped_side == desync_side_t::desync_middle )
							compare_value = std::min ( 1.0f, best_value );

						best_side = i;

						if ( compare_value > delta )
							best_value = delta;
						else {
							best_weight = _weight_delta;
							side = i;
						}

						ent->poses ( ) = backup_poses;
					}

					if ( velocity_delta > best_value && best_weight < 0.0001f && side != -1 ) {
						desync_side = side;
						rec.m_resolved = true;
					}
				}
			}
		}
	}

	if ( !jitter_resolver ) {
		auto newest_resolver_update = std::numeric_limits< float >::max ( );

		for ( auto& anim_rec : cur_anim_info )
			if ( anim_rec.m_resolved )
				newest_resolver_update = anim_rec.m_simtime;

		float max_speed = N ( 260 );
		auto weapon_info = ent->weapon ( ) ? ent->weapon ( )->data ( ) : nullptr;

		if ( weapon_info )
			max_speed = ent->scoped ( ) ? weapon_info->m_max_speed_alt : weapon_info->m_max_speed;

		if ( abs ( cs::i::globals->m_curtime - newest_resolver_update ) > 5.0f
			|| ( !anim_info [ idx ].empty ( ) && server_layers [ 12 ].m_weight > 0.1f && anim_info [ idx ][ 0 ].m_anim_layers [ desync_side_t::desync_max ][ 12 ].m_weight > 0.1f && rec.m_vel.length_2d ( ) < max_speed * 0.666f ) )
			desync_side = last_anti_freestand_sides [ idx ];
	}

	last_resolved_sides [ idx ] = desync_side;

	/* skeet bruteforce */
	int brute_side = 1;

	switch ( resolver_mode % 3 ) {
	case 0: /* none */ {
		brute_side = desync_side;
	} break;
	case 1:
	case 2: /* anti bruteforce / air / onshot ??? */ {
		if ( !( ( total_misses + 1 ) % 3 ) ) {
			const auto fails = missed_shots.bad_resolve;
			const auto total_shots = fails + hits;

			auto chance = 50.0f;

			if ( total_shots )
				chance = 100.0f * static_cast< float >( fails ) / static_cast< float >( total_shots );

			srand ( total_misses + hits );

			auto rand_chance = static_cast< float >( rand ( ) % 101 );

			if ( rand_chance <= chance )
				brute_side = 2;
			else
				brute_side = desync_side;
		}

		if ( !( rec.m_flags & flags_t::on_ground ) ) {
			const auto resolver_mode_brute_cycle = resolver_mode ? 3 : 0;

			auto new_misses = 0;

			if ( resolver_mode_brute_cycle )
				new_misses = total_misses % resolver_mode_brute_cycle;
			else
				new_misses = 0;

			if ( !new_misses && default_animstate.m_time_in_air >= 0.75f )
				brute_side = 2;
			else
				brute_side = desync_side;
		}
		else if ( shot ) {
			const auto delta_yaw = angle_diff1 ( default_animstate.m_eye_yaw, default_animstate.m_abs_yaw );

			last_resolved_sides [ idx ] = brute_side = ( delta_yaw <= 0.0f ) ? 0 : 2;
		}
	} break;
	case 3: {
		if ( resolver_mode == 3 ) {
			if ( missed_shots.bad_resolve % 3 == 1 ) {
				if ( default_animstate.m_time_since_move > 10.0f ) {
					brute_side = 2;
					break;
				}
			}

			brute_side = desync_side;
		}
	} break;
	}

	rec.m_side = desync_side_skeet [ ( brute_side + total_misses ) % desync_side_skeet.size ( ) ];

	if ( !resolver ) {
		rec.m_side = desync_side_t::desync_max;
		rec.m_resolved = false;

		return false;
	}

	return true;
}

bool anims::resolver::resolve_desync( player_t* ent, anim_info_t& rec, bool shot ) {
	OBF_BEGIN;
	/* set default desync direction to middle (0 deg range) */
	const auto idx = ent->idx ( );

	float max_speed = N ( 260 );
	auto weapon_info = ent->weapon ( ) ? ent->weapon ( )->data ( ) : nullptr;

	IF ( weapon_info )
		max_speed = ent->scoped ( ) ? weapon_info->m_max_speed_alt : weapon_info->m_max_speed; ENDIF;

	rec.m_side = desync_side_t::desync_max;

	static auto& resolver = options::vars [ _ ( "ragebot.resolve_desync" ) ].val.b;

	security_handler::update ( );

	const auto anim_layers = ent->layers ( );

	player_info_t pl_info;
	cs::i::engine->get_player_info ( idx, &pl_info );

	/* bot check */
	IF ( pl_info.m_fake_player ) {
		rec.m_side = desync_side_t::desync_max;
		RETURN ( false );
	} ENDIF;

	IF ( !resolver || !g::local || !anim_layers || ent->team ( ) == g::local->team ( ) || anim_info [ idx ].size ( ) <= N( 1 ) || rec.m_choked_commands <= 0 || shot ) {
		IF ( ent->team ( ) != g::local->team ( ) && ( anim_info [ idx ].size ( ) <= N ( 1 ) || rec.m_choked_commands <= 0 || shot ) ) {
			rec.m_side = apply_antiaim ( ent, rec, rec.m_vel.length_2d ( ), max_speed );

			const auto has_accurate_weapon = g::local && g::local->weapon ( ) && g::local->weapon ( )->item_definition_index ( ) == weapons_t::ssg08;
			const auto has_bad_desync = /*ent->desync_amount ( ) <= 34.0f*/false;

			switch ( features::ragebot::get_misses ( idx ).bad_resolve % 4 ) {
			case 0: bruteforce ( has_bad_desync ? 3 : 0, rec.m_side, rec ); break;
			case 1: bruteforce ( has_bad_desync ? 4 : ( has_accurate_weapon ? 1 : 2 ), rec.m_side, rec ); break;
			case 2: bruteforce ( has_bad_desync ? 3 : ( has_accurate_weapon ? 2 : 1 ), rec.m_side, rec ); break;
			case 3: bruteforce ( 4, rec.m_side, rec ); break;
			default: bruteforce ( has_bad_desync ? 3 : 0, rec.m_side, rec ); break;
			}

			RETURN ( true );
		} ENDIF;

		rec.m_side = apply_antiaim ( ent, rec, rec.m_vel.length_2d ( ), max_speed );

		const auto has_accurate_weapon = g::local && g::local->weapon ( ) && g::local->weapon ( )->item_definition_index ( ) == weapons_t::ssg08;
		const auto has_bad_desync = /*ent->desync_amount ( ) <= 34.0f*/false;

		switch ( features::ragebot::get_misses ( idx ).bad_resolve % 4 ) {
		case 0: bruteforce ( has_bad_desync ? 3 : 0, rec.m_side, rec ); break;
		case 1: bruteforce ( has_bad_desync ? 4 : ( has_accurate_weapon ? 1 : 2 ), rec.m_side, rec ); break;
		case 2: bruteforce ( has_bad_desync ? 3 : ( has_accurate_weapon ? 2 : 1 ), rec.m_side, rec ); break;
		case 3: bruteforce ( 4, rec.m_side, rec ); break;
		default: bruteforce ( has_bad_desync ? 3 : 0, rec.m_side, rec ); break;
		}
		RETURN( false );
	} ENDIF;

	memcpy ( &rdata::latest_layers [ idx ], anim_layers, sizeof( rdata::latest_layers [ idx ] ) );

	/* start of resolver */
	const auto speed_2d = rec.m_vel.length_2d ( );
	const auto delta_yaw = angle_diff1 ( rec.m_anim_state [ desync_side_t::desync_max ].m_eye_yaw, rec.m_anim_state [ desync_side_t::desync_max ].m_abs_yaw );
	const auto delta_yaw_old = angle_diff1 ( anim_info [ idx ][ N ( 0 ) ].m_anim_state [ desync_side_t::desync_max ].m_eye_yaw, anim_info [ idx ][ N ( 0 ) ].m_anim_state [ desync_side_t::desync_max ].m_abs_yaw );
	const auto delta_yaw_total = angle_diff1 ( rec.m_anim_state [ desync_side_t::desync_max ].m_abs_yaw, anim_info [ idx ][ N ( 0 ) ].m_anim_state [ desync_side_t::desync_max ].m_abs_yaw );
	const auto delta_eye_yaw = angle_diff1 ( rec.m_anim_state [ desync_side_t::desync_max ].m_eye_yaw, anim_info [ idx ][ N ( 0 ) ].m_anim_state [ desync_side_t::desync_max ].m_eye_yaw );
	const auto delta_at_target = angle_diff1 ( rec.m_anim_state [ desync_side_t::desync_max ].m_eye_yaw, cs::calc_angle ( g::local->eyes ( ), rec.m_origin + vec3_t ( N ( 0 ), N ( 0 ), N ( 64 ) ) ).y );

	/* onshot desync */
	//IF ( shot && abs ( rec.m_angles.x ) < N ( 75 ) && abs ( anim_info [ idx ][ 0 ].m_angles.x ) > N ( 75 ) ) {
	//	const desync_side_t new_desync_side = ( delta_yaw > N ( 0 ) ) ? desync_side_t::desync_right_max : desync_side_t::desync_left_max;
	//
	//	IF ( ( new_desync_side < desync_side_t::desync_middle && rdata::resolved_side_run [ idx ] > desync_side_t::desync_middle )
	//		|| ( new_desync_side > desync_side_t::desync_middle && rdata::resolved_side_run [ idx ] < desync_side_t::desync_middle ) )
	//		features::ragebot::get_misses ( idx ).bad_resolve = N ( 0 ); ENDIF;
	//	
	//	rdata::resolved_side [ idx ] = rdata::resolved_side_run [ idx ] = ( delta_yaw <= N ( 0 ) ) ? desync_side_t::desync_right_max : desync_side_t::desync_left_max;
	//
	//	rdata::new_resolve [ idx ] = true;
	//
	//	rdata::last_good_weight [ idx ] = false;
	//	rdata::last_bad_weight [ idx ] = true;
	//}
	///* jitter resolver */
	//ELSE {
		IF ( delta_yaw > N ( 1 ) && delta_yaw_old > N ( 1 )
			&& abs ( delta_yaw - delta_yaw_old ) > abs ( delta_eye_yaw )
			&& abs ( delta_yaw ) > N ( 45 ) && abs ( delta_yaw_old ) > N ( 45 ) && delta_yaw * delta_yaw_old < N ( 0 ) ) {
			float some_yaw = N ( 0 );

			IF ( delta_yaw - delta_yaw_old <= N ( 0 ) )
				some_yaw = rec.m_anim_state [ desync_side_t::desync_max ].m_eye_yaw - N ( 90 );
			ELSE
				some_yaw = rec.m_anim_state [ desync_side_t::desync_max ].m_eye_yaw + N ( 90 );
			ENDIF;

			some_yaw = cs::normalize ( some_yaw );

			float v41 = angle_diff1 ( some_yaw, rec.m_anim_state [ desync_side_t::desync_max ].m_abs_yaw );
			float v42 = angle_diff1 ( some_yaw, anim_info [ idx ][ 0 ].m_anim_state [ desync_side_t::desync_max ].m_abs_yaw );

			rdata::resolved_side_run [ idx ] = ( abs ( v41 ) <= abs ( v42 ) ) ? desync_side_t::desync_right_max : desync_side_t::desync_middle;
			rdata::new_resolve [ idx ] = true;
			rdata::prefer_edge [ idx ] = false;

			IF ( speed_2d > max_speed * 0.9f )
				rdata::was_moving [ idx ] = true; ENDIF;
		}
		ELSE {
			IF ( !!( rec.m_flags & flags_t::on_ground ) && !!( anim_info [ idx ][ N ( 0 ) ].m_flags & flags_t::on_ground ) ) {
				IF ( speed_2d <= 5.0f ) {
					IF ( rdata::was_moving [ idx ] ) {
						IF ( !rdata::new_resolve [ idx ] )
							rdata::prefer_edge [ idx ] = true; ENDIF;

						rdata::new_resolve [ idx ] = false;
						rdata::was_moving [ idx ] = false;
					} ENDIF;
					
					IF ( anim_layers [ N ( 6 ) ].m_weight <= N( 0 ) && anim_layers [ N ( 6 ) ].m_playback_rate > N( 0 ) && anim_layers [ N ( 6 ) ].m_playback_rate < 0.0001f && anim_info [ idx ].size( ) >= N( 3 ) ) {
						const auto rate_delta_1 = abs ( anim_layers [ N ( 6 ) ].m_playback_rate - anim_info [ idx ][ N ( 0 ) ].m_anim_layers [ desync_side_t::desync_max ][ N ( 6 ) ].m_playback_rate );
						const auto rate_delta_2 = abs ( anim_info [ idx ][ N ( 0 ) ].m_anim_layers [ desync_side_t::desync_max ][ N ( 6 ) ].m_playback_rate - anim_info [ idx ][ N ( 1 ) ].m_anim_layers [ desync_side_t::desync_max ][ N ( 6 ) ].m_playback_rate );
					
						IF ( anim_info [ idx ][ N ( 0 ) ].m_anim_layers [ desync_side_t::desync_max ][ N ( 6 ) ].m_weight <= N ( 0 )
							&& anim_info [ idx ][ N ( 1 ) ].m_anim_layers [ desync_side_t::desync_max ][ N ( 6 ) ].m_weight <= N ( 0 )
							&& anim_info [ idx ][ N ( 2 ) ].m_anim_layers [ desync_side_t::desync_max ][ N ( 6 ) ].m_weight <= N ( 0 ) ) {
							IF ( rate_delta_1 >= 0.000003f && rate_delta_2 >= 0.000003f ) {
								const auto side1 = anim_layers [ N ( 6 ) ].m_playback_rate >= anim_info [ idx ][ N ( 0 ) ].m_anim_layers [ desync_side_t::desync_max ][ N ( 6 ) ].m_playback_rate
									&& anim_layers [ N ( 6 ) ].m_playback_rate <= anim_info [ idx ][ N ( 1 ) ].m_anim_layers [ desync_side_t::desync_max ][ N ( 6 ) ].m_playback_rate
									&& anim_layers [ N ( 6 ) ].m_playback_rate >= anim_info [ idx ][ N ( 2 ) ].m_anim_layers [ desync_side_t::desync_max ][ N ( 6 ) ].m_playback_rate;
								const auto side2 = anim_layers [ N ( 6 ) ].m_playback_rate <= anim_info [ idx ][ N ( 0 ) ].m_anim_layers [ desync_side_t::desync_max ][ N ( 6 ) ].m_playback_rate
									&& anim_layers [ N ( 6 ) ].m_playback_rate >= anim_info [ idx ][ N ( 1 ) ].m_anim_layers [ desync_side_t::desync_max ][ N ( 6 ) ].m_playback_rate
									&& anim_layers [ N ( 6 ) ].m_playback_rate <= anim_info [ idx ][ N ( 2 ) ].m_anim_layers [ desync_side_t::desync_max ][ N ( 6 ) ].m_playback_rate;
					
								IF ( side1 || side2 ) {
									rdata::resolved_jitter [ idx ] = true;
									rdata::jitter_sync [ idx ] = side1;
					
									rdata::resolved_side_jitter1 [ idx ] = rdata::resolved_side_run [ idx ];
					
									IF ( rdata::resolved_side_jitter1 [ idx ] >= desync_side_t::desync_middle )
										rdata::resolved_side_jitter2 [ idx ] = desync_side_t::desync_left_max;
									ELSE
										rdata::resolved_side_jitter2 [ idx ] = desync_side_t::desync_right_max;
									ENDIF;
								} ENDIF;
							}
							ELSE {
								IF ( rate_delta_1 < 0.000003f && rate_delta_2 < 0.000003f )
									rdata::resolved_jitter [ idx ] = false; ENDIF;
							} ENDIF;
						} ENDIF;
					}
					ELSE {
						const auto lby_delta_yaw = angle_diff1 ( rec.m_anim_state [ desync_side_t::desync_max ].m_eye_yaw, rec.m_lby );

						IF ( anim_layers [ N ( 3 ) ].m_weight <= N ( 0 )
							&& anim_layers [ N ( 3 ) ].m_cycle <= N ( 0 )
							&& anim_layers [ N ( 6 ) ].m_weight <= N ( 0 )
							&& anim_layers [ N ( 6 ) ].m_playback_rate <= N ( 0 )
							&& abs ( lby_delta_yaw ) > N ( 35 ) ) {
							desync_side_t new_desync_side = desync_side_t::desync_middle;

							IF ( lby_delta_yaw > N ( 0 ) && rdata::resolved_side_run [ idx ] < desync_side_t::desync_middle ) {
								new_desync_side = rdata::resolved_side_run [ idx ];
							} ELSE {
								IF ( lby_delta_yaw <= N ( 0 ) && rdata::resolved_side_run [ idx ] >= desync_side_t::desync_middle )
									new_desync_side = rdata::resolved_side_run [ idx ];
								ELSE
									new_desync_side = ( lby_delta_yaw <= N ( 0 ) ) ? desync_side_t::desync_right_max : desync_side_t::desync_left_max;
								ENDIF;
							} ENDIF;

							IF ( rdata::resolved_side_run [ idx ] != new_desync_side )
								features::ragebot::get_misses ( idx ).bad_resolve = N ( 0 ); ENDIF;

							rdata::resolved_side_run [ idx ] = new_desync_side;
							rdata::new_resolve [ idx ] = true;
							rdata::prefer_edge [ idx ] = false;
						}
						ELSE {
							/* resolve center-real / anti-alignment on standing */
							IF ( !rdata::new_resolve [ idx ]
								&& !features::ragebot::get_misses ( idx ).bad_resolve
								&& anim_layers [ N ( 6 ) ].m_weight <= N ( 0 )
								&& anim_layers [ N ( 6 ) ].m_playback_rate > N ( 0 )
								&& anim_layers [ N ( 6 ) ].m_playback_rate <= 0.06f
								&& abs ( delta_yaw ) < N ( 35 ) && abs ( delta_yaw_old ) < N ( 35 )
								&& abs ( delta_eye_yaw ) < N ( 26 )
								&& abs ( delta_at_target ) > N ( 12 ) && abs ( delta_at_target ) < N ( 76 ) ) {
								desync_side_t new_desync_side = desync_side_t::desync_middle;

								IF ( delta_at_target <= N ( 0 ) )
									new_desync_side = delta_at_target >= -N ( 44 ) ? desync_side_t::desync_right_half : desync_side_t::desync_right_max;
								ELSE
									new_desync_side = delta_at_target <= N ( 44 ) ? desync_side_t::desync_left_half : desync_side_t::desync_left_max;
								ENDIF;

								rdata::resolved_side_run [ idx ] = new_desync_side;
								rdata::new_resolve [ idx ] = true;
								rdata::prefer_edge [ idx ] = false;
							} ENDIF;
						} ENDIF;
					} ENDIF;

					rdata::last_good_weight [ idx ] = false;
				}
				/* moving */
				ELSE {
					const auto& prev_rec = anim_info [ idx ][ 0 ];

					const bool is_running = anim_layers [ N ( 11 ) ].m_weight <= 0.0f;
					const bool antiaim_disabled = abs ( rec.m_angles.x ) < 70.0f && abs ( prev_rec.m_angles.x ) < 70.0f && !rec.m_shot && !prev_rec.m_shot;

					bool side_changed = false;
					desync_side_t& desync_side = antiaim_disabled ? rdata::resolved_side [ idx ] : rdata::resolved_side_run [ idx ];
					const desync_side_t backup_desync_side = desync_side;

					/* TEST */
					const auto middle_delta_weight = abs ( anim_layers [ N ( 6 ) ].m_weight - rec.m_anim_layers [ desync_side_t::desync_middle ][ N ( 6 ) ].m_weight ) * N ( 1000 );
					const auto left_delta_weight = abs ( anim_layers [ N ( 6 ) ].m_weight - rec.m_anim_layers [ desync_side_t::desync_left_max ][ N ( 6 ) ].m_weight ) * N ( 1000 );
					const auto right_delta_weight = abs ( anim_layers [ N ( 6 ) ].m_weight - rec.m_anim_layers [ desync_side_t::desync_right_max ][ N ( 6 ) ].m_weight ) * N ( 1000 );
					const auto smallest_weight = std::min ( middle_delta_weight, std::min ( left_delta_weight, right_delta_weight ) );
					
					auto& default_animstate = rec.m_anim_state [ desync_side_t::desync_max ];
					auto running_speed = std::clamp ( default_animstate.m_run_speed, 0.0f, 1.0f );
					auto yaw_modifier = ( ( ( default_animstate.m_ground_fraction * -0.3f ) - 0.2f ) * running_speed ) + 1.0f;

					if ( default_animstate.m_duck_amount > 0.0f ) {
						auto speed_factor = std::clamp ( default_animstate.m_unk_feet_speed_ratio, 0.0f, 1.0f );
						yaw_modifier += ( ( default_animstate.m_duck_amount * speed_factor ) * ( 0.5f - yaw_modifier ) );
					}

					/* moving resolver */
					const auto weight_delta = abs ( anim_layers [ N ( 6 ) ].m_weight - prev_rec.m_anim_layers [ desync_side_t::desync_max ][ N ( 6 ) ].m_weight ) * N ( 1000 );
					
					// change resolver tolerance based on weight consistency
					// if delta < X, max_weight = 3, else max_weight = 1
					// check for layer 12 weight while slow walking/changing direction

					IF ( anim_layers [ N ( 6 ) ].m_weight > 0.01f ) {
						const auto middle_delta_weight = abs ( anim_layers [ N ( 6 ) ].m_weight - rec.m_anim_layers [ desync_side_t::desync_middle ][ N ( 6 ) ].m_weight ) * N ( 1000 );
						const auto left_delta_weight = abs ( anim_layers [ N ( 6 ) ].m_weight - rec.m_anim_layers [ desync_side_t::desync_left_max ][ N ( 6 ) ].m_weight ) * N ( 1000 );
						const auto right_delta_weight = abs ( anim_layers [ N ( 6 ) ].m_weight - rec.m_anim_layers [ desync_side_t::desync_right_max ][ N ( 6 ) ].m_weight ) * N ( 1000 );

						const auto middle_delta_rate = abs ( anim_layers [ N ( 6 ) ].m_playback_rate - rec.m_anim_layers [ desync_side_t::desync_middle ][ N ( 6 ) ].m_playback_rate ) * N ( 1000 );
						const auto left_delta_rate = abs ( anim_layers [ N ( 6 ) ].m_playback_rate - rec.m_anim_layers [ desync_side_t::desync_left_max ][ N ( 6 ) ].m_playback_rate ) * N ( 1000 );
						const auto right_delta_rate = abs ( anim_layers [ N ( 6 ) ].m_playback_rate - rec.m_anim_layers [ desync_side_t::desync_right_max ][ N ( 6 ) ].m_playback_rate ) * N ( 1000 );

						IF ( right_delta_rate < left_delta_rate && right_delta_rate < middle_delta_rate && right_delta_weight < 1.0f ) {
							IF ( desync_side != desync_side_t::desync_right_max )
								side_changed = true; ENDIF;

							desync_side = desync_side_t::desync_right_max;

							rdata::new_resolve [ idx ] = true;
							rdata::prefer_edge [ idx ] = false;
							rec.m_resolved = true;
						}
						ELSE {
							IF ( anim_layers [ N ( 6 ) ].m_weight < 0.95f
								&& ( ( left_delta_rate < middle_delta_rate && left_delta_rate < right_delta_rate && left_delta_weight < 1.0f )
								|| ( middle_delta_rate < left_delta_rate && middle_delta_rate < right_delta_rate && middle_delta_weight < 1.0f ) )
								&& abs ( left_delta_rate - middle_delta_rate ) < ( ( left_delta_rate + middle_delta_rate ) * 0.5f ) * 0.75f ) {
								IF ( desync_side != desync_side_t::desync_left_half )
									side_changed = true; ENDIF;

								desync_side = desync_side_t::desync_left_half;

								rdata::new_resolve [ idx ] = true;
								rdata::prefer_edge [ idx ] = false;
								rec.m_resolved = true;
							}
							ELSE {
								IF ( left_delta_rate < middle_delta_rate && left_delta_rate < right_delta_rate && left_delta_weight < 1.0f ) {
									IF ( desync_side != desync_side_t::desync_left_max )
										side_changed = true; ENDIF;

									desync_side = desync_side_t::desync_left_max;

									rdata::new_resolve [ idx ] = true;
									rdata::prefer_edge [ idx ] = false;
									rec.m_resolved = true;
								} ELSE {
									IF ( middle_delta_rate < left_delta_rate && middle_delta_rate < right_delta_rate && middle_delta_weight < 1.0f ) {
										IF ( desync_side != desync_side_t::desync_middle )
											side_changed = true; ENDIF;
									
										desync_side = desync_side_t::desync_middle;
									
										rdata::new_resolve [ idx ] = true;
										rdata::prefer_edge [ idx ] = false;
										rec.m_resolved = true;
									} ENDIF;
								} ENDIF;
							} ENDIF;
						} ENDIF;

						IF ( ( backup_desync_side >= desync_side_t::desync_middle && desync_side < desync_side_t::desync_middle ) || ( backup_desync_side < desync_side_t::desync_middle && desync_side >= desync_side_t::desync_middle ) )
							features::ragebot::get_misses ( idx ).bad_resolve = N ( 0 ); ENDIF;

						IF ( is_running )
							rdata::was_moving [ idx ] = true; ENDIF;

						IF ( side_changed && is_running ) {
							IF ( rdata::last_good_weight [ idx ] ) {
								rdata::resolved_jitter [ idx ] = true;
								rdata::jitter_sync [ idx ] = cs::time2ticks ( rec.m_simtime ) % N ( 2 );
							} ENDIF;

							rdata::last_good_weight [ idx ] = true;
						}
						ELSE {
							rdata::last_good_weight [ idx ] = false;
						} ENDIF;

						IF ( !side_changed && is_running ) {
							IF ( rdata::last_bad_weight [ idx ] )
								rdata::resolved_jitter [ idx ] = false; ENDIF;

							rdata::last_bad_weight [ idx ] = true;
						}
						ELSE {
							rdata::last_bad_weight [ idx ] = false;
						} ENDIF;

						IF ( rdata::resolved_jitter [ idx ] && is_running ) {
							IF ( rdata::jitter_sync [ idx ] == cs::time2ticks ( rec.m_simtime ) % 2 )
								rdata::resolved_side_jitter1 [ idx ] = desync_side;
							ELSE
								rdata::resolved_side_jitter2 [ idx ] = desync_side;
							ENDIF;
						} ENDIF;
					} ENDIF;
				} ENDIF;
			} ENDIF;
		} ENDIF;
	//} ENDIF;

	//const auto eyes_max = ent->eyes ( );
	//const auto eyes_fwd = cs::angle_vec ( vec3_t ( 0.0f, ent->angles ( ).y, 0.0f ) ).normalized ( );
	//const auto eyes_right = eyes_fwd.cross_product ( vec3_t ( N ( 0 ), N ( 0 ), N ( 1 ) ) );
	//
	//auto left_head = eyes_max + eyes_fwd * N ( 30 ) + eyes_right * N ( 10 );
	//auto center_head = eyes_max + eyes_fwd * N ( 30 );
	//auto right_head = eyes_max + eyes_fwd * N ( 30 ) - eyes_right * N ( 10 );
	//
	//const auto left_visible = cs::is_visible ( left_head );
	//const auto center_visible = cs::is_visible ( center_head );
	//const auto right_visible = cs::is_visible ( right_head );

	/* only do spotted resolver if one side is visible */
	//IF ( ( left_visible && !right_visible )
	//	|| ( !left_visible && right_visible ) ) {
	//	const auto spotted = is_spotted ( g::local, ent );
	//
	//	dbg_print ( _("%d\n"), spotted );
	//
	//	/* check side based on visibility of head */
	//	desync_side_t wanted_side = desync_side_t::desync_middle;
	//
	//	if ( spotted )
	//		wanted_side = left_visible ? desync_side_t::desync_left_max : desync_side_t::desync_right_max;
	//	else
	//		wanted_side = left_visible ? desync_side_t::desync_right_max : desync_side_t::desync_left_max;
	//
	//	/* if already resolved to correct side, use current resolve */
	//	desync_side_t new_desync_side = desync_side_t::desync_middle;
	//
	//	IF ( wanted_side < desync_side_t::desync_middle && rdata::resolved_side_run [ idx ] < desync_side_t::desync_middle ) {
	//		new_desync_side = rdata::resolved_side_run [ idx ];
	//	} ELSE {
	//		IF ( wanted_side >= desync_side_t::desync_middle && rdata::resolved_side_run [ idx ] >= desync_side_t::desync_middle )
	//			new_desync_side = rdata::resolved_side_run [ idx ];
	//		ELSE
	//			new_desync_side = wanted_side;
	//		ENDIF;
	//	} ENDIF;
	//
	//	IF ( rdata::resolved_side_run [ idx ] != new_desync_side )
	//		features::ragebot::get_misses ( idx ).bad_resolve = N ( 0 ); ENDIF;
	//
	//	rdata::resolved_side_run [ idx ] = new_desync_side;
	//	rdata::new_resolve [ idx ] = true;
	//	rdata::prefer_edge [ idx ] = false;
	//} ENDIF;

	//IF ( ( ( left_visible && !right_visible )
	//	|| ( !left_visible && right_visible ) )
	//	&& !features::ragebot::get_misses ( idx ).bad_resolve
	//	&& speed_2d <= max_speed * 0.34f ) {
	//	const auto spotted = is_spotted ( g::local, ent );
	//
	//	/* check side based on visibility of head */
	//	desync_side_t wanted_side = left_visible ? desync_side_t::desync_right_max : desync_side_t::desync_left_max;
	//
	//	/* if already resolved to correct side, use current resolve */
	//	desync_side_t new_desync_side = desync_side_t::desync_middle;
	//
	//	IF ( wanted_side < desync_side_t::desync_middle && rdata::resolved_side_run [ idx ] < desync_side_t::desync_middle ) {
	//		new_desync_side = rdata::resolved_side_run [ idx ];
	//	} ELSE {
	//		IF ( wanted_side >= desync_side_t::desync_middle && rdata::resolved_side_run [ idx ] >= desync_side_t::desync_middle )
	//			new_desync_side = rdata::resolved_side_run [ idx ];
	//		ELSE
	//			new_desync_side = wanted_side;
	//		ENDIF;
	//	} ENDIF;
	//
	//	IF ( rdata::resolved_side_run [ idx ] != new_desync_side )
	//		features::ragebot::get_misses ( idx ).bad_resolve = N ( 0 ); ENDIF;
	//
	//	rdata::resolved_side_run [ idx ] = new_desync_side;
	//	rdata::new_resolve [ idx ] = true;
	//	rdata::prefer_edge [ idx ] = false;
	//} ENDIF;

	/* try to hide head */
	//const auto right_dir = fwd.cross_product ( vec3_t ( N ( 0 ), N ( 0 ), N ( 1 ) ) );
	//const auto left_dir = -right_dir;
	//const auto dmg_left = autowall::dmg ( g::local, ent, src + left_dir * N ( 35 ), eyes_max + fwd * N ( 30 ) + left_dir * N ( 10 ), N( 0 ) ) + autowall::dmg ( g::local, ent, src - left_dir * N ( 10 ), eyes_max + fwd * N ( 35 ) + left_dir * N ( 10 ), N ( 0 ) /* pretend player would be there */ );
	//const auto dmg_right = autowall::dmg ( g::local, ent, src + right_dir * N ( 35 ), eyes_max + fwd * N ( 30 ) + right_dir * N ( 10 ), N ( 0 ) ) + autowall::dmg ( g::local, ent, src - right_dir * N ( 10 ), eyes_max + fwd * N ( 35 ) + right_dir * N ( 10 ), N ( 0 ) /* pretend player would be there */ );
	//const auto one_side_hittable = ( dmg_left && !dmg_right ) || ( !dmg_left && dmg_right );
	//const auto occluded_side = ( dmg_left > dmg_right ) ? desync_side_t::desync_left_max : desync_side_t::desync_right_max;

	/* resolve standing */
	auto target_side_tmp = apply_antiaim ( ent, rec, speed_2d, max_speed );

	/* edging players */
	//IF ( rdata::prefer_edge [ idx ] && one_side_hittable && autowall::dmg ( g::local, ent, src, rec.m_aim_bones [ desync_side_t::desync_max ][ N ( 8 ) ].origin ( ), hitbox_head ) > N ( 0 ) ) {
	//	IF ( occluded_side < desync_side_t::desync_middle && target_side_tmp < desync_side_t::desync_middle ) {
	//		target_side_tmp = rdata::resolved_side [ idx ];
	//	}
	//	ELSE {
	//		IF ( occluded_side > desync_side_t::desync_middle && target_side_tmp > desync_side_t::desync_middle )
	//			target_side_tmp = rdata::resolved_side [ idx ];
	//		ELSE
	//			target_side_tmp = occluded_side;
	//		ENDIF;
	//	} ENDIF;
	//} ENDIF;

	const auto has_accurate_weapon = g::local && g::local->weapon ( ) && g::local->weapon ( )->item_definition_index ( ) == weapons_t::ssg08;
	const auto has_bad_desync = /*ent->desync_amount ( ) <= 34.0f*/false;

	//CASE ( features::ragebot::get_misses ( idx ).bad_resolve % N( 4 ) )
	//	WHEN ( N ( 0 ) ) DO bruteforce ( has_bad_desync ? N ( 3 ) : N ( 0 ), target_side_tmp, rec ); BREAK; DONE
	//	WHEN ( N ( 1 ) ) DO bruteforce ( has_bad_desync ? N ( 4 ) : ( has_accurate_weapon ? N ( 1 ) : N ( 2 ) ), target_side_tmp, rec ); BREAK; DONE
	//	WHEN ( N ( 2 ) ) DO bruteforce ( has_bad_desync ? N ( 3 ) : ( has_accurate_weapon ? N ( 2 ) : N ( 1 ) ), target_side_tmp, rec ); BREAK; DONE
	//	WHEN ( N ( 3 ) ) DO bruteforce ( N ( 4 ), target_side_tmp, rec ); BREAK; DONE
	//	DEFAULT bruteforce ( has_bad_desync ? N ( 3 ) : N ( 0 ), target_side_tmp, rec ); DONE
	//ENDCASE;

	switch ( features::ragebot::get_misses ( idx ).bad_resolve % 4 ) {
	case 0: bruteforce ( has_bad_desync ? 3 : 0, target_side_tmp, rec ); break;
	case 1: bruteforce ( has_bad_desync ? 4 : ( has_accurate_weapon ? 1 : 2 ), target_side_tmp, rec ); break;
	case 2: bruteforce ( has_bad_desync ? 3 : ( has_accurate_weapon ? 2 : 1 ), target_side_tmp, rec ); break;
	case 3: bruteforce ( 4, target_side_tmp, rec ); break;
	default: bruteforce ( has_bad_desync ? 3 : 0, target_side_tmp, rec ); break;
	}

	RETURN( true );
	OBF_END;
}

#pragma optimize( "2", on )