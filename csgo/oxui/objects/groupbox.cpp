#include "groupbox.hpp"
#include "../panels/panel.hpp"
#include "shapes.hpp"
#include "../themes/purple.hpp"

void oxui::group::think( ) {
	auto& parent_window = find_parent< window >( object_window );
	animate( rect( parent_window.cursor_pos.x, parent_window.cursor_pos.y, area.w, area.h ) );
}

void oxui::group::draw( ) {
	think( );

	auto& parent_panel = find_parent< panel >( object_panel );
	auto& parent_window = find_parent< window >( object_window );

	auto& tfont = parent_panel.fonts [ OSTR( "title" ) ];
	auto& font = parent_panel.fonts [ OSTR( "object" ) ];

	/* reset draw cursor pos */
	auto& cursor_pos = parent_window.cursor_pos;

	shapes::box( rect( cursor_pos.x, cursor_pos.y, area.w, area.h ), fade_timer, true, true, true, true, true );
	binds::fill_rect( rect( cursor_pos.x, cursor_pos.y, area.w, 26 ), theme.title_bar );
	shapes::box( rect( cursor_pos.x, cursor_pos.y, area.w, 26 ), fade_timer, true, true, false, true, true, false );
	shapes::box( rect( cursor_pos.x, cursor_pos.y, area.w, 26 ), 0.0, false, false, true, false, false, false );
	binds::text( pos( cursor_pos.x + 6, cursor_pos.y + 4 ), tfont, title, theme.title_text, false );

	/* move all objects inside group */
	cursor_pos.x += theme.spacing;
	cursor_pos.y += theme.spacing * 2;

	/* draw group objects */
	binds::clip( rect( cursor_pos.x, cursor_pos.y, area.w, area.h ), [ & ] ( ) {
		std::for_each(
			objects.begin( ),
			objects.end( ),
			[ & ] ( std::shared_ptr< obj >& child ) {
				child->area = rect( 0, 0, area.w - theme.spacing * 2, theme.spacing );

				child->draw_ex( );
			}
		);
		} );
}