#include "esp.hpp"
#include "../menu/menu.hpp"
#include "../globals.hpp"
#include "../animations/animations.hpp"
#include "ragebot.hpp"
#include "../animations/resolver.hpp"
#include "other_visuals.hpp"
#include "autowall.hpp"
#include <locale>

ID3DXFont* features::esp::indicator_font = nullptr;
ID3DXFont* features::esp::watermark_font = nullptr;
ID3DXFont* features::esp::esp_font = nullptr;
ID3DXFont* features::esp::dbg_font = nullptr;
float box_alpha = 0.0f;

oxui::visual_editor::settings_t* visuals;

std::array< std::deque< std::pair< vec3_t, bool > >, 65 > features::esp::ps_points;
std::array< features::esp::esp_data_t, 65 > features::esp::esp_data;

void draw_esp_box( int x, int y, int w, int h, bool dormant, oxui::visual_editor::settings_t* visuals ) {
	render::outline( x - 1, y - 1, w + 2, h + 2, D3DCOLOR_RGBA( 0, 0, 0, std::clamp< int >( box_alpha * 60.0f, 0, 60 ) ) );
	render::outline( x, y, w, h, dormant ? D3DCOLOR_RGBA ( 150, 150, 150, static_cast< int >( static_cast< float >( visuals->esp_box.picker->clr.a ) * box_alpha ) ) : D3DCOLOR_RGBA ( visuals->esp_box.picker->clr.r, visuals->esp_box.picker->clr.g, visuals->esp_box.picker->clr.b, static_cast< int >( static_cast< float >( visuals->esp_box.picker->clr.a )* box_alpha ) ) );
}

auto cur_offset_left_height = 0;
auto cur_offset_right_height = 0;
auto cur_offset_left = 4;
auto cur_offset_right = 4;
auto cur_offset_bottom = 4;
auto cur_offset_top = 4;

