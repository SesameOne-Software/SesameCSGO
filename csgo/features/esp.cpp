#include "esp.hpp"
#include "../menu/menu.hpp"
#include "../globals.hpp"
#include "../animations/animations.hpp"

ID3DXFont* features::esp::indicator_font = nullptr;
ID3DXFont* features::esp::esp_font = nullptr;

void features::esp::draw_esp_box( int x, int y, int w, int h ) {
	static auto& esp_clr = oxui::theme.main;

	render::outline( x - 1, y - 1, w + 2, h + 2, D3DCOLOR_RGBA( 0, 0, 0, 17 ) );
	render::outline( x, y, w, h, D3DCOLOR_RGBA( esp_clr.r, esp_clr.g, esp_clr.b, esp_clr.a ) );
}

void features::esp::draw_nametag( int x, int y, int w, int h, const std::wstring_view& name ) {
	const auto name_tag_dim = 16;

	static auto& esp_clr = oxui::theme.main;

	render::dim dim;
	render::text_size( esp_font, name, dim );
	const auto box_w = w < ( dim.w + name_tag_dim ) ? ( dim.w + name_tag_dim ) : w;

	render::rectangle( w < ( dim.w + name_tag_dim ) ? ( x - ( dim.w - w + name_tag_dim ) / 2 ) : x, y - name_tag_dim - 4, box_w, name_tag_dim, D3DCOLOR_RGBA( 0, 0, 0, 28 ) );
	render::rectangle( w < ( dim.w + name_tag_dim ) ? ( x - ( dim.w - w + name_tag_dim ) / 2 ) : x + 1, y - name_tag_dim - 4 + 1, box_w - 1, 2, D3DCOLOR_RGBA( esp_clr.r, esp_clr.g, esp_clr.b, esp_clr.a ) );
	render::text( x + w / 2 - dim.w / 2, y - name_tag_dim - 4 + name_tag_dim / 2 - dim.h / 2, D3DCOLOR_RGBA( 255, 255, 255, 255 ), esp_font, name.data( ) );
}

uint32_t features::esp::color_variable_weight( float val, float cieling ) {
	const auto clamped = std::clamp( val, 0.0f, cieling );
	const auto scaled = static_cast< int >( clamped * ( 255.0f / cieling ) );

	return D3DCOLOR_RGBA( scaled, 255 - scaled, 0, 255 );
}

