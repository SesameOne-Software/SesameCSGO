#include "checkbox.hpp"
#include "../panels/panel.hpp"
#include "shapes.hpp"
#include "../themes/purple.hpp"

void oxui::checkbox::think( ) {
	auto& parent_window = find_parent< window >( object_window );
	auto& cursor_pos = parent_window.cursor_pos;
	animate( rect( cursor_pos.x, cursor_pos.y, area.w, area.h ) );
}

void oxui::checkbox::draw( ) {
	think( );

	auto& parent_panel = find_parent< panel >( object_panel );
	auto& parent_window = find_parent< window >( object_window );

	auto& font = parent_panel.fonts [ OSTR( "object") ];

	auto& cursor_pos = parent_window.cursor_pos;

	/* highlighted selection */
	shapes::box( rect( cursor_pos.x, cursor_pos.y, area.w, area.h ), fade_timer, true, true, true, false, false, false );

	rect text_size;
	binds::text_bounds( font, label, text_size );

	auto area_center_y = cursor_pos.y + theme.spacing / 2;
	auto check_dimensions = rect( 0, 0, 10, 6 );

	/* centered text */
	binds::text( pos( cursor_pos.x + 6, area_center_y - text_size.h / 2 - 1 ), font, label, theme.text, true );
	
	/* check box */
	binds::rect( rect( cursor_pos.x + area.w - check_dimensions.w - 6, area_center_y - check_dimensions.h / 2, check_dimensions.w, check_dimensions.h ), theme.main );
	
	/* check mark */
	auto max_offset = check_dimensions.w - ( check_dimensions.h - 3 ) - 3;
	auto time_since_click = std::clamp( parent_panel.time - checked_time, 0.0, theme.animation_speed );
	auto check_offset_x = int( max_offset - double( max_offset ) * ( time_since_click * ( 1.0 / theme.animation_speed ) ) );

	if ( checked )
		check_offset_x = int( double( max_offset ) * ( time_since_click * ( 1.0 / theme.animation_speed ) ) );

	check_offset_x = std::clamp( check_offset_x, 0, max_offset );

	binds::fill_rect( rect( cursor_pos.x + area.w - check_dimensions.w - 6 + 1 + check_offset_x, area_center_y - check_dimensions.h / 2 + 1, check_dimensions.h - 1, check_dimensions.h - 1 ), color( theme.main.r, theme.main.g, theme.main.b, 125 ) );
}