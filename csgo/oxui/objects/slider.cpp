#include "slider.hpp"
#include "../panels/panel.hpp"
#include "shapes.hpp"
#include "../themes/purple.hpp"

void oxui::slider::think( ) {
	area.h = theme.spacing + theme.spacing / 2;

	auto& parent_panel = find_parent< panel >( object_panel );
	auto& parent_window = find_parent< window >( object_window );
	auto& cursor_pos = parent_window.cursor_pos;
	animate( rect( cursor_pos.x, cursor_pos.y, area.w, area.h ) );

	/* copied from rendering function */
	auto slider_dimensions = rect( 0, 0, area.w - theme.spacing, 6 );
	auto start_x = cursor_pos.x + theme.spacing / 2;

	/* if started clicking slider, do slider logic */
	if ( shapes::hovering( rect( cursor_pos.x, cursor_pos.y, area.w, area.h ), true ) ) {
		pos mouse_pos;
		binds::mouse_pos( mouse_pos );

		auto mouse_offset = std::clamp( mouse_pos.x - start_x, 0, slider_dimensions.w );
		auto previous_val = value;
		value = double( mouse_offset ) * ( ( max - min ) / slider_dimensions.w );

		if ( previous_val != value ) {
			changed_time = parent_panel.time;
			old_value = previous_val;
		}
	}
}

void oxui::slider::draw( ) {
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
	auto area_center_y1 = cursor_pos.y + theme.spacing;
	auto slider_dimensions = rect( 0, 0, area.w - theme.spacing, 6 );
	auto start_x = cursor_pos.x + theme.spacing / 2;

	/* centered text */
	binds::text( pos( start_x, area_center_y - text_size.h / 2 - 1 ), font, label, theme.text, true );
	
	/* slider box */
	binds::rect( rect( start_x, area_center_y1 - slider_dimensions.h / 2, slider_dimensions.w, slider_dimensions.h ), theme.main );
	
	/* slider inner box*/
	auto slide_w_old_value = std::clamp( int( double( slider_dimensions.w ) * ( old_value / ( max - min ) ) ), 0, slider_dimensions.w - 1 );
	auto slide_w_new_value = std::clamp( int( double( slider_dimensions.w ) * ( value / ( max - min ) ) ), 0, slider_dimensions.w - 1 );
	auto slide_w = slide_w_old_value + ( std::clamp( parent_panel.time - changed_time, 0.0, theme.animation_speed ) / theme.animation_speed ) * ( slide_w_new_value - slide_w_old_value );
	binds::fill_rect( rect( start_x + 1, area_center_y1 - slider_dimensions.h / 2 + 1, slide_w, slider_dimensions.h - 1 ), color( theme.main.r, theme.main.g, theme.main.b, 125 ) );

	/* slider value popup */
	wchar_t text_value [ 64 ];
	swprintf_s( text_value, OSTR( "%0.1f"), value );
	rect value_text_size;
	binds::text_bounds( font, text_value, value_text_size );
	auto value_annotation_alpha = fade_timer > theme.animation_speed ? 255 : int( fade_timer * ( 1.0 / theme.animation_speed ) * 255.0 );
	binds::fill_rect( rect( start_x + 1 + slide_w - value_text_size.w / 2 - 2, area_center_y1 - slider_dimensions.h / 2 + 1 - value_text_size.h - 6, value_text_size.w + 4, value_text_size.h  + 4 ), color( theme.bg.r, theme.bg.g, theme.bg.b, value_annotation_alpha ) );
	binds::text( pos( start_x + 1 + slide_w - value_text_size.w / 2, area_center_y1 - slider_dimensions.h / 2 + 1 - value_text_size.h - 4 ), font, text_value, color( theme.text.r, theme.text.g, theme.text.b, value_annotation_alpha ), true );

	cursor_pos.y += theme.spacing / 2;
}