void draw_esp_widget ( const oxui::rect& box, const oxui::esp_widget_t& esp_widget, bool dormant, double value, double max, const std::wstring_view& to_print = _ (L"") ) {
	uint32_t clr1 = D3DCOLOR_RGBA ( 0, 0, 0, std::clamp< int > ( static_cast< float >( esp_widget.picker->clr.a ) / 2, 0, 60 ) );
	uint32_t clr = D3DCOLOR_RGBA ( esp_widget.picker->clr.r, esp_widget.picker->clr.g, esp_widget.picker->clr.b, esp_widget.picker->clr.a );

	if ( dormant )
		clr = D3DCOLOR_RGBA ( 150, 150, 150, static_cast< int >( static_cast< float >( esp_widget.picker->clr.a ) * box_alpha ) );

	switch ( esp_widget.type ) {
	case oxui::esp_widget_type_t::info_type_bar: {
		const auto fraction = std::clamp( value / max, 0.0, 1.0 );
		const auto calc_height = fraction * box.h;

		switch ( esp_widget.location ) {
		case oxui::esp_widget_pos_t::pos_left:
			render::outline ( box.x - cur_offset_left - 4, box.y, 4, box.h, clr1 );
			render::rectangle ( box.x - cur_offset_left - 4 + 1, box.y + ( box.h - calc_height ) + 1, 4 - 1, calc_height, clr );

			if ( visuals->show_value )
			render::text ( box.x - cur_offset_left - 4 + 1, box.y + ( box.h - calc_height ) + 1, D3DCOLOR_RGBA( 255, 255, 255, 255), features::esp::dbg_font, std::to_wstring(static_cast<int>( value )), false, true );
			cur_offset_left += 6;
			break;
		case oxui::esp_widget_pos_t::pos_right:
			render::outline ( box.x + box.w + cur_offset_right, box.y, 4, box.h, clr1 );
			render::rectangle ( box.x + box.w + cur_offset_right + 1, box.y + ( box.h - calc_height ) + 1, 4 - 1, calc_height, clr );

			if ( visuals->show_value )
			render::text ( box.x + box.w + cur_offset_right + 1, box.y + ( box.h - calc_height ) + 1, D3DCOLOR_RGBA ( 255, 255, 255, 255 ), features::esp::dbg_font, std::to_wstring ( static_cast< int >( value ) ), false, true );
			cur_offset_right += 6;
			break;
		case oxui::esp_widget_pos_t::pos_bottom:
			render::outline ( box.x, box.y + box.h + cur_offset_bottom, box.w, 4, clr1 );
			render::rectangle ( box.x + 1, box.y + box.h + cur_offset_bottom + 1, static_cast< float >( box.w )* fraction + 1, 4 - 1, clr );

			if ( visuals->show_value )
			render::text ( box.x + 1 + static_cast< float >( box.w )* fraction + 1, box.y + box.h + cur_offset_bottom + 1, D3DCOLOR_RGBA ( 255, 255, 255, 255 ), features::esp::dbg_font, std::to_wstring ( static_cast< int >( value ) ), false, true );
			cur_offset_bottom += 6;
			break;
		case oxui::esp_widget_pos_t::pos_top:
			render::outline ( box.x, box.y - cur_offset_top - 4, box.w, 4, clr1 );
			render::rectangle ( box.x + 1, box.y - cur_offset_top - 4 + 1, static_cast< float >( box.w )* fraction + 1, 4 - 1, clr );

			if ( visuals->show_value )
			render::text ( box.x + 1 + static_cast< float >( box.w )* fraction + 1, box.y - cur_offset_top - 4 + 1, D3DCOLOR_RGBA ( 255, 255, 255, 255 ), features::esp::dbg_font, std::to_wstring ( static_cast< int >( value ) ), false, true );
			cur_offset_top += 6;
			break;
		}
	} break;
	case oxui::esp_widget_type_t::info_type_text: {
		render::dim text_dim;
		render::text_size ( features::esp::esp_font, to_print, text_dim );

		switch ( esp_widget.location ) {
		case oxui::esp_widget_pos_t::pos_left:
			render::text ( box.x - cur_offset_left - text_dim.w, box.y + cur_offset_left_height, clr, features::esp::esp_font, to_print, true );
			cur_offset_left_height += text_dim.h + 4;
			break;
		case oxui::esp_widget_pos_t::pos_right:
			render::text ( box.x + cur_offset_right + box.w, box.y + cur_offset_right_height, clr, features::esp::esp_font, to_print, true );
			cur_offset_right_height += text_dim.h + 4;
			break;
		case oxui::esp_widget_pos_t::pos_bottom:
			render::text ( box.x + box.w / 2 - text_dim.w / 2, box.y + box.h + cur_offset_bottom, clr, features::esp::esp_font, to_print, true );
			cur_offset_bottom += text_dim.h + 4;
			break;
		case oxui::esp_widget_pos_t::pos_top:
			render::text ( box.x + box.w / 2 - text_dim.w / 2, box.y - cur_offset_top - text_dim.h, clr, features::esp::esp_font, to_print, true );
			cur_offset_top += text_dim.h + 4;
			break;
		}
	} break;
	case oxui::esp_widget_type_t::info_type_number: {

	} break;
	case oxui::esp_widget_type_t::info_type_flag: {

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
	PAD ( 8 );
	int m_count;
	PAD ( 4 );
};

snd_data_t cached_data;

void features::esp::handle_dynamic_updates ( ) {
	if ( !g::local )
		return;
	
	static auto get_active_sounds = pattern::search ( _ ( "engine.dll" ), _ ( "55 8B EC 83 E4 F8 81 EC 44 03 00 00 53 56" ) ).get< void( __thiscall* )( snd_data_t* ) >( );

	memset ( &cached_data, 0, sizeof cached_data );
	get_active_sounds ( &cached_data );

	if ( !cached_data.m_count )
		return;
	
	for ( auto i = 0; i < cached_data.m_count; i++ ) {
		const auto sound = cached_data.m_sounds [ i ];
	
		if ( !sound.m_from_server || !sound.m_sound_src || sound.m_sound_src > 64 || !sound.m_origin || *sound.m_origin == vec3_t ( 0.0f, 0.0f, 0.0f ) )
			continue;
	
		auto pl = csgo::i::ent_list->get< player_t* > ( sound.m_sound_src );
	
		if ( !pl || !pl->dormant( ) )
			continue;
	
		vec3_t end_pos = *sound.m_origin;
	
		trace_t tr;
		csgo::util_tracehull ( *sound.m_origin + vec3_t ( 0.0f, 0.0f, 1.0f ), *sound.m_origin - vec3_t ( 0.0f, 0.0f, 4096.0f ), pl->mins( ), pl->maxs( ), 0x201400B, pl, &tr );
	
		if ( tr.did_hit ( ) )
			end_pos = tr.m_endpos;
	
		esp_data [ pl->idx( ) ].m_pos = end_pos;
		esp_data [ pl->idx ( ) ].m_last_seen = csgo::i::globals->m_curtime;
	}
}

void features::esp::render( ) {
	if ( !g::local )
		return;

	static auto spawn_time = 0.0f;

	if ( g::local->spawn_time ( ) != spawn_time ) {
		for ( auto i = 1; i <= csgo::i::globals->m_max_clients; i++ ) {
			esp_data [ i ].m_pl = nullptr;
			esp_data [ i ].m_dormant = true;
			esp_data [ i ].m_first_seen = esp_data [ i ].m_last_seen = 0.0f;
		}

		spawn_time = g::local->spawn_time ( );

		return;
	}

	for ( auto i = 1; i <= csgo::i::globals->m_max_clients; i++ ) {
		auto e = csgo::i::ent_list->get< player_t* >( i );

		if ( !e ) {
			esp_data [ i ].m_pl = nullptr;
			continue;
		}

		if ( !get_visuals ( e, &visuals ) ) {
			esp_data [ i ].m_pl = nullptr;
			continue;
		}

		vec3_t flb, brt, blb, frt, frb, brb, blt, flt;
		float left, top, right, bottom;

		vec3_t min = e->mins( );
		vec3_t max = e->maxs( );

		const auto recs = lagcomp::get_all ( e );

		if ( recs.second ) {
			min += recs.first.front ( ).m_origin;
			max += recs.first.front ( ).m_origin;
		}
		else {
			min += e->abs_origin ( );
			max += e->abs_origin ( );
		}

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

		if ( !csgo::render::world_to_screen( flb, points [ 3 ] )
			|| !csgo::render::world_to_screen( brt, points [ 5 ] )
			|| !csgo::render::world_to_screen( blb, points [ 0 ] )
			|| !csgo::render::world_to_screen( frt, points [ 4 ] )
			|| !csgo::render::world_to_screen( frb, points [ 2 ] )
			|| !csgo::render::world_to_screen( brb, points [ 1 ] )
			|| !csgo::render::world_to_screen( blt, points [ 6 ] )
			|| !csgo::render::world_to_screen( flt, points [ 7 ] ) ) {
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

		esp_data [ e->idx ( ) ].m_pl = e;
		esp_data [ e->idx ( ) ].m_box.left = left;
		esp_data [ e->idx ( ) ].m_box.right = right;
		esp_data [ e->idx ( ) ].m_box.bottom = bottom;
		esp_data [ e->idx ( ) ].m_box.top = top;
		esp_data [ e->idx ( ) ].m_dormant = e->dormant ( );

		if ( !e->dormant ( ) ) {
			if ( recs.second )
				esp_data [ e->idx ( ) ].m_pos = recs.first.front ( ).m_origin;
			else
				esp_data [ e->idx ( ) ].m_pos = e->abs_origin ( );

			if ( esp_data [ e->idx ( ) ].m_first_seen == 0.0f )
				esp_data [ e->idx ( ) ].m_first_seen = csgo::i::globals->m_curtime;

			esp_data [ e->idx ( ) ].m_last_seen = csgo::i::globals->m_curtime;

			if ( e && e->weapon ( ) && e->weapon ( )->data ( ) ) {
				std::string hud_name = e->weapon ( )->data ( )->m_weapon_name;

				hud_name.erase ( 0, 7 );

				for ( auto& character : hud_name ) {
					if ( character == '_' ) {
						character = ' ';
						continue;
					}

					if ( character >= 65 && character <= 90 )
						character = std::tolower ( character );
				}

				esp_data [ e->idx ( ) ].m_weapon_name = std::wstring ( hud_name.begin ( ), hud_name.end ( ) );

				if ( e->weapon ( ) && e->weapon ( )->item_definition_index ( ) == 64 )
					esp_data [ e->idx ( ) ].m_weapon_name = _ ( L"revolver" );
			}
		}
		else {
			esp_data [ e->idx ( ) ].m_first_seen = 0.0f;
		}

		auto dormant_time = std::max< float > ( 7.0f/*esp_fade_time*/, 0.1f );

		if ( esp_data [ e->idx ( ) ].m_pl && std::fabsf( csgo::i::globals->m_curtime - esp_data [ e->idx ( ) ].m_last_seen ) < dormant_time ) {
			auto calc_alpha = [ & ] ( float time, float fade_time, bool add = false ) {
				return ( std::clamp< float > ( dormant_time - ( std::clamp< float > ( add ? ( dormant_time - std::clamp< float >( std::fabsf ( csgo::i::globals->m_curtime - time ), 0.0f, dormant_time ) ) : std::fabsf ( csgo::i::globals->m_curtime - time ), std::max< float > ( dormant_time - fade_time, 0.0f ), dormant_time ) ), 0.0f, fade_time ) / fade_time );
			};

			if ( !esp_data [ e->idx ( ) ].m_dormant )
				box_alpha = calc_alpha ( esp_data [ e->idx ( ) ].m_first_seen, 0.6f, true );
			else
				box_alpha = calc_alpha( esp_data [ e->idx ( ) ].m_last_seen, 2.0f );

			player_info_t info;
			csgo::i::engine->get_player_info ( e->idx ( ), &info );

			wchar_t buf [ 36 ] { '\0' };
			std::wstring wname = _ ( L"" );
			if ( MultiByteToWideChar ( CP_UTF8, 0, info.m_name, -1, buf, 36 ) > 0 )
				wname = buf;

			if ( visuals->esp_box.enabled )
				draw_esp_box ( left, top, right - left, bottom - top, esp_data [ e->idx ( ) ].m_dormant, visuals );

			cur_offset_left_height = 0;
			cur_offset_right_height = 0;
			cur_offset_left = 4;
			cur_offset_right = 4;
			cur_offset_bottom = 4;
			cur_offset_top = 4;

			oxui::rect esp_rect { static_cast<int>( left ), static_cast< int >(top), static_cast< int >(right - left), static_cast< int >(bottom - top) };

			if ( visuals->health_bar.enabled )
				draw_esp_widget ( esp_rect, visuals->health_bar, esp_data [ e->idx ( ) ].m_dormant, e->health ( ), 100.0 );

			if ( visuals->ammo_bar.enabled && e->weapon ( ) && e->weapon ( )->data ( ) )
				draw_esp_widget ( esp_rect, visuals->ammo_bar, esp_data [ e->idx ( ) ].m_dormant, e->weapon ( )->ammo( ), e->weapon ( )->data ( )->m_max_clip );

			if ( visuals->desync_bar.enabled )
				draw_esp_widget ( esp_rect, visuals->desync_bar, esp_data [ e->idx ( ) ].m_dormant, e->desync_amount( ), 58.0 );

			if ( visuals->esp_name.enabled )
				draw_esp_widget ( esp_rect, visuals->esp_name, esp_data [ e->idx ( ) ].m_dormant, 0.0, 0.0, wname );

			if ( visuals->esp_weapon.enabled )
				draw_esp_widget ( esp_rect, visuals->esp_weapon, esp_data [ e->idx ( ) ].m_dormant, 0.0, 0.0, esp_data [ e->idx ( ) ].m_weapon_name );
		}
	}
}