#include "other_visuals.hpp"
#include "../globals.hpp"
#include "lagcomp.hpp"
#include <deque>
#include <mutex>
#include "../menu/options.hpp"
#include "esp.hpp"

#include "../renderer/render.hpp"
#include "../fmt/format.h"

float features::spread_circle::total_spread = 0.0f;

bool features::get_visuals( player_t* pl, visual_config_t& out ) {
	memset( &out, 0, sizeof out );

	if ( !pl || !pl->is_player( ) )
		return false;

	if ( pl == g::local ) {
		MUTATE_START
		static auto& options = options::vars [ _( "visuals.local.options" ) ].val.l;
		static auto& health_bar_placement = options::vars [ _( "visuals.local.health_bar_location" ) ].val.i;
		static auto& desync_bar_placement = options::vars [ _( "visuals.local.desync_bar_location" ) ].val.i;
		static auto& ammo_bar_placement = options::vars [ _( "visuals.local.ammo_bar_location" ) ].val.i;
		static auto& value_text_placement = options::vars [ _( "visuals.local.value_text_location" ) ].val.i;
		static auto& nametag_placement = options::vars [ _( "visuals.local.nametag_location" ) ].val.i;
		static auto& weapon_name_placement = options::vars [ _( "visuals.local.weapon_name_location" ) ].val.i;
		static auto& reflectivity = options::vars [ _( "visuals.local.reflectivity" ) ].val.f;
		static auto& phong = options::vars [ _( "visuals.local.phong" ) ].val.f;
		static auto& chams_color = options::vars [ _( "visuals.local.chams_color" ) ].val.c;
		static auto& xqz_chams_color = options::vars [ _( "visuals.local.xqz_chams_color" ) ].val.c;
		static auto& backtrack_chams_color = options::vars [ _( "visuals.local.backtrack_chams_color" ) ].val.c;
		static auto& hit_matrix_color = options::vars [ _( "visuals.local.hit_matrix_color" ) ].val.c;
		static auto& glow_color = options::vars [ _( "visuals.local.glow_color" ) ].val.c;
		static auto& rimlight_overlay_color = options::vars [ _( "visuals.local.rimlight_overlay_color" ) ].val.c;
		static auto& box_color = options::vars [ _( "visuals.local.box_color" ) ].val.c;
		static auto& health_bar_color = options::vars [ _( "visuals.local.health_bar_color" ) ].val.c;
		static auto& ammo_bar_color = options::vars [ _( "visuals.local.ammo_bar_color" ) ].val.c;
		static auto& desync_bar_color = options::vars [ _( "visuals.local.desync_bar_color" ) ].val.c;
		static auto& name_color = options::vars [ _( "visuals.local.name_color" ) ].val.c;
		static auto& weapon_color = options::vars [ _( "visuals.local.weapon_color" ) ].val.c;
		static auto& desync_chams_color = options::vars [ _( "visuals.local.desync_chams_color" ) ].val.c;
		static auto& desync_rimlight_overlay_color = options::vars [ _( "visuals.local.desync_rimlight_overlay_color" ) ].val.c;

		out.reflectivity = reflectivity;
		out.phong = phong;
		out.chams = options [ 0 ];
		out.chams_flat = options [ 1 ];
		out.chams_xqz = options [ 2 ];
		out.desync_chams = options [ 3 ];
		out.desync_chams_fakelag = options [ 4 ];
		out.desync_chams_rimlight = options [ 5 ];
		out.glow = options [ 6 ];
		out.rimlight_overlay = options [ 7 ];
		out.esp_box = options [ 8 ];
		out.health_bar = options [ 9 ];
		out.ammo_bar = options [ 10 ];
		out.desync_bar = options [ 11 ];
		out.value_text = options [ 12 ];
		out.nametag = options [ 13 ];
		out.weapon_name = options [ 14 ];
		out.health_bar_placement = health_bar_placement;
		out.ammo_bar_placement = ammo_bar_placement;
		out.desync_bar_placement = desync_bar_placement;
		out.value_text_placement = value_text_placement;
		out.nametag_placement = nametag_placement;
		out.weapon_name_placement = weapon_name_placement;
		out.chams_color = chams_color;
		out.chams_xqz_color = xqz_chams_color;
		out.backtrack_chams_color = backtrack_chams_color;
		out.hit_matrix_color = hit_matrix_color;
		out.glow_color = glow_color;
		out.rimlight_color = rimlight_overlay_color;
		out.box_color = box_color;
		out.health_bar_color = health_bar_color;
		out.ammo_bar_color = ammo_bar_color;
		out.desync_bar_color = desync_bar_color;
		out.name_color = name_color;
		out.weapon_color = weapon_color;
		out.chams_color = chams_color;
		out.desync_chams_color = desync_chams_color;
		out.desync_rimlight_color = desync_rimlight_overlay_color;
		MUTATE_END

		return true;
	}

	if ( g::local->team( ) != pl->team( ) ) {
		MUTATE_START
		static auto& options = options::vars [ _( "visuals.enemies.options" ) ].val.l;
		static auto& health_bar_placement = options::vars [ _( "visuals.enemies.health_bar_location" ) ].val.i;
		static auto& desync_bar_placement = options::vars [ _( "visuals.enemies.desync_bar_location" ) ].val.i;
		static auto& ammo_bar_placement = options::vars [ _( "visuals.enemies.ammo_bar_location" ) ].val.i;
		static auto& value_text_placement = options::vars [ _( "visuals.enemies.value_text_location" ) ].val.i;
		static auto& nametag_placement = options::vars [ _( "visuals.enemies.nametag_location" ) ].val.i;
		static auto& weapon_name_placement = options::vars [ _( "visuals.enemies.weapon_name_location" ) ].val.i;
		static auto& reflectivity = options::vars [ _( "visuals.enemies.reflectivity" ) ].val.f;
		static auto& phong = options::vars [ _( "visuals.enemies.phong" ) ].val.f;
		static auto& chams_color = options::vars [ _( "visuals.enemies.chams_color" ) ].val.c;
		static auto& xqz_chams_color = options::vars [ _( "visuals.enemies.xqz_chams_color" ) ].val.c;
		static auto& backtrack_chams_color = options::vars [ _( "visuals.enemies.backtrack_chams_color" ) ].val.c;
		static auto& hit_matrix_color = options::vars [ _( "visuals.enemies.hit_matrix_color" ) ].val.c;
		static auto& glow_color = options::vars [ _( "visuals.enemies.glow_color" ) ].val.c;
		static auto& rimlight_overlay_color = options::vars [ _( "visuals.enemies.rimlight_overlay_color" ) ].val.c;
		static auto& box_color = options::vars [ _( "visuals.enemies.box_color" ) ].val.c;
		static auto& health_bar_color = options::vars [ _( "visuals.enemies.health_bar_color" ) ].val.c;
		static auto& ammo_bar_color = options::vars [ _( "visuals.enemies.ammo_bar_color" ) ].val.c;
		static auto& desync_bar_color = options::vars [ _( "visuals.enemies.desync_bar_color" ) ].val.c;
		static auto& name_color = options::vars [ _( "visuals.enemies.name_color" ) ].val.c;
		static auto& weapon_color = options::vars [ _( "visuals.enemies.weapon_color" ) ].val.c;

		out.reflectivity = reflectivity;
		out.phong = phong;
		out.chams = options [ 0 ];
		out.chams_flat = options [ 1 ];
		out.chams_xqz = options [ 2 ];
		out.backtrack_chams = options [ 3 ];
		out.hit_matrix = options [ 4 ];
		out.glow = options [ 5 ];
		out.rimlight_overlay = options [ 6 ];
		out.esp_box = options [ 7 ];
		out.health_bar = options [ 8 ];
		out.ammo_bar = options [ 9 ];
		out.desync_bar = options [ 10 ];
		out.value_text = options [ 11 ];
		out.nametag = options [ 12 ];
		out.weapon_name = options [ 13 ];
		out.health_bar_placement = health_bar_placement;
		out.ammo_bar_placement = ammo_bar_placement;
		out.desync_bar_placement = desync_bar_placement;
		out.value_text_placement = value_text_placement;
		out.nametag_placement = nametag_placement;
		out.weapon_name_placement = weapon_name_placement;
		out.chams_color = chams_color;
		out.chams_xqz_color = xqz_chams_color;
		out.backtrack_chams_color = backtrack_chams_color;
		out.hit_matrix_color = hit_matrix_color;
		out.glow_color = glow_color;
		out.rimlight_color = rimlight_overlay_color;
		out.box_color = box_color;
		out.health_bar_color = health_bar_color;
		out.ammo_bar_color = ammo_bar_color;
		out.desync_bar_color = desync_bar_color;
		out.name_color = name_color;
		out.weapon_color = weapon_color;
		out.chams_color = chams_color;
		MUTATE_END

		return true;
	}

	MUTATE_START
	static auto& options = options::vars [ _( "visuals.teammates.options" ) ].val.l;
	static auto& health_bar_placement = options::vars [ _( "visuals.teammates.health_bar_location" ) ].val.i;
	static auto& desync_bar_placement = options::vars [ _( "visuals.teammates.desync_bar_location" ) ].val.i;
	static auto& ammo_bar_placement = options::vars [ _( "visuals.teammates.ammo_bar_location" ) ].val.i;
	static auto& value_text_placement = options::vars [ _( "visuals.teammates.value_text_location" ) ].val.i;
	static auto& nametag_placement = options::vars [ _( "visuals.teammates.nametag_location" ) ].val.i;
	static auto& weapon_name_placement = options::vars [ _( "visuals.teammates.weapon_name_location" ) ].val.i;
	static auto& reflectivity = options::vars [ _( "visuals.teammates.reflectivity" ) ].val.f;
	static auto& phong = options::vars [ _( "visuals.teammates.phong" ) ].val.f;
	static auto& chams_color = options::vars [ _( "visuals.teammates.chams_color" ) ].val.c;
	static auto& xqz_chams_color = options::vars [ _( "visuals.teammates.xqz_chams_color" ) ].val.c;
	static auto& backtrack_chams_color = options::vars [ _( "visuals.teammates.backtrack_chams_color" ) ].val.c;
	static auto& hit_matrix_color = options::vars [ _( "visuals.teammates.hit_matrix_color" ) ].val.c;
	static auto& glow_color = options::vars [ _( "visuals.teammates.glow_color" ) ].val.c;
	static auto& rimlight_overlay_color = options::vars [ _( "visuals.teammates.rimlight_overlay_color" ) ].val.c;
	static auto& box_color = options::vars [ _( "visuals.teammates.box_color" ) ].val.c;
	static auto& health_bar_color = options::vars [ _( "visuals.teammates.health_bar_color" ) ].val.c;
	static auto& ammo_bar_color = options::vars [ _( "visuals.teammates.ammo_bar_color" ) ].val.c;
	static auto& desync_bar_color = options::vars [ _( "visuals.teammates.desync_bar_color" ) ].val.c;
	static auto& name_color = options::vars [ _( "visuals.teammates.name_color" ) ].val.c;
	static auto& weapon_color = options::vars [ _( "visuals.teammates.weapon_color" ) ].val.c;

	out.reflectivity = reflectivity;
	out.phong = phong;
	out.chams = options [ 0 ];
	out.chams_flat = options [ 1 ];
	out.chams_xqz = options [ 2 ];
	out.glow = options [ 3 ];
	out.rimlight_overlay = options [ 4 ];
	out.esp_box = options [ 5 ];
	out.health_bar = options [ 6 ];
	out.ammo_bar = options [ 7 ];
	out.desync_bar = options [ 8 ];
	out.value_text = options [ 9 ];
	out.nametag = options [ 10 ];
	out.weapon_name = options [ 11 ];
	out.health_bar_placement = health_bar_placement;
	out.ammo_bar_placement = ammo_bar_placement;
	out.desync_bar_placement = desync_bar_placement;
	out.value_text_placement = value_text_placement;
	out.nametag_placement = nametag_placement;
	out.weapon_name_placement = weapon_name_placement;
	out.chams_color = chams_color;
	out.chams_xqz_color = xqz_chams_color;
	out.backtrack_chams_color = backtrack_chams_color;
	out.hit_matrix_color = hit_matrix_color;
	out.glow_color = glow_color;
	out.rimlight_color = rimlight_overlay_color;
	out.box_color = box_color;
	out.health_bar_color = health_bar_color;
	out.ammo_bar_color = ammo_bar_color;
	out.desync_bar_color = desync_bar_color;
	out.name_color = name_color;
	out.weapon_color = weapon_color;
	out.chams_color = chams_color;
	MUTATE_END

	return true;
}

