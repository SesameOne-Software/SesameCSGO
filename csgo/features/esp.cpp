#include "esp.hpp"
#include "../menu/menu.hpp"
#include "../globals.hpp"
#include "ragebot.hpp"
#include "../animations/resolver.hpp"
#include "other_visuals.hpp"
#include "autowall.hpp"
#include <locale>

#include "../renderer/render.hpp"

#include "../imgui/imgui.h"
#include "../imgui/imgui_impl_dx9.h"
#include "../imgui/imgui_impl_win32.h"
#include "../imgui/imgui_internal.h"

float box_alpha = 0.0f;

std::array< std::deque< std::pair< vec3_t, bool > >, 65 > features::esp::ps_points;
std::array< features::esp::esp_data_t, 65 > features::esp::esp_data;

void draw_esp_box( int x, int y, int w, int h, bool dormant, const options::option::colorf& esp_box_color ) {
	render::outline( x - 1, y - 1, w + 2, h + 2, rgba ( 0, 0, 0, std::clamp< int >( esp_box_color.a * 60.0f, 0, 60 ) ) );
	render::outline( x, y, w, h, dormant ? rgba ( 150, 150, 150, static_cast< int > ( esp_box_color.a * 255.0f * box_alpha ) ) : rgba ( static_cast< int > ( esp_box_color.r * 255.0f ), static_cast< int > ( esp_box_color.g * 255.0f ), static_cast< int > ( esp_box_color.b * 255.0f ), static_cast< int >( esp_box_color.a * 255.0f * box_alpha ) ) );
}

auto cur_offset_left_height = 0;
auto cur_offset_right_height = 0;
auto cur_offset_left = 4;
auto cur_offset_right = 4;
auto cur_offset_bottom = 4;
auto cur_offset_top = 4;

enum esp_type_t {
	esp_type_bar = 0,
	esp_type_text,
	esp_type_number,
	esp_type_flag
};

