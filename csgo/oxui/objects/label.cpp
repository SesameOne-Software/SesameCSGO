#include "label.hpp"
#include "../panels/panel.hpp"
#include "shapes.hpp"
#include "../themes/purple.hpp"

void oxui::label::think( ) {
	auto& parent_window = find_parent< window >( object_window );
	auto& cursor_pos = parent_window.cursor_pos;
	animate( rect( cursor_pos.x, cursor_pos.y, area.w, area.h ) );
}

void oxui::label::draw( ) {
	think( );

	auto& parent_panel = find_parent< panel >( object_panel );
	auto& parent_window = find_parent< window >( object_window );

	auto& font = parent_panel.fonts [ OSTR( "object" ) ];

	auto& cursor_pos = parent_window.cursor_pos;

	auto hover_highlight_time = theme.animation_speed;
	auto border_max_alpha = fade_timer > hover_highlight_time ? 255 : int ( fade_timer * ( 1.0 / hover_highlight_time ) * 80.0 + 175.0 );

	rect text_size;
	binds::text_bounds( font, text, text_size );

	if ( left_side ) {
		/* left-sided text */
		binds::text ( pos ( cursor_pos.x + theme.spacing, cursor_pos.y + theme.spacing / 2 ), font, text, color ( theme.text.r, theme.text.g, theme.text.b, border_max_alpha ), false );
	}
	else {
		auto area_center_x = cursor_pos.x + area.w / 2;
		auto area_center_y = cursor_pos.y + theme.spacing / 2;
		auto check_dimensions = rect ( 0, 0, 10, 6 );

		/* centered text */
		binds::text ( pos ( area_center_x - text_size.w / 2 - 1, area_center_y - text_size.h / 2 - 1 ), font, text, color ( theme.text.r, theme.text.g, theme.text.b, border_max_alpha ), false );
	}
}