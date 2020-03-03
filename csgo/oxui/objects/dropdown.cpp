#include "dropdown.hpp"
#include "../panels/panel.hpp"
#include "shapes.hpp"
#include "../themes/purple.hpp"

void oxui::dropdown::think( ) {
	auto& parent_window = find_parent< window >( object_window );
	auto& cursor_pos = parent_window.cursor_pos;
	animate( rect( cursor_pos.x, cursor_pos.y, area.w, area.h ) );

	if ( shapes::clicking( rect( cursor_pos.x, cursor_pos.y, area.w, area.h ), false, opened ) ) {
		g_input = ( opened = !opened );
	}

	/* handle */
	if ( opened ) {
		/* get list end position */
		auto end_pos_y = cursor_pos.y + theme.spacing + theme.list_spacing;

		/* background of the list */
		rect list_pos( cursor_pos.x, end_pos_y, area.w, area.h * items.size( ) );

		hovered_index = 0;

		auto index = 0;

		for ( const auto& it : items ) {
			/* check if we are clicking the thingy */
			if ( shapes::clicking( list_pos, false, true ) ) {
				value = index;

				/* remove if you want to close it manually instead of automatically (snek) */ {
					g_input = true;
					opened = false;
				}
			}
			else if ( shapes::hovering( list_pos, false, true ) ) {
				hovered_index = index;
			}

			list_pos.y += theme.spacing;
			index++;
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
	shapes::box( box_area, fade_timer, true, true, true, true, true, false );

	auto hovering_box = shapes::hovering( box_area, false, true );

	rect text_size;

	if ( hovering_box )
		binds::text_bounds( font, items [ value ], text_size );
	else
		binds::text_bounds( font, label, text_size );

	auto area_center_x = cursor_pos.x + area.w / 2;
	auto area_center_y = cursor_pos.y + theme.spacing / 2;

	/* centered text */
	if ( hovered_index ) {
		rect text_size;
		binds::text_bounds( font, items [ hovered_index ], text_size );
		binds::text( pos( area_center_x - text_size.w / 2 - 1, area_center_y - text_size.h / 2 - 1 ), font, items [ hovered_index ], theme.text, false );
	}
	else {
		binds::text( pos( area_center_x - text_size.w / 2 - 1, area_center_y - text_size.h / 2 - 1 ), font, hovering_box ? items [ value ] : label, theme.text, false );
	}

	/* dropdown arrow! */ {
		auto arrow_max_alpha = fade_timer > theme.animation_speed ? 255 : int( fade_timer * ( 1.0 / theme.animation_speed ) * 255 );
		auto arrow_clr = color( theme.main.r, theme.main.g, theme.main.b, opened ? 255 : arrow_max_alpha );
		auto midpoint_x = cursor_pos.x + area.w - theme.spacing / 2 - arrow_size / 2;

		if ( !opened ) {
			binds::line( pos( midpoint_x - arrow_size / 2, area_center_y - arrow_size / 2 ), pos( midpoint_x, area_center_y + arrow_size / 2 ), arrow_clr );
			binds::line( pos( midpoint_x + arrow_size / 2, area_center_y - arrow_size / 2 ), pos( midpoint_x, area_center_y + arrow_size / 2 ), arrow_clr );
		}
		else {
			binds::line( pos( midpoint_x - arrow_size / 2, area_center_y + arrow_size / 2 ), pos( midpoint_x, area_center_y - arrow_size / 2 ), arrow_clr );
			binds::line( pos( midpoint_x + arrow_size / 2, area_center_y + arrow_size / 2 ), pos( midpoint_x, area_center_y - arrow_size / 2 ), arrow_clr );
		}
	}

	/* draw items */
	if ( opened ) {
		g_input = false;

		parent_window.draw_overlay( [ = ] ( ) {
			/* get list end position */
			auto end_pos_y = cursor_pos.y + theme.spacing + theme.list_spacing;

			/* background of the list */
			shapes::box( rect( cursor_pos.x, end_pos_y, area.w, area.h * items.size( ) ), fade_timer, false, true, true, true, true, true );

			/* render the items name */
			auto index = 0;

			for ( const auto& it : items ) {
				/* get the text size */
				rect item_text_size;
				binds::text_bounds( font, it, item_text_size );

				/* render the name */
				binds::text( pos { area_center_x - item_text_size.w / 2 - 1, end_pos_y + area.h * ( index + 1 ) - area.h / 2 - item_text_size.h / 2 }, font, it, hovered_index == index ? theme.main : theme.text, hovered_index == index );

				index++;
			}
			} );
	}
}