void draw_esp_widget( const ImRect& box, const options::option::colorf& widget_color, esp_type_t type, bool show_value, const int orientation, bool dormant, double value, double max, std::string to_print = _( "" ) ) {
	uint32_t clr1 = rgba ( 0, 0, 0, std::clamp< int >( static_cast< float >( widget_color.a * 255.0f ) / 2.0f, 0, 125 ) );
	uint32_t clr = rgba ( static_cast< int > ( widget_color.r * 255.0f ), static_cast< int > ( widget_color.g * 255.0f ), static_cast< int > ( widget_color.b * 255.0f ), static_cast< int > ( widget_color.a * 255.0f ) );

	if ( dormant )
		clr = rgba ( 150, 150, 150, static_cast< int >( widget_color.a * 255.0f * box_alpha ) );

	switch ( type ) {
	case esp_type_bar: {
		const auto sval = std::to_string ( static_cast< int >( value ) );

		vec3_t text_size;
		render::text_size ( sval, _ ( "dbg_font" ), text_size );

		const auto fraction = std::clamp ( value / max, 0.0, 1.0 );
		const auto calc_height = fraction * box.Max.y;

		switch ( orientation ) {
		case features::esp_placement_left:
			render::rect ( box.Min.x - cur_offset_left - 5 + 1, box.Min.y + ( box.Max.y - calc_height ) + 1, 5 - 1, calc_height, clr );
			render::outline ( box.Min.x - cur_offset_left - 5, box.Min.y, 5, box.Max.y, clr1 );

			if ( show_value )
				render::text ( box.Min.x - cur_offset_left - 5 + 1 + 5 / 2 - text_size.x / 2, box.Min.y + ( box.Max.y - calc_height ) + 1 - text_size.y / 2, sval, _ ( "dbg_font" ), rgba ( 255, 255, 255, 255 ), true );
			cur_offset_left += 7;
			break;
		case features::esp_placement_right:
			render::rect ( box.Min.x + box.Max.x + cur_offset_right + 1, box.Min.y + ( box.Max.y - calc_height ) + 1, 5 - 1, calc_height, clr );
			render::outline ( box.Min.x + box.Max.x + cur_offset_right, box.Min.y, 5, box.Max.y, clr1 );

			if ( show_value )
				render::text ( box.Min.x + box.Max.x + cur_offset_right + 1 + 5 / 2 - text_size.x / 2, box.Min.y + ( box.Max.y - calc_height ) + 1 - text_size.y / 2, sval, _ ( "dbg_font" ), rgba ( 255, 255, 255, 255 ), true );
			cur_offset_right += 7;
			break;
		case features::esp_placement_bottom:
			render::rect ( box.Min.x + 1, box.Min.y + box.Max.y + cur_offset_bottom + 1, static_cast< float >( box.Max.x )* fraction + 1, 5 - 1, clr );
			render::outline ( box.Min.x, box.Min.y + box.Max.y + cur_offset_bottom, box.Max.x, 5, clr1 );

			if ( show_value )
				render::text ( box.Min.x + 1 + static_cast< float >( box.Max.x )* fraction + 1 - text_size.x / 2, box.Min.y + box.Max.y + cur_offset_bottom + 1 + 5 / 2 - text_size.y / 2, sval, _ ( "dbg_font" ), rgba ( 255, 255, 255, 255 ), true );
			cur_offset_bottom += 7;
			break;
		case features::esp_placement_top:
			render::rect ( box.Min.x + 1, box.Min.y - cur_offset_top - 5 + 1, static_cast< float >( box.Max.x )* fraction + 1, 5 - 1, clr );
			render::outline ( box.Min.x, box.Min.y - cur_offset_top - 5, box.Max.x, 5, clr1 );

			if ( show_value )
				render::text ( box.Min.x + 1 + static_cast< float >( box.Max.x )* fraction + 1 - text_size.x / 2, box.Min.y - cur_offset_top - 5 + 1 + 5 / 2 - text_size.y / 2, sval, _ ( "dbg_font" ), rgba ( 255, 255, 255, 255 ), true );
			cur_offset_top += 7;
			break;
		}
	} break;
		case esp_type_text: {
			std::string as_str = to_print;

			vec3_t text_size;
			render::text_size ( as_str, _ ( "esp_font" ), text_size );

			switch ( orientation ) {
			case features::esp_placement_left:
				render::text ( box.Min.x - cur_offset_left - text_size.x, box.Min.y + cur_offset_left_height, as_str, _ ( "esp_font" ), clr, true );
				cur_offset_left_height += text_size.y + 2;
				break;
			case features::esp_placement_right:
				render::text ( box.Min.x + cur_offset_right + box.Max.x, box.Min.y + cur_offset_right_height, as_str, _ ( "esp_font" ), clr, true );
				cur_offset_right_height += text_size.y + 2;
				break;
			case features::esp_placement_bottom:
				render::text ( box.Min.x + box.Max.x / 2 - text_size.x / 2, box.Min.y + box.Max.y + cur_offset_bottom, as_str, _ ( "esp_font" ), clr, true );
				cur_offset_bottom += text_size.y + 2;
				break;
			case features::esp_placement_top:
				render::text ( box.Min.x + box.Max.x / 2 - text_size.x / 2, box.Min.y - cur_offset_top - text_size.y, as_str, _ ( "esp_font" ), clr, true );
				cur_offset_top += text_size.y + 2;
				break;
			}
		} break;
		case esp_type_number: {

		} break;
		case esp_type_flag: {

		} break;
		default: break;
	}
}

struct snd_info_t {
	int m_guid;
	void* m_hfile;
	int m_sound_src;
	int m_channel;
	int m_speaker_ent;
	float m_vol;
	float m_last_spatialized_vol;
	float m_rad;
	int pitch;
	vec3_t* m_origin;
	vec3_t* m_dir;
	bool m_update_positions;
	bool m_is_sentence;
	bool m_dry_mix;
	bool m_speaker;
	bool m_from_server;
};