void features::esp::draw_bars( int x, int y, int w, int h, int health_amount, float desync_amount, player_t* pl ) {
	FIND( bool, lc, "Visuals", "ESP", "Lag-Comp", oxui::object_checkbox );
	FIND( bool, balance_979, "Visuals", "ESP", "979", oxui::object_checkbox );
	FIND( bool, health, "Visuals", "ESP", "Health", oxui::object_checkbox );
	FIND( bool, desync, "Visuals", "ESP", "Desync", oxui::object_checkbox );
	FIND( bool, weapon, "Visuals", "ESP", "Weapon", oxui::object_checkbox );

	const int desync_clamped = std::clamp( desync_amount, 0.0f, 58.0f );
	const int health_clamped = std::clamp( health_amount, 0, 100 );

	auto cur_y = y + h + 4;
	auto cur_y_side = y;

	/* 979 activity ( balance adjust ) */
	if ( balance_979 ) {
		auto balance_adjusting = false;

		for ( auto& layer : pl->overlays( ) )
			if ( layer.m_sequence == 979 )
				balance_adjusting = true;

		render::text( x + w + 6, cur_y_side, balance_adjusting ? D3DCOLOR_RGBA( 0, 255, 0, 255 ) : D3DCOLOR_RGBA( 255, 0, 0, 255 ), indicator_font, _( L"979" ) );

		cur_y_side += 24;
	}

	/* lag-comp break indicator */
	if ( lc
		&& animations::data::old_origin [ pl->idx( ) ] != vec3_t( FLT_MAX, FLT_MAX, FLT_MAX )
		&& animations::data::origin [ pl->idx( ) ] != vec3_t( FLT_MAX, FLT_MAX, FLT_MAX ) ) {
		render::text( x + w + 6, cur_y_side, color_variable_weight( animations::data::old_origin[ pl->idx( ) ].dist_to_sqr( animations::data::origin [ pl->idx( ) ] ), 4096.0f ), indicator_font, _( L"LC" ) );
		cur_y_side += 24;
	}

	/* health bar */
	if ( health ) {
		render::outline( x, cur_y, w, 4, D3DCOLOR_RGBA( 0, 0, 0, 17 ) );
		render::rectangle( x + 1, cur_y + 1, static_cast< float >( w )* ( static_cast< float >( health_clamped ) / 100.0f ) - 1, 4 - 1, D3DCOLOR_RGBA( 0, 255, 0, 150 ) );
		cur_y += 6;
	}

	/* desync bar */
	if ( desync ) {
		render::outline( x, cur_y, w, 4, D3DCOLOR_RGBA( 0, 0, 0, 17 ) );
		render::rectangle( x + 1, cur_y + 1, static_cast< float >( w )* ( static_cast< float >( desync_clamped ) / 58.0f ) - 1, 4 - 1, D3DCOLOR_RGBA( 88, 37, 196, 150 ) );
		cur_y += 6;
	}

	if ( weapon ) {
		const auto name_tag_dim = 16;

		static auto& esp_clr = oxui::theme.main;

		std::wstring weapon_name = L"-";

		if ( pl && pl->weapon( ) && pl->weapon( )->data( ) ) {
			std::string hud_name = pl->weapon( )->data( )->m_weapon_name;

			hud_name.erase( 0, 6 );

			for ( auto& character : hud_name ) {
				if ( character == '_' ) {
					character = ' ';
					continue;
				}

				character = std::tolower( character );
			}

			weapon_name = std::wstring( hud_name.begin( ), hud_name.end( ) );
		}

		render::dim dim;
		render::text_size( esp_font, weapon_name, dim );
		const auto box_w = w < ( dim.w + name_tag_dim ) ? ( dim.w + name_tag_dim ) : w;

		cur_y += name_tag_dim + 4;

		render::rectangle( w < ( dim.w + name_tag_dim ) ? ( x - ( dim.w - w + name_tag_dim ) / 2 ) : x, cur_y - 24 - 4, box_w, name_tag_dim, D3DCOLOR_RGBA( 0, 0, 0, 28 ) );
		render::rectangle( w < ( dim.w + name_tag_dim ) ? ( x - ( dim.w - w + name_tag_dim ) / 2 ) : x + 1, cur_y - 4 - 3, box_w - 1, 2, D3DCOLOR_RGBA( esp_clr.r, esp_clr.g, esp_clr.b, esp_clr.a ) );
		render::text( x + w / 2 - dim.w / 2, cur_y - name_tag_dim - 4 + name_tag_dim / 2 - dim.h / 2, D3DCOLOR_RGBA( 255, 255, 255, 255 ), esp_font, weapon_name );

		cur_y += dim.h + 2;
	}
}

void features::esp::render( ) {
	FIND( bool, esp, "Visuals", "ESP", "ESP", oxui::object_checkbox );
	FIND( bool, box, "Visuals", "ESP", "Box", oxui::object_checkbox );
	FIND( bool, name, "Visuals", "ESP", "Name", oxui::object_checkbox );

	FIND( bool, team, "Visuals", "Targets", "Team", oxui::object_checkbox );
	FIND( bool, enemy, "Visuals", "Targets", "Enemy", oxui::object_checkbox );
	FIND( bool, local, "Visuals", "Targets", "Local", oxui::object_checkbox );
	FIND( bool, weapon, "Visuals", "Targets", "Weapon", oxui::object_checkbox );

	if ( !g::local )
		return;

	if ( !esp )
		return;

	for ( auto i = 1; i <= csgo::i::globals->m_max_clients; i++ ) {
		auto e = csgo::i::ent_list->get< player_t* >( i );

		if ( !e )
			continue;

		if ( !enemy
			&& e->team( ) != g::local->team( ) )
			continue;

		if ( !team
			&& e->team( ) == g::local->team( )
			&& e != g::local )
			continue;

		if ( !local
			&& e == g::local )
			continue;

		if ( !weapon
			&& e->client_class( )
			&& !std::strcmp( e->client_class( )->m_network_name, _( "CWeapon" ) ) )
			continue;

		if ( ( enemy || team )
			&& ( !e->alive( ) || e->dormant( ) ) )
			continue;

		vec3_t flb, brt, blb, frt, frb, brb, blt, flt;
		float left, top, right, bottom;

		vec3_t min = e->mins( ) + e->abs_origin( );
		vec3_t max = e->maxs( ) + e->abs_origin( );

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

		player_info_t info;
		csgo::i::engine->get_player_info( e->idx( ), &info );
		std::string str = info.m_name;

		if ( box )
			draw_esp_box( left, top, right - left, bottom - top );

		if ( name )
			draw_nametag( left, top, right - left, bottom - top, std::wstring( str.begin( ), str.end( ) ) );

		draw_bars( left, top, right - left, bottom - top, e->health( ), e->desync_amount( ), e );
	}
}