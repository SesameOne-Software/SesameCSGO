#ifndef OXUI_PANEL_HPP
#define OXUI_PANEL_HPP

#include <memory>
#include <vector>
#include "../types/types.hpp"
#include "../objects/object.hpp"
#include "../objects/window.hpp"
#include "../../sesame_font.h"
#include "../../sesame_logo.h"

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

		panel( ) { reset( ); type = object_panel; }
		~panel( ) { }

		void destroy( ) {
			std::for_each( fonts.begin( ), fonts.end( ), [ ] ( const std::pair<str, font>& f ) {
				reinterpret_cast< ID3DXFont* > ( f.second )->Release( );
				} );

			sprite->Release ( );
			tex->Release ( );
		}

		void reset( ) {
			binds::create_font ( OSTR ( "Arial" ), 10, false, fonts [ OSTR ( "watermark" ) ] );
			binds::create_font( OSTR( "Arial" ), 16, false, fonts [ OSTR( "title" ) ] );
			binds::create_font( OSTR( "Arial" ), 16, false, fonts [ OSTR( "object" ) ] );
			binds::create_font ( OSTR ( "Arial" ), 16, true, fonts [ OSTR ( "group" ) ] );

			D3DXCreateFontW ( csgo::i::dev, 13, 0, FW_NORMAL, 0, false, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, _ ( L"sesame" ), reinterpret_cast< LPD3DXFONT* > ( &fonts [ OSTR ( "check" ) ] ) );
			D3DXCreateFontW ( csgo::i::dev, 21, 0, FW_NORMAL, 0, false, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, _ ( L"sesame" ), reinterpret_cast< LPD3DXFONT* > ( &fonts [ OSTR ( "tab" ) ] ) );
	
			D3DXCreateSprite ( csgo::i::dev, &sprite );
			D3DXCreateTextureFromFileInMemory ( csgo::i::dev, sesame_logo_data, 4 * 761 * 541, &tex );
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