struct snd_data_t {
	snd_info_t* m_sounds;
	PAD( 8 );
	int m_count;
	PAD( 4 );
};

snd_data_t cached_data;

void features::esp::handle_dynamic_updates( ) {
	if ( !g::local )
		return;

	static auto get_active_sounds = pattern::search( _( "engine.dll" ), _( "55 8B EC 83 E4 F8 81 EC 44 03 00 00 53 56" ) ).get< void( __thiscall* )( snd_data_t* ) >( );

	memset( &cached_data, 0, sizeof cached_data );
	get_active_sounds( &cached_data );

	if ( !cached_data.m_count )
		return;

	for ( auto i = 0; i < cached_data.m_count; i++ ) {
		const auto sound = cached_data.m_sounds [ i ];

		if ( !sound.m_from_server || !sound.m_sound_src || sound.m_sound_src > 64 || !sound.m_origin || *sound.m_origin == vec3_t( 0.0f, 0.0f, 0.0f ) )
			continue;

		auto pl = cs::i::ent_list->get< player_t* >( sound.m_sound_src );

		if ( !pl || !pl->dormant( ) )
			continue;

		vec3_t end_pos = *sound.m_origin;

		trace_t tr;
		cs::util_tracehull( *sound.m_origin + vec3_t( 0.0f, 0.0f, 1.0f ), *sound.m_origin - vec3_t( 0.0f, 0.0f, 4096.0f ), pl->mins( ), pl->maxs( ), 0x201400B, pl, &tr );

		if ( tr.did_hit( ) )
			end_pos = tr.m_endpos;

		esp_data [ pl->idx( ) ].m_pos = end_pos;
		esp_data [ pl->idx( ) ].m_dormant = true;
		esp_data [ pl->idx( ) ].m_last_seen = prediction::curtime( );

		//dbg_print( _( "sound\n" ) );
	}
}

extern std::array< animlayer_t, 13 > latest_animlayers;

