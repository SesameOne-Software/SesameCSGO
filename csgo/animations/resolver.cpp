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
	
	char buffer [ 256 ] = { 0 };
	strcpy ( buffer, "con_filter_text \"" );
	sprintf ( &buffer[ sizeof ( "con_filter_text " ) - 1], fmt, args... );
	strcat ( buffer, "\"" );

	cs::i::engine->client_cmd_unrestricted ( buffer );

	con_color_msg ( s_clr, fmt, args... );
}

#pragma optimize( "2", off )

// FIXME!!!
bool anims::resolver::process_blood ( const effect_data_t& effect_data ) {
	auto hit_player = cs::i::ent_list->get_by_handle< player_t* > ( effect_data.m_ent_handle );
	
	if ( !hit_player || !hit_player->is_player ( ) )
		return false;
	
	auto idx = hit_player->idx ( );

	std::vector<const features::ragebot::shot_t*> shots { };
	features::ragebot::get_shots ( idx, shots );

	if ( shots.empty ( ) )
		return false;

	const auto last_shot = shots.front ( );

	if ( last_shot->hitbox == hitbox_t::r_foot
		|| last_shot->hitbox == hitbox_t::l_foot
		|| last_shot->hitbox == hitbox_t::r_calf
		|| last_shot->hitbox == hitbox_t::l_calf )
		return false;

	auto mdl = hit_player->mdl ( );

	if ( !mdl )
		return false;

	auto studio_mdl = cs::i::mdl_info->studio_mdl ( mdl );

	if ( !studio_mdl )
		return false;

	auto set = studio_mdl->hitbox_set ( N( 0 ) );

	if ( !set )
		return false;

	auto hitbox = set->hitbox ( static_cast<int>( last_shot->hitbox ) );

	if ( !hitbox )
		return false;

	vec3_t hitbox_pos_hit;
	
	if ( hitbox->m_radius > N(0) )
		hitbox_pos_hit = effect_data.m_origin - effect_data.m_normal * hitbox->m_radius;
	else
		hitbox_pos_hit = effect_data.m_origin;

	const auto angle_to_real_hitbox = cs::calc_angle ( g::local->eyes ( ), hitbox_pos_hit );
	const auto angle_to_player = cs::calc_angle ( g::local->eyes ( ), last_shot->rec.m_origin );

	const auto delta_hitbox_angle = cs::normalize ( angle_to_real_hitbox.y - angle_to_player.y );

	//const auto real_hitbox_side = delta_hitbox_angle > N ( 0 ) ? desync_side_t::desync_right_max : desync_side_t::desync_left_max;
	//
	//if ( ( rdata::resolved_side_run [ V ( idx ) ] >= desync_side_t::desync_middle && real_hitbox_side < desync_side_t::desync_middle )
	//	|| ( rdata::resolved_side_run [ V ( idx ) ] < desync_side_t::desync_middle && real_hitbox_side >= desync_side_t::desync_middle )) {
	//	features::ragebot::get_misses ( V ( idx ) ).bad_resolve = N ( 0 );
	//
	//	rdata::resolved_side_run [ V ( idx ) ] = real_hitbox_side;
	//	rdata::resolved_jitter [ V ( idx ) ] = false;
	//	rdata::new_resolve [ V ( idx ) ] = true;
	//	rdata::prefer_edge [ V ( idx ) ] = false;
	//
	//	//dbg_print ( _("-- BLOOD RESOLVED ACTIVE!! --") );
	//}

	return true;
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
	player->bone_cache ( ) = shot->rec.m_aim_bones.data ( );
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
void anims::resolver::process_hurt ( event_t* event ) {
	static auto& hit_sound = options::vars [ _ ( "visuals.other.hit_sound" ) ].val.i;

	auto dmg = event->get_int ( _ ( "dmg_health" ) );
	auto attacker = cs::i::ent_list->get< player_t* > ( cs::i::engine->get_player_for_userid ( event->get_int ( _ ( "attacker" ) ) ) );
	auto victim = cs::i::ent_list->get< player_t* > ( cs::i::engine->get_player_for_userid ( event->get_int ( _ ( "userid" ) ) ) );
	auto hitgroup = event->get_int ( _ ( "hitgroup" ) );

	if ( !attacker || !victim || attacker != g::local || !g::local->is_enemy( victim ) || victim->idx ( ) <= 0 || victim->idx ( ) > 64 )
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
	VMP_BEGINMUTATION ( );
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
								VEC_TRANSFORM ( hhitbox->m_bbmin, shot.rec.m_aim_bones[ hhitbox->m_bone ], vmin );
								VEC_TRANSFORM ( hhitbox->m_bbmax, shot.rec.m_aim_bones[ hhitbox->m_bone ], vmax );

								if ( autowall::trace_ray ( vmin, vmax, shot.rec.m_aim_bones[ hhitbox->m_bone ], hhitbox->m_radius, shot.src, shot.processed_impact_pos ) )
									found_hitbox = true;
							}
						}
					}

					if ( shot.processed_impact_dmg ) {
						reason = _ ( "bad anims" );
						features::ragebot::get_misses ( player->idx ( ) ).bad_resolve++;
					}
					else if ( (!g::cvars::weapon_accuracy_nospread->get_bool ( ) || found_hitbox )
						&& g::local->velocity_modifier ( ) < shot.vel_modifier ) {
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
			else
				print_console ( info_color, _ ( ", hc: auto" ) );

			if ( shot.processed_hurt && g::local->alive ( ) )
				print_console ( info_color, _ ( ", dmg: %dhp" ), shot.processed_dmg );

			if ( shot.dmg != -1 )
				print_console ( info_color, _ ( ", tdmg: %dhp" ), shot.dmg );

			print_console ( info_color, _ ( ", desync: %d°" ), static_cast< int >( shot.body_yaw + 0.5f ) );

			print_console ( info_color, _ ( ", fl: " ) );

			if ( !( shot.rec.m_flags & flags_t::on_ground ) )
				print_console ( info_color, _ ( "air" ) );
			else if ( shot.rec.m_anim_layers [ 6 ].m_weight <= 0.01f )
				print_console ( info_color, _ ( "stand" ) );
			else if ( shot.rec.m_anim_layers [ 6 ].m_weight < 0.95f )
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

	VMP_END ( );
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

bool anims::resolver::is_spotted ( player_t* src, player_t* dst ) {
	static const auto spotted_by_mask_off = netvars::get_offset ( _ ( "DT_BaseEntity->m_bSpottedByMask" ) );

	auto idx = dst->idx ( );

	if ( idx > N ( 0 ) && idx <= cs::i::globals->m_max_clients )
		return *reinterpret_cast< uint32_t* >( reinterpret_cast< uintptr_t >( dst ) + spotted_by_mask_off ) & ( N ( 1 ) << ( src->idx ( ) - 1 ) );

	return false;
}