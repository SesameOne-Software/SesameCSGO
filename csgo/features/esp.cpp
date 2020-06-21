#include "esp.hpp"
#include "../menu/menu.hpp"
#include "../globals.hpp"
#include "../animations/animations.hpp"
#include "ragebot.hpp"
#include "../animations/resolver.hpp"
#include "autowall.hpp"
#include <locale>

ID3DXFont* features::esp::indicator_font = nullptr;
ID3DXFont* features::esp::watermark_font = nullptr;
ID3DXFont* features::esp::esp_font = nullptr;
ID3DXFont* features::esp::dbg_font = nullptr;
float box_alpha = 0.0f;

std::array< std::deque< std::pair< vec3_t, bool > >, 65 > features::esp::ps_points;
std::array< features::esp::esp_data_t, 65 > features::esp::esp_data;

void features::esp::draw_esp_box( int x, int y, int w, int h, bool dormant ) {
	OPTION ( oxui::color, esp_clr, "Sesame->C->ESP->Colors->Box", oxui::object_colorpicker );

	render::outline( x - 1, y - 1, w + 2, h + 2, D3DCOLOR_RGBA( 0, 0, 0, std::clamp< int >( box_alpha * 60.0f, 0, 60 ) ) );
	render::outline( x, y, w, h, dormant ? D3DCOLOR_RGBA ( 150, 150, 150, static_cast< int >( static_cast< float >( esp_clr.a ) * box_alpha ) ) : D3DCOLOR_RGBA ( esp_clr.r, esp_clr.g, esp_clr.b, static_cast< int >( static_cast< float >( esp_clr.a )* box_alpha ) ) );
}

void features::esp::draw_nametag( int x, int y, int w, int h, const std::wstring_view& name, bool dormant ) {
	OPTION ( oxui::color, esp_clr, "Sesame->C->ESP->Colors->Name", oxui::object_colorpicker );

	const auto name_tag_dim = 16;

	//render::dim dim;
	//render::text_size( esp_font, name, dim );
	//const auto box_w = w < ( dim.w + name_tag_dim ) ? ( dim.w + name_tag_dim ) : w;
	//
	//render::rectangle( w < ( dim.w + name_tag_dim ) ? ( x - ( dim.w - w + name_tag_dim ) / 2 ) : x, y - name_tag_dim - 4, box_w, name_tag_dim, D3DCOLOR_RGBA( 0, 0, 0, 28 ) );
	//render::rectangle( w < ( dim.w + name_tag_dim ) ? ( x - ( dim.w - w + name_tag_dim ) / 2 ) : x + 1, y - name_tag_dim - 4 + 1, box_w - 1, 2, D3DCOLOR_RGBA( esp_clr.r, esp_clr.g, esp_clr.b, esp_clr.a ) );
	//render::text( x + w / 2 - dim.w / 2, y - name_tag_dim - 4 + name_tag_dim / 2 - dim.h / 2, D3DCOLOR_RGBA( 255, 255, 255, 255 ), esp_font, name.data( ) );

	render::dim dim;
	render::text_size ( esp_font, name, dim );
	render::text ( x + w / 2 - dim.w / 2, y - 2 - dim.h, dormant ? D3DCOLOR_RGBA ( 150, 150, 150, static_cast< int >( static_cast< float >( esp_clr.a )* box_alpha ) ) : D3DCOLOR_RGBA ( esp_clr.r, esp_clr.g, esp_clr.b, static_cast< int >( static_cast< float >( esp_clr.a )* box_alpha ) ), esp_font, name.data ( ), true );
}

uint32_t features::esp::color_variable_weight( float val, float cieling ) {
	const auto clamped = std::clamp( val, 0.0f, cieling );
	const auto scaled = static_cast< int >( clamped * ( 255.0f / cieling ) );

	return D3DCOLOR_RGBA( scaled, 255 - scaled, 0, 255 );
}