void features::esp::render( ) {
	if ( !g::local )
		return;

	static auto spawn_time = 0.0f;

	if ( g::local->spawn_time( ) != spawn_time ) {
		for ( auto i = 1; i <= cs::i::globals->m_max_clients; i++ ) {
			esp_data [ i ].m_pl = nullptr;
			esp_data [ i ].m_dormant = true;
			esp_data [ i ].m_first_seen = esp_data [ i ].m_last_seen = 0.0f;
		}

		spawn_time = g::local->spawn_time( );

		return;
	}

	for ( auto i = 1; i <= cs::i::globals->m_max_clients; i++ ) {
		auto e = cs::i::ent_list->get< player_t* >( i );

		if ( !e || !e->alive() ) {
			esp_data [ i ].m_pl = nullptr;
			continue;
		}

		features::visual_config_t visuals;

		if ( !get_visuals( e, visuals ) ) {
			esp_data [ i ].m_pl = nullptr;
			continue;
		}

		vec3_t flb, brt, blb, frt, frb, brb, blt, flt;
		float left, top, right, bottom;

		vec3_t min = e->mins( );
		vec3_t max = e->maxs( );

		min += e->abs_origin( );
		max += e->abs_origin( );

		vec3_t points [ ] = {
			vec3_t( min.x, min.y, min.z ),
			vec3_t( min.x, max.y, min.z ),
			vec3_t( max.x, max.y, min.z ),
			vec3_t( max.x, min.y, min.z ),
			vec3_t( max.x, max.y, max.z ),
			vec3_t( min.x, max.y, max.z ),
			vec3_t( min.x, min.y, max.z ),
			vec3_t( max.x, min.y, max.z )
		};

		if ( !cs::render::world_to_screen( flb, points [ 3 ] )
			|| !cs::render::world_to_screen( brt, points [ 5 ] )
			|| !cs::render::world_to_screen( blb, points [ 0 ] )
			|| !cs::render::world_to_screen( frt, points [ 4 ] )
			|| !cs::render::world_to_screen( frb, points [ 2 ] )
			|| !cs::render::world_to_screen( brb, points [ 1 ] )
			|| !cs::render::world_to_screen( blt, points [ 6 ] )
			|| !cs::render::world_to_screen( flt, points [ 7 ] ) ) {
			continue;
		}

		vec3_t arr [ ] = { flb, brt, blb, frt, frb, brb, blt, flt };

		left = flb.x;
		top = flb.y;
		right = flb.x;
		bottom = flb.y;

		for ( auto i = 1; i < 8; i++ ) {
			if ( left > arr [ i ].x )
				left = arr [ i ].x;

			if ( bottom < arr [ i ].y )
				bottom = arr [ i ].y;

			if ( right < arr [ i ].x )
				right = arr [ i ].x;

			if ( top > arr [ i ].y )
				top = arr [ i ].y;
		}

		const auto subtract_w = ( right - left ) / 5;
		const auto subtract_h = ( right - left ) / 16;

		right -= subtract_w / 2;
		left += subtract_w / 2;
		bottom -= subtract_h / 2;
		top += subtract_h / 2;

		esp_data [ e->idx( ) ].m_pl = e;
		esp_data [ e->idx( ) ].m_box.left = left;
		esp_data [ e->idx( ) ].m_box.right = right;
		esp_data [ e->idx( ) ].m_box.bottom = bottom;
		esp_data [ e->idx( ) ].m_box.top = top;
		esp_data [ e->idx( ) ].m_dormant = e->dormant( );

		if ( !e->dormant( ) ) {
			esp_data [ e->idx( ) ].m_pos = e->abs_origin( );

			if ( esp_data [ e->idx( ) ].m_first_seen == 0.0f )
				esp_data [ e->idx( ) ].m_first_seen = prediction::curtime( );

			esp_data [ e->idx( ) ].m_last_seen = prediction::curtime( );

			if ( e && e->weapon( ) && e->weapon( )->data( ) ) {
				std::string hud_name = e->weapon( )->data( )->m_weapon_name;

				hud_name.erase( 0, 7 );

				for ( auto& character : hud_name ) {
					if ( character == '_' ) {
						character = ' ';
						continue;
					}

					if ( character >= 65 && character <= 90 )
						character = std::tolower( character );
				}

				esp_data [ e->idx( ) ].m_weapon_name = hud_name;

				if ( e->weapon( ) && e->weapon( )->item_definition_index( ) == weapons_t::revolver )
					esp_data [ e->idx( ) ].m_weapon_name = _( "revolver" );
			}
		}
		else {
			esp_data [ e->idx( ) ].m_first_seen = 0.0f;
		}

		auto dormant_time = std::max< float >( 9.0f/*esp_fade_time*/, 0.1f );

		if ( esp_data [ e->idx( ) ].m_pl && std::fabsf( prediction::curtime( ) - esp_data [ e->idx( ) ].m_last_seen ) < dormant_time ) {
			auto calc_alpha = [ & ] ( float time, float fade_time, bool add = false ) {
				return ( std::clamp< float >( dormant_time - ( std::clamp< float >( add ? ( dormant_time - std::clamp< float >( std::fabsf( prediction::curtime( ) - time ), 0.0f, dormant_time ) ) : std::fabsf( prediction::curtime( ) - time ), std::max< float >( dormant_time - fade_time, 0.0f ), dormant_time ) ), 0.0f, fade_time ) / fade_time );
			};

			if ( !esp_data [ e->idx( ) ].m_dormant )
				box_alpha = calc_alpha( esp_data [ e->idx( ) ].m_first_seen, 0.6f, true );
			else
				box_alpha = calc_alpha( esp_data [ e->idx( ) ].m_last_seen, 2.0f );

			std::string name = _ ( "");

			player_info_t info;

			if ( cs::i::engine->get_player_info ( e->idx ( ), &info ) )
				name = info.m_name;

			if ( visuals.esp_box )
				draw_esp_box( left, top, right - left, bottom - top, esp_data [ e->idx( ) ].m_dormant, visuals.box_color );

			cur_offset_left_height = 0;
			cur_offset_right_height = 0;
			cur_offset_left = 4;
			cur_offset_right = 4;
			cur_offset_bottom = 4;
			cur_offset_top = 4;

			ImVec4 esp_rect { left, top, right - left, bottom - top };

			//if ( e == g::local ) {
			//	int y = 20;
			//	if ( std::isfinite<float> ( latest_animlayers [ 6 ].m_cycle ) )
			//		features::esp::esp_font.draw_text ( 20, y, _ ( "local.layer[6].cycle: " ) + std::to_string ( latest_animlayers [ 6 ].m_cycle ), 0xffffffff, truetype::text_flags_t::text_flags_outline );
			//	y += 30;
			//	if ( std::isfinite<float> ( latest_animlayers [ 6 ].m_weight ) )
			//		features::esp::esp_font.draw_text ( 20, y, _ ( "local.layer[6].weight: " ) + std::to_string ( latest_animlayers [ 6 ].m_weight ), 0xffffffff, truetype::text_flags_t::text_flags_outline );
			//	y += 30;
			//	if ( std::isfinite<float> ( latest_animlayers [ 6 ].m_sequence ) )
			//		features::esp::esp_font.draw_text ( 20, y, _ ( "local.layer[6].sequence: " ) + std::to_string ( latest_animlayers [ 6 ].m_sequence ), 0xffffffff, truetype::text_flags_t::text_flags_outline );
			//	y += 30;
			//	if ( std::isfinite<float> ( latest_animlayers [ 6 ].m_weight_delta_rate ) )
			//		features::esp::esp_font.draw_text ( 20, y, _ ( "local.layer[6].weight_delta_rate: " ) + std::to_string ( latest_animlayers [ 6 ].m_weight_delta_rate ), 0xffffffff, truetype::text_flags_t::text_flags_outline );
			//	y += 30;
			//	if ( std::isfinite<float> ( latest_animlayers [ 6 ].m_playback_rate ) )
			//		features::esp::esp_font.draw_text ( 20, y, _ ( "local.layer[6].playback_rate: " ) + std::to_string ( latest_animlayers [ 6 ].m_playback_rate ), 0xffffffff, truetype::text_flags_t::text_flags_outline );
			//}

			//if ( e == g::local ) {
			//	int y = 20;
			//
			//	for ( auto i = 0; i < 13; i++ ) {
			//		if ( std::isfinite<float> ( latest_animlayers [ i ].m_cycle ) )
			//			features::esp::esp_font.draw_text ( 20, y, fmt::format(_("layer[{}].cycle : {:.1f}"), i, latest_animlayers [ i ].m_cycle ), 0xffffffff, truetype::text_flags_t::text_flags_outline );
			//		y += 30;
			//		if ( std::isfinite<float> ( latest_animlayers [ i ].m_weight ) )
			//			features::esp::esp_font.draw_text ( 20, y, fmt::format ( _ ( "layer[{}].weight : {:.1f}" ), i, latest_animlayers [ i ].m_weight ), 0xffffffff, truetype::text_flags_t::text_flags_outline );
			//		y += 30;
			//		if ( std::isfinite<float> ( latest_animlayers [ i ].m_sequence ) )
			//			features::esp::esp_font.draw_text ( 20, y, fmt::format ( _ ( "layer[{}].sequence : {}" ), i, latest_animlayers [ i ].m_sequence ), 0xffffffff, truetype::text_flags_t::text_flags_outline );
			//		y += 30;
			//		if ( std::isfinite<float> ( latest_animlayers [ i ].m_weight_delta_rate ) )
			//			features::esp::esp_font.draw_text ( 20, y, fmt::format ( _ ( "layer[{}].weight_delta_rate : {:.1f}" ), i, latest_animlayers [ i ].m_weight_delta_rate ), 0xffffffff, truetype::text_flags_t::text_flags_outline );
			//		y += 30;
			//		if ( std::isfinite<float> ( latest_animlayers [ i ].m_playback_rate ) )
			//			features::esp::esp_font.draw_text ( 20, y, fmt::format ( _ ( "layer[{}].playback_rate : {:.1f}" ), i, latest_animlayers [ i ].m_playback_rate ), 0xffffffff, truetype::text_flags_t::text_flags_outline );
			//	}
			//}

			//if ( std::isfinite<float> ( latest_animlayers [ 4 ].m_playback_rate ) )
			//	features::esp::esp_font.draw_text ( 20, 30, fmt::format ( _ ( "layer[{}].playback_rate : {:.1f}" ), 4, latest_animlayers [ 4 ].m_playback_rate ), 0xffffffff, truetype::text_flags_t::text_flags_outline );

			if ( visuals.health_bar )
				draw_esp_widget( esp_rect, visuals.health_bar_color, esp_type_bar, visuals.value_text, visuals.health_bar_placement, esp_data [ e->idx( ) ].m_dormant, e->health( ), 100.0 );

			if ( visuals.ammo_bar && e->weapon( ) && e->weapon( )->data( ) && e->weapon( )->ammo( ) != -1 )
				draw_esp_widget( esp_rect, visuals.ammo_bar_color, esp_type_bar, visuals.value_text, visuals.ammo_bar_placement, esp_data [ e->idx( ) ].m_dormant, e->weapon( )->ammo( ), e->weapon( )->data( )->m_max_clip );

			if ( visuals.desync_bar )
				draw_esp_widget( esp_rect, visuals.desync_bar_color, esp_type_bar, visuals.value_text, visuals.desync_bar_placement, esp_data [ e->idx( ) ].m_dormant, e->desync_amount( ), 58.0 );

			if ( visuals.nametag )
				draw_esp_widget( esp_rect, visuals.name_color, esp_type_text, visuals.value_text, visuals.nametag_placement, esp_data [ e->idx( ) ].m_dormant, 0.0, 0.0, name );

			if ( visuals.weapon_name )
				draw_esp_widget( esp_rect, visuals.weapon_color, esp_type_text, visuals.value_text, visuals.weapon_name_placement, esp_data [ e->idx( ) ].m_dormant, 0.0, 0.0, esp_data [ e->idx( ) ].m_weapon_name );

			/* DEBUGGING STUFF */
			//if ( std::isfinite<float> ( anims::feet_playback_rate [ e->idx ( ) ] ) )
			//	draw_esp_widget ( esp_rect, visuals.weapon_color, esp_type_text, visuals.value_text, esp_placement_right, esp_data [ e->idx ( ) ].m_dormant, 0.0, 0.0, _ ( "rate : " ) + std::to_string ( anims::feet_playback_rate [ e->idx ( ) ] ) );
			//
			////if ( std::isfinite<float> ( anims::desync_sign [ e->idx ( ) ] ) )
			////	draw_esp_widget ( esp_rect, visuals.weapon_color, esp_type_text, visuals.value_text, esp_placement_right, esp_data [ e->idx ( ) ].m_dormant, 0.0, 0.0, _ ( "side : " ) + std::to_string ( anims::desync_sign [ e->idx ( ) ] ) );
			//
			//const auto delta = anims::angle_diff ( csgo::normalize( e->angles ( ).y ), csgo::normalize( csgo::vec_angle ( e->vel ( ) ).y ) );
			//
			//if ( std::isfinite<float> ( delta ) )
			//	draw_esp_widget ( esp_rect, visuals.weapon_color, esp_type_text, visuals.value_text, esp_placement_right, esp_data [ e->idx ( ) ].m_dormant, 0.0, 0.0, _ ( "angle_diff : " ) + std::to_string ( delta ) );
			//
			////draw_esp_widget ( esp_rect, visuals.weapon_color, esp_type_text, visuals.value_text, esp_placement_right, esp_data [ e->idx ( ) ].m_dormant, 0.0, 0.0, _ ( "choke : " ) + std::to_string ( anims::choked_commands [ e->idx ( ) ] ) );
		}
	}
}