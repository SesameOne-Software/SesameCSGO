#ifndef OXUI_PANEL_HPP
#define OXUI_PANEL_HPP

#include <memory>
#include <vector>
#include "../types/types.hpp"
#include "../objects/object.hpp"
#include "../objects/window.hpp"
#include "../../sesame_font.h"
#include "../../sesame_logo.h"
#include "../../player_images.h"

namespace oxui {
	/*
	*	INFO: Constructs a panel; an area in which objects, windows, and forms can be drawn in.
	*	These items are contained inside the panel, and are processed & drawn inside the panel's bounds.
	*
	*	USAGE: const auto main_panel = std::make_shared< oxui::panel >( oxui::rect( 0, 0, screen.w, screen.h ) );
	*/
	class panel : public obj {
		std::vector< std::shared_ptr< window > > windows;

	public:
		std::map< str, font > fonts;
		ID3DXSprite* sprite = nullptr;
		IDirect3DTexture9* tex = nullptr;

		struct {
			struct {
				ID3DXSprite* sprite = nullptr;
				IDirect3DTexture9* tex = nullptr;
			} flat;

			struct {
				ID3DXSprite* sprite = nullptr;
				IDirect3DTexture9* tex = nullptr;
			} glow;

			struct {
				ID3DXSprite* sprite = nullptr;
				IDirect3DTexture9* tex = nullptr;
			} lighting;

			struct {
				ID3DXSprite* sprite = nullptr;
				IDirect3DTexture9* tex = nullptr;
			} normal;

			struct {
				ID3DXSprite* sprite = nullptr;
				IDirect3DTexture9* tex = nullptr;
			} rimlight;
		} player_img;

		panel( ) { reset( ); type = object_panel; }
		~panel( ) { }

		void destroy( ) {
			std::for_each( fonts.begin( ), fonts.end( ), [ ] ( const std::pair<str, font>& f ) {
				reinterpret_cast< ID3DXFont* > ( f.second )->Release( );
				} );

			player_img.flat.sprite->Release ( );
			player_img.glow.sprite->Release ( );
			player_img.lighting.sprite->Release ( );
			player_img.normal.sprite->Release ( );
			player_img.rimlight.sprite->Release ( );
			player_img.flat.tex->Release ( );
			player_img.glow.tex->Release ( );
			player_img.lighting.tex->Release ( );
			player_img.normal.tex->Release ( );
			player_img.rimlight.tex->Release ( );

			sprite->Release ( );
			tex->Release ( );
		}

		void reset( ) {
			binds::create_font ( OSTR ( "Segoe UI" ), 10, false, fonts [ OSTR ( "watermark" ) ] );
			binds::create_font( OSTR( "Segoe UI" ), 18, false, fonts [ OSTR( "title" ) ] );
			binds::create_font( OSTR( "Segoe UI" ), 18, false, fonts [ OSTR( "object" ) ] );
			binds::create_font ( OSTR ( "Segoe UI" ), 18, true, fonts [ OSTR ( "group" ) ] );

			D3DXCreateFontW ( csgo::i::dev, 13, 0, FW_NORMAL, 0, false, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, _ ( L"sesame" ), reinterpret_cast< LPD3DXFONT* > ( &fonts [ OSTR ( "check" ) ] ) );
			D3DXCreateFontW ( csgo::i::dev, 21, 0, FW_NORMAL, 0, false, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, _ ( L"sesame" ), reinterpret_cast< LPD3DXFONT* > ( &fonts [ OSTR ( "tab" ) ] ) );
	
			D3DXCreateSprite ( csgo::i::dev, &player_img.flat.sprite );
			D3DXCreateSprite ( csgo::i::dev, &player_img.glow.sprite );
			D3DXCreateSprite ( csgo::i::dev, &player_img.lighting.sprite );
			D3DXCreateSprite ( csgo::i::dev, &player_img.normal.sprite );
			D3DXCreateSprite ( csgo::i::dev, &player_img.rimlight.sprite );
			D3DXCreateTextureFromFileInMemory ( csgo::i::dev, player_flat, 4 * 200 * 420, &player_img.flat.tex );
			D3DXCreateTextureFromFileInMemory ( csgo::i::dev, player_glow, 4 * 200 * 420, &player_img.glow.tex );
			D3DXCreateTextureFromFileInMemory ( csgo::i::dev, player_lighting, 4 * 200 * 420, &player_img.lighting.tex );
			D3DXCreateTextureFromFileInMemory ( csgo::i::dev, player_normal, 4 * 200 * 420, &player_img.normal.tex );
			D3DXCreateTextureFromFileInMemory ( csgo::i::dev, player_rimlight, 4 * 200 * 420, &player_img.rimlight.tex );

			D3DXCreateSprite ( csgo::i::dev, &sprite );
			D3DXCreateTextureFromFileInMemory ( csgo::i::dev, sesame_logo_data, 4 * 150 * 107, &tex );
		}

		void add_window( const std::shared_ptr< window >& new_window ) {
			new_window->parent = this;
			windows.push_back( new_window );
		}

		void draw( )override {}

		/* call to render all windows */
		void render( double t );
	};
}

#endif // OXUI_PANEL_HPP