void features::offscreen_esp::draw( ) {
	static auto& bomb_esp = options::vars [ _( "visuals.other.bomb_esp" ) ].val.b;
	static auto& bomb_timer = options::vars [ _( "visuals.other.bomb_timer" ) ].val.b;
	static auto& offscreen_esp = options::vars [ _( "visuals.other.offscreen_esp" ) ].val.b;
	static auto& offscreen_esp_distance = options::vars [ _( "visuals.other.offscreen_esp_distance" ) ].val.f;
	static auto& offscreen_esp_size = options::vars [ _( "visuals.other.offscreen_esp_size" ) ].val.f;
	static auto& offscreen_esp_color = options::vars [ _( "visuals.other.offscreen_esp_color" ) ].val.c;

	if ( !g::local )
		return;

	float w = 0, h = 0;
	render::screen_size( w, h );

	vec3_t center( w * 0.5f, h * 0.5f, 0.0f );

	const auto calc_distance = offscreen_esp_distance / 100.0f * ( h * 0.5f );

	for ( auto i = 1; i <= cs::i::ent_list->get_highest_index( ); i++ ) {
		const auto pl = cs::i::ent_list->get< player_t* >( i );

		if ( !pl || !pl->client_class( ) )
			continue;

		switch ( pl->client_class( )->m_class_id ) {
			case 40: {
				if ( pl->valid( ) && offscreen_esp && pl != g::local && pl->team( ) != g::local->team( ) ) {
					vec3_t screen;

					//auto interp_origin = lagcomp::data::cham_records [ pl->idx( ) ].m_bones1 [ 1 ].origin( );

					if ( !cs::render::world_to_screen( screen, pl->abs_origin( ) ) ) {
						auto target_ang = cs::vec_angle( center - screen );
						target_ang.y = cs::normalize( target_ang.y - 90.0f );

						auto top_ang = cs::vec_angle( vec3_t( 0.0f, -calc_distance - offscreen_esp_size * 0.5f ) ) + target_ang;
						auto left_ang = cs::vec_angle( vec3_t( -offscreen_esp_size * 0.5f, -calc_distance + offscreen_esp_size * 0.5f ) ) + target_ang;
						auto right_ang = cs::vec_angle( vec3_t( offscreen_esp_size * 0.5f, -calc_distance + offscreen_esp_size * 0.5f ) ) + target_ang;

						top_ang.y = cs::normalize( top_ang.y );
						left_ang.y = cs::normalize( left_ang.y );
						right_ang.y = cs::normalize( right_ang.y );

						const auto mag1 = calc_distance + offscreen_esp_size * 0.5f;
						const auto mag2 = calc_distance - offscreen_esp_size * 0.5f;

						const auto top_pos = center + cs::angle_vec( top_ang ) * mag1;
						const auto left_pos = center + cs::angle_vec( left_ang ) * mag2;
						const auto right_pos = center + cs::angle_vec( right_ang ) * mag2;

						render::polygon( { {top_pos.x, top_pos.y}, {left_pos.x, left_pos.y}, {right_pos.x, right_pos.y} }, rgba ( static_cast< int > ( offscreen_esp_color.r * 255.0f ), static_cast< int > ( offscreen_esp_color.g * 255.0f ), static_cast< int > ( offscreen_esp_color.b * 255.0f ), static_cast< int > ( offscreen_esp_color.a * 255.0f ) ), false );
						render::polygon( { {top_pos.x, top_pos.y}, {left_pos.x, left_pos.y}, {right_pos.x, right_pos.y} }, rgba ( static_cast< int > ( offscreen_esp_color.r * 255.0f ), static_cast< int > ( offscreen_esp_color.g * 255.0f ), static_cast< int > ( offscreen_esp_color.b * 255.0f ), 255 ), true, 2.5f );
					}
				}
			} break;
			case 128: {
				const auto as_bomb = reinterpret_cast< planted_c4_t* >( pl );

				if ( as_bomb ) {
					if ( !as_bomb->bomb_defused( ) ) {
						const auto timer = as_bomb->c4_blow( ) - cs::i::globals->m_curtime;
						const auto fraction = timer / 40.0f;

						if ( fraction >= 0.0f ) {
							if ( bomb_timer ) {
								const auto defuse_fraction = ( as_bomb->defuse_countdown( ) - cs::i::globals->m_curtime ) / as_bomb->defuse_length( );
								const auto time_left = fmt::format ( _("{:.2f} seconds"), timer );

								render::rect( 0, 1, w * fraction, 4, rgba ( 0, 255, 0, 255 ) );

								if ( reinterpret_cast< player_t* >( as_bomb->get_defuser( ) )->valid( ) )
									render::rect ( 0, 5, w * defuse_fraction, 4, rgba ( 84, 195, 255, 255 ) );

								vec3_t text_dim;
								render::text_size ( time_left, _("esp_font"), text_dim );
								render ::text( w / 2 - text_dim.x / 2, 65, time_left, _ ( "esp_font" ), rgba ( 255, 255, 255, 255 ) ,true);
							}

							if ( bomb_esp ) {
								auto c4_origin = as_bomb->origin( ) + vec3_t( 0.0f, 0.0f, 8.0f );

								vec3_t bomb_screen;
								const auto transformed = cs::render::world_to_screen( bomb_screen, c4_origin );

								vec3_t calc_pos;

								if ( !transformed ) {
									if ( offscreen_esp ) {
										auto target_ang = cs::vec_angle( center - bomb_screen );
										target_ang.y = cs::normalize( target_ang.y - 90.0f );

										auto top_ang = cs::vec_angle( vec3_t( 0.0f, -calc_distance - offscreen_esp_size * 0.5f ) ) + target_ang;

										top_ang.y = cs::normalize( top_ang.y );

										const auto mag1 = calc_distance + offscreen_esp_size * 0.5f;

										calc_pos = center + cs::angle_vec( top_ang ) * mag1;

										vec3_t text_dim;
										render::text_size ( _("!"), _ ( "indicator_font" ), text_dim );

										render::circle( calc_pos.x, calc_pos.y - text_dim.y / 2.0f, 18.0f, 32, rgba ( 19, 19, 19, 255 ) );
										render::circle( calc_pos.x, calc_pos.y - text_dim.y / 2.0f, 18.0f, 32, rgba ( 255, 0, 0, 255 ), true );
										render::circle( calc_pos.x, calc_pos.y - text_dim.y / 2.0f, 18.0f, 31, rgba ( 255, 0, 0, 255 ), true );
										render::text ( calc_pos.x - text_dim.x / 2.0f, calc_pos.y - text_dim.y, _( "!" ), _ ( "indicator_font" ), rgba ( 255, 0, 0, 255 ), true );
									}
								}
								else {
									calc_pos = bomb_screen;

									vec3_t text_dim;
									
									render::circle( calc_pos.x, calc_pos.y - text_dim.y / 2.0f, 18.0f, 32, rgba ( 19, 19, 19, 255 ) );
									render::circle( calc_pos.x, calc_pos.y - text_dim.y / 2.0f, 18.0f, 32, rgba ( 255, 0, 0, 255 ), true );
									render::circle( calc_pos.x, calc_pos.y - text_dim.y / 2.0f, 18.0f, 31, rgba ( 255, 0, 0, 255 ), true );
									render::text ( calc_pos.x - text_dim.x / 2.0f, calc_pos.y - text_dim.y, _( "!" ), _ ( "indicator_font" ), rgba ( 255, 0, 0, 255 ), true );
								}
							}
						}
					}
				}
			} break;
		}
	}
}

