#ifndef OXUI_PANEL_HPP
#define OXUI_PANEL_HPP

#include <memory>
#include <vector>
#include "../types/types.hpp"
#include "../objects/object.hpp"
#include "../objects/window.hpp"

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

		panel( ) { reset( ); type = object_panel; }
		~panel( ) { }
		
		void destroy( ) {
			std::for_each( fonts.begin( ), fonts.end( ), [ ] ( const std::pair<str, font>& f ) {
				( ( ID3DXFont* ) f.second )->Release( );
				} );
		}

		void reset( ) {
			binds::create_font( OSTR("Segoe UI"), 16, true, fonts [ OSTR( "title") ] );
			binds::create_font( OSTR("Segoe UI Light"), 16, false, fonts [ OSTR( "object") ] );
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