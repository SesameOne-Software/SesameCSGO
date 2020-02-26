#include "object.hpp"
#include "window.hpp"
#include "shapes.hpp"
#include "../themes/purple.hpp"

#include "checkbox.hpp"
#include "slider.hpp"

void oxui::obj::animate( const rect& area ) {
	auto& parent_panel = find_parent< obj >( object_panel );

	fade_timer = parent_panel.time;

	if ( type == object_checkbox ? shapes::clicking( area ) : shapes::hovering( area ) && GetAsyncKeyState( VK_LBUTTON ) ) {
		switch ( type ) {
		case object_checkbox: {
			auto as_checkbox = static_cast< checkbox* >( this );
			as_checkbox->checked = !as_checkbox->checked;
			as_checkbox->checked_time = parent_panel.time;
			break;
		}
		}

		start_click_time = parent_panel.time;
		fading_animation_timer = parent_panel.time;
	}
	else if ( start_click_time != 0.0 ) {
		fade_timer = std::clamp( theme.animation_speed - ( parent_panel.time - start_click_time ) * 0.5, theme.animation_speed / 2.0, theme.animation_speed );

		if ( fade_timer <= theme.animation_speed / 2.0 ) {
			start_click_time = 0.0;
			end_hover_time = parent_panel.time;
			fade_timer = theme.animation_speed / 2.0;
		}

		fading_animation_timer = theme.animation_speed / 2.0;
	}
	else if ( shapes::hovering( area ) ) {
		time = parent_panel.time;

		if ( start_hover_time == 0.0 )
			start_hover_time = parent_panel.time;

		fade_timer = std::clamp( parent_panel.time - start_hover_time, 0.0, theme.animation_speed / 2.0 );
		end_hover_time = parent_panel.time;
		fading_animation_timer = theme.animation_speed / 2.0;
	}
	else {
		fade_timer = std::clamp( fading_animation_timer - ( parent_panel.time - end_hover_time ), 0.0, fading_animation_timer );
		start_hover_time = 0.0;
	}
}

void oxui::obj::draw_ex( ) {
	draw( );

	/* create new line */
	auto& parent_window = find_parent< window >( object_window );
	parent_window.cursor_pos.y += theme.spacing;
}