void features::spread_circle::draw( ) {
	static auto& spread_circle = options::vars [ _( "visuals.other.spread_circle" ) ].val.b;
	static auto& gradient_spread_circle = options::vars [ _( "visuals.other.gradient_spread_circle" ) ].val.b;
	static auto& spread_circle_color = options::vars [ _( "visuals.other.spread_circle_color" ) ].val.c;
	static auto& fov = options::vars [ _( "visuals.other.fov" ) ].val.f;
	static auto& viewmodel_fov = options::vars [ _( "visuals.other.viewmodel_fov" ) ].val.f;

	float w = 0, h = 0;
	render::screen_size( w, h );

	if ( !g::local || !g::local->alive( ) || !g::local->weapon( ) || !spread_circle || !total_spread )
		return;

	//vec3_t ang;
	//csgo::i::engine->get_viewangles( ang );
	//auto forward = csgo::angle_vec( ang ).normalized( );
	//auto right = csgo::angle_vec( ang + vec3_t( 0.0f, 90.0f, 0.0f ) ).normalized( );
	//auto up = csgo::angle_vec( ang + vec3_t( 90.0f, 0.0f, 0.0f ) ).normalized( );
	//const auto spread_coeff = total_spread * 0.5f;
	//const auto src = g::local->eyes( );
	//const auto left_point = src + ( forward - right * spread_coeff ) * 4096.0f;
	//const auto right_point = src + ( forward + right * spread_coeff ) * 4096.0f;
//
	//trace_t tr;
	//trace_t tr1;
	//ray_t ray;
	//ray_t ray1;
	//trace_filter_t filter;
//
	//filter.m_skip = g::local;
//
	//ray.init( src, left_point );
	//ray1.init( src, right_point );
//
	//csgo::i::trace->trace_ray( ray, mask_shot, &filter, &tr );
	//csgo::i::trace->trace_ray( ray1, mask_shot, &filter, &tr1 );
//
	//vec3_t screen_left, screen_right;
	//if ( csgo::render::world_to_screen( screen_left, tr.m_endpos ) && csgo::render::world_to_screen( screen_right, tr1.m_endpos ) ) {
	//	render::rectangle( screen_left.x, screen_left.y, 2, 2, rgba( 255, 0, 0, 255 ) );
	//	render::rectangle( screen_right.x, screen_right.y, 2, 2, rgba( 255, 0, 0, 255 ) );
	//}

	const auto weapon = g::local->weapon( );
	const auto radius = ( total_spread * 320.0f ) / std::tanf( cs::deg2rad( fov ) * 0.5f );

	if ( gradient_spread_circle ) {
		auto x = w / 2;
		auto y = h / 2;

		struct vtx_t {
			float x, y, z, rhw;
			std::uint32_t color;
		};

		std::vector< vtx_t > circle( 48 + 2 );

		auto pi = D3DX_PI;
		const auto angle = 0.0f;

		circle [ 0 ].x = static_cast< float > ( x ) - 0.5f;
		circle [ 0 ].y = static_cast< float > ( y ) - 0.5f;
		circle [ 0 ].z = 0;
		circle [ 0 ].rhw = 1;
		circle [ 0 ].color = D3DCOLOR_RGBA( static_cast< int > ( spread_circle_color.r * 255.0f ), static_cast< int > ( spread_circle_color.g * 255.0f ), static_cast< int > ( spread_circle_color.b * 255.0f ), 0 );

		for ( auto i = 1; i < 48 + 2; i++ ) {
			circle [ i ].x = ( float )( x - radius * std::cosf( pi * ( ( i - 1 ) / ( 48.0f / 2.0f ) ) ) ) - 0.5f;
			circle [ i ].y = ( float )( y - radius * std::sinf( pi * ( ( i - 1 ) / ( 48.0f / 2.0f ) ) ) ) - 0.5f;
			circle [ i ].z = 0;
			circle [ i ].rhw = 1;
			circle [ i ].color = D3DCOLOR_RGBA( static_cast< int > ( spread_circle_color.r * 255.0f ), static_cast< int > ( spread_circle_color.g * 255.0f ), static_cast< int > ( spread_circle_color.b * 255.0f ), static_cast< int > ( spread_circle_color.a * 255.0f ) );
		}

		const auto _res = 48 + 2;

		for ( auto i = 0; i < _res; i++ ) {
			circle [ i ].x = x + std::cosf( angle ) * ( circle [ i ].x - x ) - std::sinf( angle ) * ( circle [ i ].y - y ) - 0.5f;
			circle [ i ].y = y + std::sinf( angle ) * ( circle [ i ].x - x ) + std::cosf( angle ) * ( circle [ i ].y - y ) - 0.5f;
		}

		IDirect3DVertexBuffer9* vb = nullptr;

		cs::i::dev->CreateVertexBuffer( ( 48 + 2 ) * sizeof( vtx_t ), D3DUSAGE_WRITEONLY, D3DFVF_XYZRHW | D3DFVF_DIFFUSE, D3DPOOL_DEFAULT, &vb, nullptr );

		void* verticies;
		vb->Lock( 0, ( 48 + 2 ) * sizeof( vtx_t ), ( void** )&verticies, 0 );
		std::memcpy( verticies, &circle [ 0 ], ( 48 + 2 ) * sizeof( vtx_t ) );
		vb->Unlock( );

		cs::i::dev->SetStreamSource( 0, vb, 0, sizeof( vtx_t ) );
		cs::i::dev->DrawPrimitive( D3DPT_TRIANGLEFAN, 0, 48 );

		if ( vb )
			vb->Release( );
	}
	else {
		render::circle( w / 2, h / 2, radius, 48, rgba ( static_cast< int > ( spread_circle_color.r * 255.0f ), static_cast< int > ( spread_circle_color.g * 255.0f ), static_cast< int > ( spread_circle_color.b * 255.0f ), static_cast< int > ( spread_circle_color.a * 255.0f ) ) );
		render::circle( w / 2, h / 2, radius, 48, rgba ( static_cast< int > ( spread_circle_color.r * 255.0f ), static_cast< int > ( spread_circle_color.g * 255.0f ), static_cast< int > ( spread_circle_color.b * 255.0f ), static_cast< int > ( spread_circle_color.a * 255.0f ) ), true );
	}
}