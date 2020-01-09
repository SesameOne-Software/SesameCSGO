#include <oxui.h>
#include <json.h>
#include <fstream>
#include <ShlObj.h>
#include "menu.h"
#include "../globals.h"
#include "../sdk/sdk.h"

IDirect3DTexture9* menu::desync_none = nullptr;
IDirect3DTexture9* menu::desync_add = nullptr;
IDirect3DTexture9* menu::desync_sub = nullptr;

void menu::draw_desync_model( desync_types desync_type, float real, float x, float y ) {
	ID3DXSprite* sprite = nullptr;
	D3DXCreateSprite( csgo::i::dev, &sprite );
	sprite->Begin( D3DXSPRITE_ALPHABLEND );

	D3DXMATRIX transform_mat;
	D3DXVECTOR2 scaling_center( 256.0f / 2.0f, 256.0f / 2.0f );
	D3DXVECTOR2 scaling( 0.42f, 0.42f );
	D3DXVECTOR2 transform_center( 256.0f / 2.0f, 256.0f / 2.0f );
	D3DXVECTOR2 transform_pos( x - 256.0f / 4.0f, y - 256.0f / 4.0f );

	D3DXMatrixTransformation2D( &transform_mat, &scaling_center, 0.0f, &scaling, &transform_center, real * ( csgo::pi / 180.0f ), &transform_pos );
	sprite->SetTransform( &transform_mat );

	static auto counter = 0;
	static auto side = false;

	if ( counter > 3 ) {
		side = !side;
		counter = 0;
	}

	counter++;

	switch ( desync_type ) {
	case desync_types::none:
		/* real */
		D3DXMatrixTransformation2D( &transform_mat, &scaling_center, 0.0f, &scaling, &transform_center, real * ( csgo::pi / 180.0f ), &transform_pos );
		sprite->SetTransform( &transform_mat );
		sprite->Draw( desync_none, nullptr, nullptr, nullptr, D3DCOLOR_RGBA( 255, 255, 255, 255 ) );
		break;
	case desync_types::additive:
		/* fake */
		D3DXMatrixTransformation2D( &transform_mat, &scaling_center, 0.0f, &scaling, &transform_center, ( real - 60.0f ) * ( csgo::pi / 180.0f ), &transform_pos );
		sprite->SetTransform( &transform_mat );
		sprite->Draw( desync_none, nullptr, nullptr, nullptr, D3DCOLOR_RGBA( 255, 50, 255, 255 ) );

		/* real */
		D3DXMatrixTransformation2D( &transform_mat, &scaling_center, 0.0f, &scaling, &transform_center, real * ( csgo::pi / 180.0f ), &transform_pos );
		sprite->SetTransform( &transform_mat );
		sprite->Draw( desync_add, nullptr, nullptr, nullptr, D3DCOLOR_RGBA( 255, 255, 255, 255 ) );
		break;
	case desync_types::subtractive:
		/* fake */
		D3DXMatrixTransformation2D( &transform_mat, &scaling_center, 0.0f, &scaling, &transform_center, ( real + 60.0f ) * ( csgo::pi / 180.0f ), &transform_pos );
		sprite->SetTransform( &transform_mat );
		sprite->Draw( desync_none, nullptr, nullptr, nullptr, D3DCOLOR_RGBA( 255, 50, 255, 255 ) );

		/* real */
		D3DXMatrixTransformation2D( &transform_mat, &scaling_center, 0.0f, &scaling, &transform_center, real * ( csgo::pi / 180.0f ), &transform_pos );
		sprite->SetTransform( &transform_mat );
		sprite->Draw( desync_sub, nullptr, nullptr, nullptr, D3DCOLOR_RGBA( 255, 255, 255, 255 ) );
		break;
	case desync_types::outward:
		/* fake */
		D3DXMatrixTransformation2D( &transform_mat, &scaling_center, 0.0f, &scaling, &transform_center, ( real + ( side ? 60.0f : -60.0f ) ) * ( csgo::pi / 180.0f ), &transform_pos );
		sprite->SetTransform( &transform_mat );

		if ( side )
			sprite->Draw( desync_sub, nullptr, nullptr, nullptr, D3DCOLOR_RGBA( 255, 50, 255, 255 ) );
		else
			sprite->Draw( desync_none, nullptr, nullptr, nullptr, D3DCOLOR_RGBA( 255, 50, 255, 255 ) );

		/* real */
		D3DXMatrixTransformation2D( &transform_mat, &scaling_center, 0.0f, &scaling, &transform_center, real * ( csgo::pi / 180.0f ), &transform_pos );
		sprite->SetTransform( &transform_mat );

		if ( side )
			sprite->Draw( desync_sub, nullptr, nullptr, nullptr, D3DCOLOR_RGBA( 255, 255, 255, 255 ) );
		else
			sprite->Draw( desync_none, nullptr, nullptr, nullptr, D3DCOLOR_RGBA( 255, 255, 255, 255 ) );

		break;
	case desync_types::inward:
		/* fake */
		D3DXMatrixTransformation2D( &transform_mat, &scaling_center, 0.0f, &scaling, &transform_center, real * ( csgo::pi / 180.0f ), &transform_pos );
		sprite->SetTransform( &transform_mat );

		if ( side )
			sprite->Draw( desync_sub, nullptr, nullptr, nullptr, D3DCOLOR_RGBA( 255, 50, 255, 255 ) );
		else
			sprite->Draw( desync_none, nullptr, nullptr, nullptr, D3DCOLOR_RGBA( 255, 50, 255, 255 ) );

		/* real */
		D3DXMatrixTransformation2D( &transform_mat, &scaling_center, 0.0f, &scaling, &transform_center, ( real + ( side ? 60.0f : -60.0f ) ) * ( csgo::pi / 180.0f ), &transform_pos );
		sprite->SetTransform( &transform_mat );

		if ( side )
			sprite->Draw( desync_sub, nullptr, nullptr, nullptr, D3DCOLOR_RGBA( 255, 255, 255, 255 ) );
		else
			sprite->Draw( desync_none, nullptr, nullptr, nullptr, D3DCOLOR_RGBA( 255, 255, 255, 255 ) );

		break;
	}

	sprite->End( );
	sprite->Release( );
}