void features::esp::draw_bars( int x, int y, int w, int h, int health_amount, float desync_amount, player_t* pl, bool dormant ) {
	//OPTION( bool, lc, "Sesame->C->ESP->Main->Box", oxui::object_checkbox );
	OPTION( bool, health, "Sesame->C->ESP->Main->Health Bar", oxui::object_checkbox );
	OPTION( bool, desync, "Sesame->C->ESP->Main->Desync Bar", oxui::object_checkbox );
	OPTION( bool, weapon, "Sesame->C->ESP->Main->Weapon", oxui::object_checkbox );
	OPTION ( bool, resolver_confidence, "Sesame->C->ESP->Main->Resolver Confidence", oxui::object_checkbox );
	OPTION ( bool, reloading_flag, "Sesame->C->ESP->Main->Reloading Flag", oxui::object_checkbox );
	OPTION ( bool, fakeduck_flag, "Sesame->C->ESP->Main->Fakeduck Flag", oxui::object_checkbox );
	OPTION ( bool, fatal_baim_flag, "Sesame->C->ESP->Main->Fatal Baim Flag", oxui::object_checkbox );

	OPTION ( oxui::color, name_clr, "Sesame->C->ESP->Colors->Name", oxui::object_colorpicker );
	OPTION ( oxui::color, indicator_on_clr, "Sesame->C->ESP->Colors->Flag On", oxui::object_colorpicker );
	OPTION ( oxui::color, indicator_off_clr, "Sesame->C->ESP->Colors->Flag Off", oxui::object_colorpicker );
	OPTION ( oxui::color, health_clr, "Sesame->C->ESP->Colors->Health Bar", oxui::object_colorpicker );
	OPTION ( oxui::color, desync_clr, "Sesame->C->ESP->Colors->Desync Bar", oxui::object_colorpicker );

	const int desync_clamped = std::clamp( desync_amount, 0.0f, 58.0f );
	const int health_clamped = std::clamp( health_amount, 0, 100 );

	auto cur_y = y + h + 4;
	auto cur_y_side = y;

	/* lag-comp break indicator */
	//if ( lc
	//	&& animations::data::old_origin [ pl->idx( ) ] != vec3_t( FLT_MAX, FLT_MAX, FLT_MAX )
	//	&& animations::data::origin [ pl->idx( ) ] != vec3_t( FLT_MAX, FLT_MAX, FLT_MAX ) ) {
	//	render::text( x + w + 6, cur_y_side, dormant ? D3DCOLOR_RGBA ( 150, 150, 150, std::clamp< int > ( box_alpha * 60.0f, 0, 60 ) ) : ( lagcomp::breaking_lc ( pl ) ? D3DCOLOR_RGBA ( indicator_on_clr.r, indicator_on_clr.g, indicator_on_clr.b, static_cast< int >( static_cast< float >( indicator_on_clr.a )* box_alpha ) ) : D3DCOLOR_RGBA ( indicator_off_clr.r, indicator_off_clr.g, indicator_off_clr.b, static_cast< int >( static_cast< float >( indicator_off_clr.a )* box_alpha ) ) ), indicator_font, _( L"LC" ), true, false );
	//	cur_y_side += 16;
	//}

	if ( resolver_confidence ) {
		render::text ( x + w + 6, cur_y_side, dormant ? D3DCOLOR_RGBA ( 150, 150, 150, std::clamp< int > ( box_alpha * 60.0f, 0, 60 ) ) : ( ( animations::resolver::get_confidence ( pl->idx ( ) ) > 55.0f ) ? D3DCOLOR_RGBA ( indicator_on_clr.r, indicator_on_clr.g, indicator_on_clr.b, static_cast< int >( static_cast< float >( indicator_on_clr.a )* box_alpha ) ) : D3DCOLOR_RGBA ( indicator_off_clr.r, indicator_off_clr.g, indicator_off_clr.b, static_cast< int >( static_cast< float >( indicator_off_clr.a )* box_alpha ) ) ), indicator_font, std::to_wstring ( static_cast < int > ( animations::resolver::get_confidence ( pl->idx ( ) ) ) ) + _ ( L"%" ), true, false );
		cur_y_side += 16;
	}

	//C6 87 ? ? ? ? ? 8B 06 8B CE FF 90

	if ( reloading_flag && esp_data [ pl->idx ( ) ].m_reloading ) {
		render::text ( x + w + 6, cur_y_side, dormant ? D3DCOLOR_RGBA ( 150, 150, 150, std::clamp< int > ( box_alpha * 60.0f, 0, 60 ) ) : ( true ? D3DCOLOR_RGBA ( indicator_on_clr.r, indicator_on_clr.g, indicator_on_clr.b, static_cast< int >( static_cast< float >( indicator_on_clr.a )* box_alpha ) ) : D3DCOLOR_RGBA ( indicator_off_clr.r, indicator_off_clr.g, indicator_off_clr.b, static_cast< int >( static_cast< float >( indicator_off_clr.a )* box_alpha ) ) ), indicator_font, _ ( L"RELOADING" ), true, false );
		cur_y_side += 16;
	}

	if ( fakeduck_flag && esp_data [ pl->idx ( ) ].m_fakeducking ) {
		render::text ( x + w + 6, cur_y_side, dormant ? D3DCOLOR_RGBA ( 150, 150, 150, std::clamp< int > ( box_alpha * 60.0f, 0, 60 ) ) : ( true ? D3DCOLOR_RGBA ( indicator_on_clr.r, indicator_on_clr.g, indicator_on_clr.b, static_cast< int >( static_cast< float >( indicator_on_clr.a )* box_alpha ) ) : D3DCOLOR_RGBA ( indicator_off_clr.r, indicator_off_clr.g, indicator_off_clr.b, static_cast< int >( static_cast< float >( indicator_off_clr.a )* box_alpha ) ) ), indicator_font, _ ( L"FAKEDUCK" ), true, false );
		cur_y_side += 16;
	}

	if ( fatal_baim_flag && esp_data [ pl->idx ( ) ].m_fatal ) {
		render::text ( x + w + 6, cur_y_side, dormant ? D3DCOLOR_RGBA ( 150, 150, 150, std::clamp< int > ( box_alpha * 60.0f, 0, 60 ) ) : ( true ? D3DCOLOR_RGBA ( indicator_on_clr.r, indicator_on_clr.g, indicator_on_clr.b, static_cast< int >( static_cast< float >( indicator_on_clr.a )* box_alpha ) ) : D3DCOLOR_RGBA ( indicator_off_clr.r, indicator_off_clr.g, indicator_off_clr.b, static_cast< int >( static_cast< float >( indicator_off_clr.a )* box_alpha ) ) ), indicator_font, _ ( L"BAIM FATAL" ), true, false );
		cur_y_side += 16;
	}

	/* health bar */
	if ( health ) {
		render::outline( x, cur_y, w, 4, D3DCOLOR_RGBA( 0, 0, 0, std::clamp< int > ( box_alpha * 60.0f, 0, 60 ) ) );
		render::rectangle ( x + 1, cur_y + 1, static_cast< float >( w )* ( static_cast< float >( health_clamped ) / 100.0f ) - 1, 4 - 1, dormant ? D3DCOLOR_RGBA ( 150, 150, 150, static_cast< int >( static_cast< float >( health_clr.a )* box_alpha ) ) : D3DCOLOR_RGBA ( health_clr.r, health_clr.g, health_clr.b, static_cast< int >( static_cast< float >( health_clr.a )* box_alpha ) ) );

		cur_y += 6;
	}

	/* desync bar */
	if ( desync ) {
		render::outline( x, cur_y, w, 4, D3DCOLOR_RGBA( 0, 0, 0, std::clamp< int > ( box_alpha * 60.0f, 0, 60 ) ) );
		render::rectangle ( x + 1, cur_y + 1, static_cast< float >( w )* ( static_cast< float >( desync_clamped ) / 58.0f ) - 1, 4 - 1, dormant ? D3DCOLOR_RGBA ( 150, 150, 150, static_cast< int >( static_cast< float >( desync_clr.a )* box_alpha ) ) : D3DCOLOR_RGBA ( desync_clr.r, desync_clr.g, desync_clr.b, static_cast< int >( static_cast< float >( desync_clr.a )* box_alpha ) ) );
		
		cur_y += 6;
	}

	if ( weapon ) {
		render::dim dim;
		render::text_size( esp_font, esp_data [ pl->idx ( ) ].m_weapon_name, dim );
		render::text( x + w / 2 - dim.w / 2, cur_y + 2, dormant ? D3DCOLOR_RGBA ( 150, 150, 150, static_cast< int >( static_cast< float >( name_clr.a )* box_alpha ) ) : D3DCOLOR_RGBA( name_clr.r, name_clr.g, name_clr.b, static_cast< int >( static_cast< float >( name_clr.a )* box_alpha ) ), esp_font, esp_data [ pl->idx ( ) ].m_weapon_name, true );

		cur_y += dim.h + 6;
	}

	//int overlay_log_count = 6;
	//
	//auto get_overlay_log = [ & ] ( ) {
	//	const auto ret = std::to_wstring( overlay_log_count ) + _(L" : ") + std::to_wstring ( animations::data::overlays [ pl->idx ( ) ][ overlay_log_count ].m_weight ) + _ ( L", " ) + std::to_wstring ( animations::data::overlays [ pl->idx ( ) ][ overlay_log_count ].m_playback_rate ) + _ ( L", " ) + std::to_wstring ( animations::data::overlays [ pl->idx ( ) ][ overlay_log_count ].m_cycle ) + _ ( L", " ) + std::to_wstring ( animations::data::overlays [ pl->idx ( ) ][ overlay_log_count ].m_sequence );
	//	overlay_log_count++;
	//	return ret;
	//};
	//
	//for ( auto i = 6; i < 7; i++ ) {
	//	render::text ( x + w + 6, cur_y_side, D3DCOLOR_RGBA( 255, 255, 255, 255 ), indicator_font, get_overlay_log ( ), true, false );
	//	cur_y_side += 16;
	//}
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
	OPTION( bool, esp, "Sesame->C->ESP->Main->ESP", oxui::object_checkbox );
	OPTION( bool, box, "Sesame->C->ESP->Main->Box", oxui::object_checkbox );
	OPTION ( bool, name, "Sesame->C->ESP->Main->Name", oxui::object_checkbox );
	OPTION( bool, team, "Sesame->C->ESP->Targets->Team", oxui::object_checkbox );
	OPTION( bool, enemy, "Sesame->C->ESP->Targets->Enemy", oxui::object_checkbox );
	OPTION ( bool, local, "Sesame->C->ESP->Targets->Local", oxui::object_checkbox );
	OPTION( double, esp_fade_time, "Sesame->C->ESP->Main->ESP Fade Time", oxui::object_slider );

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

	if ( !esp )
		return;

	for ( auto i = 1; i <= csgo::i::globals->m_max_clients; i++ ) {
		auto e = csgo::i::ent_list->get< player_t* >( i );

		if ( !e ) {
			esp_data [ i ].m_pl = nullptr;
			continue;
		}

		if ( !enemy
			&& e->team ( ) != g::local->team ( ) ) {
			esp_data [ e->idx ( ) ].m_pl = nullptr;
			continue;
		}

		if ( !team
			&& e->team ( ) == g::local->team ( )
			&& e != g::local ) {
			esp_data [ e->idx ( ) ].m_pl = nullptr;
			continue;
		}

		if ( !local
			&& e == g::local ) {
			esp_data [ e->idx ( ) ].m_pl = nullptr;
			continue;
		}

		if ( ( enemy || team )
			&& !e->alive ( ) ) {
			esp_data [ e->idx ( ) ].m_pl = nullptr;
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

		auto dormant_time = std::max< float > ( esp_fade_time, 0.1f );

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

			if ( box )
				draw_esp_box ( left, top, right - left, bottom - top, esp_data [ e->idx ( ) ].m_dormant );

			if ( name )
				draw_nametag ( left, top, right - left, bottom - top, wname, esp_data [ e->idx ( ) ].m_dormant );

			draw_bars ( left, top, right - left, bottom - top, e->health ( ), e->desync_amount ( ), e, esp_data [ e->idx ( ) ].m_dormant );
		}
	}
}