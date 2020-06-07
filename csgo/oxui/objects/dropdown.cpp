#include "dropdown.hpp"
#include "../panels/panel.hpp"
#include "shapes.hpp"
#include "../themes/purple.hpp"

void oxui::dropdown::think( ) {
	auto& parent_window = find_parent< window >( object_window );
	auto& cursor_pos = parent_window.cursor_pos;
	animate( rect( cursor_pos.x, cursor_pos.y, area.w, area.h ) );

	if ( shapes::clicking( rect( cursor_pos.x, cursor_pos.y, area.w, area.h ), false, opened ) ) {
		opened = !opened;
		g_input = !opened;
	}

	/* handle */
	if ( opened ) {
		/* get list end position */
		auto end_pos_y = cursor_pos.y + theme.spacing + theme.list_spacing;

		/* background of the list */
		rect list_pos( cursor_pos.x, end_pos_y, area.w, area.h );

		hovered_index = -1;

		auto index = 0;

		for ( const auto& it : items ) {
			const auto backup_input_clip_area = g_oxui_input_clip_area;

			/* ignore clipping */
			g_oxui_input_clip_area = false;

			/* check if we are clicking the thingy */
			if ( shapes::clicking( list_pos, false, true ) ) {
				value = index;

				/* remove if you want to close it manually instead of automatically (snek) */ {
					shapes::finished_input_frame = true;
					shapes::click_start = pos ( 0, 0 );
					g_input = true;
					opened = false;
				}
			}
			else if ( shapes::hovering( list_pos, false, true ) ) {
				hovered_index = index;
			}

			/* ignore clipping */
			g_oxui_input_clip_area = backup_input_clip_area;

			list_pos.y += theme.spacing;
			index++;
		}

		/* we clicked outside the dropdown list, let's close */
		if ( utils::key_state ( VK_LBUTTON ) ) {
			pos mouse_pos;
			binds::mouse_pos ( mouse_pos );

			const auto hovered_area = mouse_pos.x >= cursor_pos.x && mouse_pos.y >= cursor_pos.y && mouse_pos.x <= cursor_pos.x + area.w && mouse_pos.y <= list_pos.y;

			if ( !hovered_area ) {
				shapes::finished_input_frame = true;
				shapes::click_start = pos ( 0, 0 );
				g_input = true;
				opened = false;
			}
		}
	}
}

void oxui::dropdown::draw( ) {
	/* change if u want a different arrow size! */
	const auto arrow_size = 4;

	think( );

	auto& parent_panel = find_parent< panel >( object_panel );
	auto& parent_window = find_parent< window >( object_window );

	auto& font = parent_panel.fonts [ OSTR( "object" ) ];

	auto& cursor_pos = parent_window.cursor_pos;

	/* button box */
	auto box_area = rect( cursor_pos.x, cursor_pos.y, area.w, area.h );
	
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

	/* highlighted selection */
	binds::rounded_rect ( { cursor_pos.x, cursor_pos.y, area.w, area.h }, 8, 16, opened ? color { 0, 0, 0, static_cast< int >( std::sin ( parent_panel.time * 2.0 * 3.141f ) * 60.0 + 60.0 ) } : color { 0, 0, 0, 90 }, true );

	auto hovering_box = shapes::hovering( box_area, false, true );

	rect text_size;

	if ( hovering_box )
		binds::text_bounds( font, items [ value ], text_size );
	else
		binds::text_bounds( font, label, text_size );

	auto area_center_x = cursor_pos.x + area.w / 2;
	auto area_center_y = cursor_pos.y + theme.spacing / 2;

	/* centered text */
	if ( hovered_index && hovered_index != -1 ) {
		rect text_size;
		binds::text_bounds( font, items [ hovered_index ], text_size );
		binds::text( pos( area_center_x - text_size.w / 2 - 1, area_center_y - text_size.h / 2 - 1 ), font, items [ hovered_index ], lerp_color ( theme.text, { 56, 56, 56, 255 }, lerp_fraction ), false );
	}
	else {
		binds::text( pos( area_center_x - text_size.w / 2 - 1, area_center_y - text_size.h / 2 - 1 ), font, hovering_box ? items [ value ] : label, lerp_color ( theme.text, { 56, 56, 56, 255 }, lerp_fraction ), false );
	}

	/* dropdown arrow! */ {
		auto arrow_max_alpha = fade_timer > theme.animation_speed ? 255 : int( fade_timer * ( 1.0 / theme.animation_speed ) * 255 );
		auto arrow_clr = color( theme.main.r, theme.main.g, theme.main.b, opened ? 255 : arrow_max_alpha );
		auto midpoint_x = cursor_pos.x + area.w - theme.spacing / 2 - arrow_size / 2;

		if ( !opened ) {
			render::polygon ( {
				{ midpoint_x - arrow_size / 2, area_center_y - arrow_size / 2 },
				{ midpoint_x + arrow_size / 2, area_center_y - arrow_size / 2 },
				{ midpoint_x, area_center_y + arrow_size / 2 }
				},
				D3DCOLOR_RGBA ( 255, 255, 255, 255 ),
				false );
		}
		else {
			render::polygon ( {
				{ midpoint_x - arrow_size / 2, area_center_y + arrow_size / 2 },
				{ midpoint_x + arrow_size / 2, area_center_y + arrow_size / 2 },
				{ midpoint_x, area_center_y - arrow_size / 2 }
				},
				D3DCOLOR_RGBA ( 255, 255, 255, 255 ),
				false );
		}
	}

	/* draw items */
	if ( opened ) {
		g_input = false;

		parent_window.draw_overlay( [ = ] ( ) {
			/* get list end position */
			auto end_pos_y = cursor_pos.y + theme.spacing + theme.list_spacing;

			/* render the items name */
			auto index = 0;

			/* background of the list */
			binds::rounded_rect ( { cursor_pos.x, end_pos_y, area.w, area.h + 8 }, 8, 16, hovered_index == index ? theme.bg : theme.container_bg, false );
			binds::rounded_rect ( { cursor_pos.x, end_pos_y + area.h * ( static_cast< int > ( items.size ( ) ) - 1 ) - 8, area.w, area.h + 8 }, 8, 16, hovered_index == ( items.size ( ) - 1 ) ? theme.bg : theme.container_bg, false );

			for ( const auto& it : items ) {
				/* get the text size */
				rect item_text_size;
				binds::text_bounds( font, it, item_text_size );

				/* render the square background if not first or last (middle of rounded rectangles) */
				if ( index && index != items.size ( ) - 1 )
					binds::fill_rect ( { cursor_pos.x, end_pos_y + area.h * index, area.w, area.h }, hovered_index == index ? theme.bg : theme.container_bg );

				/* render the name */
				binds::text( { area_center_x - item_text_size.w / 2 - 1, end_pos_y + area.h * ( index + 1 ) - area.h / 2 - item_text_size.h / 2 }, font, it, theme.text, hovered_index == index );

				index++;
			}

			/* outline of the list */
			binds::rounded_rect ( rect ( cursor_pos.x, end_pos_y, area.w, area.h * items.size ( ) ), 8, 16, { 0, 0, 0, 90 }, true );
		} );
	}
}