void menu::save( const std::string& file ) {
	char docs [ MAX_PATH ];
	SHGetFolderPathA( nullptr, CSIDL_PERSONAL, nullptr, SHGFP_TYPE_CURRENT, docs );

	std::ofstream f( docs + std::string( "\\o2\\cfg\\" ) + file );

	if ( f.is_open( ) ) {
		nlohmann::json json;

		for ( const auto& option : oxui::vars::items ) {
			switch ( option.second.type ) {
			case oxui::option_type::boolean:
				json [ "o2" ][ option.first.data( ) ] = option.second.val.b;
				break;
			case oxui::option_type::integer:
				json [ "o2" ][ option.first.data( ) ] = option.second.val.i;
				break;
			case oxui::option_type::floating:
				json [ "o2" ][ option.first.data( ) ] = option.second.val.f;
				break;
			case oxui::option_type::string: {
				std::string str( option.second.val.s );
				json [ "o2" ][ option.first.data( ) ] = str;
			} break;
			case oxui::option_type::color:
				json [ "o2" ][ option.first.data( ) ][ "r" ] = option.second.val.c.r;
				json [ "o2" ][ option.first.data( ) ][ "g" ] = option.second.val.c.g;
				json [ "o2" ][ option.first.data( ) ][ "b" ] = option.second.val.c.b;
				json [ "o2" ][ option.first.data( ) ][ "a" ] = option.second.val.c.a;
				break;
			default:
				break;
			}
		}

		const auto dump = json.dump( );

		f.write( dump.c_str( ), dump.length( ) );
	}
}

void menu::load( const std::string& file ) {
	char docs [ MAX_PATH ];
	SHGetFolderPathA( nullptr, CSIDL_PERSONAL, nullptr, SHGFP_TYPE_CURRENT, docs );

	std::ifstream f( docs + std::string( "\\o2\\cfg\\" ) + file );

	if ( f.is_open( ) ) {
		f.seekg( 0, f.end );
		size_t length = f.tellg( );
		f.seekg( 0, f.beg );

		const auto buf = new char [ length ];

		f.read( buf, length );

		const auto json = nlohmann::json::parse( buf );

		for ( auto& option : oxui::vars::items ) {
			/* failed to find o2 object */
			if ( json.find( "o2" ) == json.end( ) )
				continue;

			const auto base_obj = json [ "o2" ];

			/* if key is not found */
			if ( base_obj.find( option.first ) == base_obj.end( ) )
				continue;

			switch ( option.second.type ) {
			case oxui::option_type::boolean:
				option.second.val.b = base_obj [ option.first.data( ) ];
				break;
			case oxui::option_type::integer:
				option.second.val.i = base_obj [ option.first.data( ) ];
				break;
			case oxui::option_type::floating:
				option.second.val.f = base_obj [ option.first.data( ) ];
				break;
			case oxui::option_type::string: {
				char c_str [ 256 ];
				auto str = base_obj [ option.first.data( ) ].get<std::string>( );
				std::copy( str.begin( ), str.end( ), c_str );
				strcpy_s( option.second.val.s, c_str );
			} break;
			case oxui::option_type::color:
				option.second.val.c.r = base_obj[ option.first.data( ) ][ "r" ];
				option.second.val.c.g = base_obj[ option.first.data( ) ][ "g" ];
				option.second.val.c.b = base_obj[ option.first.data( ) ][ "b" ];
				option.second.val.c.a = base_obj[ option.first.data( ) ][ "a" ];
				break;
			default:
				break;
			}
		}

		delete [ ] buf;
	}
}

