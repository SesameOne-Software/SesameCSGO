#include "panel.hpp"
#include "../objects/shapes.hpp"

void oxui::panel::render( double t ) {
	binds::screen_size( area );

	time = t;

	/* draw all windows */
	binds::clip( area, [ & ] ( ) {
		std::for_each(
			windows.begin( ),
			windows.end( ),
			[ ] ( std::shared_ptr< window >& child ) {
				child->draw( );
			}
		);
		} );

	if ( !shapes::click_switch && GetAsyncKeyState( VK_LBUTTON ) )
		shapes::click_switch = true;
}