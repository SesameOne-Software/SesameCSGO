#include "button.hpp"
#include "../panels/panel.hpp"
#include "shapes.hpp"
#include "../themes/purple.hpp"

void oxui::button::think( ) {
	auto& parent_window = find_parent< window >( object_window );
	auto& cursor_pos = parent_window.cursor_pos;
	animate( rect( cursor_pos.x, cursor_pos.y, area.w, area.h ) );

	if ( shapes::clicking( rect( cursor_pos.x, cursor_pos.y, area.w, area.h ) ) )
		callback( );
}

void oxui::button::draw( ) {
	think( );

	auto& parent_panel = find_parent< panel >( object_panel );
	auto& parent_window = find_parent< window >( object_window );

	auto& font = parent_panel.fonts [ OSTR( "object" ) ];

	auto& cursor_pos = parent_window.cursor_pos;

	const auto fade_alpha = fade_timer > theme.animation_speed ? 90 : int ( fade_timer * ( 1.0 / theme.animation_speed ) * 90.0 );
	const auto lerp_fraction = ( fade_timer * 2.0 ) > theme.animation_speed ? 1.0 : ( ( fade_timer * 2.0 ) * ( 1.0 / theme.animation_speed ) * 1.0 );

	auto lerp_color = [ ] ( const color& clr_from, const color& clr_to, double lerp_fraction ) -> color {
		return {
			static_cast< int > ( ( clr_to.r - clr_from.r ) * lerp_fraction + clr_from.r ),
			static_cast< int > ( ( clr_to.g - clr_from.g ) * lerp_fraction + clr_from.g ),
			static_cast< int > ( ( clr_to.b - clr_from.b ) * lerp_fraction + clr_from.b ),
			static_cast< int > ( ( clr_to.a - clr_from.a ) * lerp_fraction + clr_from.a )
		};
	};

	binds::rounded_rect ( { cursor_pos.x, cursor_pos.y, area.w, area.h }, 8, 16, { 0, 0, 0, 90 }, false );
	
	if ( fade_alpha )
		binds::rounded_rect ( { cursor_pos.x, cursor_pos.y, area.w, area.h }, 8, 16, { 0, 0, 0, fade_alpha }, true );
	else
		binds::rounded_rect ( { cursor_pos.x, cursor_pos.y, area.w, area.h }, 8, 16, { 0, 0, 0, 90 }, true );
	
	rect text_size;
	binds::text_bounds( font, label, text_size );

	auto area_center_x = cursor_pos.x + area.w / 2;
	auto area_center_y = cursor_pos.y + theme.spacing / 2;
	auto check_dimensions = rect( 0, 0, 10, 6 );

	/* centered text */
	binds::text( pos( area_center_x - text_size.w / 2 - 1, area_center_y - text_size.h / 2 - 1 ), font, label, theme.text, false );
}