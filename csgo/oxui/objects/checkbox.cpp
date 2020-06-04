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

	auto& font = parent_panel.fonts [ OSTR( "object" ) ];

	auto& cursor_pos = parent_window.cursor_pos;

	const auto fade_alpha = fade_timer > theme.animation_speed ? 90 : int ( fade_timer * ( 1.0 / theme.animation_speed ) * 90.0 );

	/* highlighted selection */
	if ( fade_alpha ) {
		binds::rounded_rect ( { cursor_pos.x, cursor_pos.y, area.w, area.h }, 8, 16, { 0, 0, 0, fade_alpha }, false );
		binds::rounded_rect ( { cursor_pos.x, cursor_pos.y, area.w, area.h }, 8, 16, { 0, 0, 0, fade_alpha }, true );
	}

	rect text_size;
	binds::text_bounds( font, label, text_size );

	auto area_center_y = cursor_pos.y + theme.spacing / 2;
	auto check_dimensions = rect( 0, 0, 16, 16 );

	/* centered text */
	binds::text( pos( cursor_pos.x + 6, area_center_y - text_size.h / 2 - 1 ), font, label, theme.text, false );

	/* check box */
	binds::rounded_rect ( { cursor_pos.x + area.w - check_dimensions.w - 6, area_center_y - check_dimensions.h / 2, check_dimensions.w, check_dimensions.h }, 4, 4, theme.container_bg, true );

	/* check mark */
	auto max_offset = check_dimensions.w - ( check_dimensions.h - 3 ) - 3;
	auto time_since_click = std::clamp( parent_panel.time - checked_time, 0.0, theme.animation_speed );
	auto check_alpha = int( theme.main.a - double( theme.main.a ) * ( time_since_click * ( 1.0 / theme.animation_speed ) ) );

	if ( checked )
		check_alpha = int( double( theme.main.a ) * ( time_since_click * ( 1.0 / theme.animation_speed ) ) );

	check_alpha = std::clamp( check_alpha, 0, theme.main.a );
	
	binds::rounded_rect ( { cursor_pos.x + area.w - check_dimensions.w - 6, area_center_y - check_dimensions.h / 2, check_dimensions.w, check_dimensions.h }, 4, 4, { theme.main.r, theme.main.g, theme.main.b, check_alpha }, false );
	binds::text ( { cursor_pos.x + area.w - check_dimensions.w - 6 + 3, area_center_y - check_dimensions.h / 2 + 3 }, parent_panel.fonts [ OSTR ( "check" ) ], OSTR ( "I" ), { theme.title_text.r, theme.title_text.g, theme.title_text.b, check_alpha }, false );
}