void menu::init( ) {
	/* rage_aim */
	oxui::vars::items [ "checkbox" ].val.b = false; oxui::vars::items [ "checkbox" ].type = oxui::option_type::boolean;
	oxui::vars::items [ "dropdown" ].val.b = false; oxui::vars::items [ "dropdown" ].type = oxui::option_type::integer;
	oxui::vars::items [ "slider" ].val.b = false; oxui::vars::items [ "slider" ].type = oxui::option_type::integer;
	oxui::vars::items [ "listbox" ].val.b = false; oxui::vars::items [ "listbox" ].type = oxui::option_type::integer;
}

void menu::render( ) {
	/* menu */
	oxui::update_anims( csgo::i::globals->m_curtime );

	const auto menu_w = 450;
	const auto menu_h = 500;

	/* create a new window */
	if ( oxui::begin( oxui::dimension( menu_w, menu_h ), "blaster", oxui::window_flags::none ) ) {
		/* tabs */
		if ( oxui::begin_tabs( 6 ) ) {
			oxui::tab( "rage", false, oxui::color { 102, 204, 255, 255 } );
			oxui::tab( "legit", false, oxui::color { 102, 181, 255, 255 } );
			oxui::tab( "visuals", false, oxui::color { 102, 148, 255, 255 } );
			oxui::tab( "skins", false, oxui::color { 102, 125, 255, 255 } );
			oxui::tab( "misc", false, oxui::color { 102, 125, 255, 255 } );
			oxui::tab( "scripts", false, oxui::color { 102, 105, 255, 255 } );

			/* end tabs */
			oxui::end_tabs( );
		}
		
		const auto category_w = menu_w / 2 - 12 * 2 - 12 / 2;
		const auto category_h = menu_h - 30 - 12 * 3;
		const auto category_2col_h = menu_h / 2 - 30 - 12 - 12 / 2;
		const auto category_internal_w = category_w - 12 * 3 - 12 / 2;
		const auto category_internal_h = category_h - 12 * 5;

		/* menu items */
		if ( oxui::vars::cur_tab_num ) {
			switch ( oxui::vars::cur_tab_num ) {
			case 1: {
				if ( oxui::begin_category( "aimbot", oxui::dimension( category_w, category_h ) ) ) {
					oxui::checkbox( "checkbox", "checkbox" );
					oxui::dropdown( "dropdown", "dropdown", { "one", "two", "three" } );
					oxui::slider( "slider", "slider", 0, 100, true );
					oxui::button( "button" );
					oxui::listbox( "listbox", "listbox", { "one", "two", "three", "four", "five", "six", "seven", "eight", "nine", "ten", "eleven", "twlve", "thirteen", "fourteen", "fifteen", "sixteen", "seventeen", "eighteen", "nineteen", "twenty" } );

					oxui::end_category( );
				}

				oxui::set_cursor_pos( oxui::point( oxui::vars::pos.x + menu_w / 2 + 12 / 2, oxui::vars::pos.y + 30 + 12 ) );

				if ( oxui::begin_category( "anti-aim", oxui::dimension( category_w, category_h ) ) ) {
					oxui::checkbox( "checkbox", "checkbox" );
					oxui::dropdown( "dropdown", "dropdown", { "1", "2", "3" } );
					oxui::slider( "slider", "slider", 0, 100, true );
					oxui::button( "button" );
					oxui::listbox( "listbox", "listbox", { "one", "two", "three", "four", "five", "six", "seven", "eight", "nine", "ten", "eleven", "twlve", "thirteen", "fourteen", "fifteen", "sixteen", "seventeen", "eighteen", "nineteen", "twenty" } );

					oxui::end_category( );
				}
				break;
			}
			case 2: {
				break;
			}
			case 3: {
				break;
			}
			case 4: {
				break;
			}
			case 5: {
				break;
			}
			case 6: {
				break;
			}
			}
		}

		/* end window */
		oxui::end( );
	}
}