#include "groupbox.hpp"
#include "../panels/panel.hpp"
#include "shapes.hpp"
#include "../themes/purple.hpp"

void oxui::group::think ( ) {
	auto& parent_window = find_parent< window > ( object_window );
	animate ( rect ( parent_window.cursor_pos.x, parent_window.cursor_pos.y, area.w, area.h ) );

	/* calculate how much room we have for new objects */
	max_height = 26 + theme.spacing * 2;

	std::for_each (
		objects.begin ( ),
		objects.end ( ),
		[ & ] ( std::shared_ptr< obj >& child ) {
		max_height += child->calc_height ( );
	} );

	if ( area.h > max_height )
		max_height = area.h;

	/* manage scrolling */
	if ( shapes::hovering ( rect ( parent_window.cursor_pos.x, parent_window.cursor_pos.y, area.w, area.h ) ) && parent_window.scroll_delta != 0.0 ) {
		scroll_offset += parent_window.scroll_delta * 18.0;
		scroll_offset = std::clamp< double > ( scroll_offset, 0, static_cast< double >( max_height - area.h ) );
	}
}

void oxui::group::draw ( ) {
	think ( );

	auto& parent_panel = find_parent< panel > ( object_panel );
	auto& parent_window = find_parent< window > ( object_window );

	auto& tfont = parent_panel.fonts [ OSTR ( "group" ) ];
	auto& font = parent_panel.fonts [ OSTR ( "object" ) ];

	/* reset draw cursor pos */
	auto& cursor_pos = parent_window.cursor_pos;

	const auto og_cursor_pos = cursor_pos;
	
	const auto window_margins = theme.spacing;
	const auto window_viewport = rect { parent_window.area.x + window_margins, parent_window.area.y + window_margins * 3, parent_window.area.w - window_margins * 2, parent_window.area.h - window_margins * 4 };

	if ( !hide_title ) {
		rect text_bounds;
		binds::text_bounds ( tfont, title, text_bounds );

		if ( fractions [ 0 ] != 0.0f ) {
			binds::text ( pos ( cursor_pos.x + theme.spacing / 2, cursor_pos.y + 4 ), tfont, title, color ( theme.title_text.r, theme.title_text.g, theme.title_text.b, 150 ), false );

			if ( extend_separator )
				binds::fill_rect ( { cursor_pos.x + text_bounds.w + theme.spacing, cursor_pos.y + 13, window_viewport.w - text_bounds.w - theme.spacing, 2 }, color ( theme.text.r, theme.text.g, theme.text.b, 45 ) );
			else
				binds::fill_rect ( { cursor_pos.x + text_bounds.w + theme.spacing, cursor_pos.y + 13, area.w - text_bounds.w - theme.spacing, 2 }, color ( theme.text.r, theme.text.g, theme.text.b, 45 ) );
		}
		else {
			binds::text ( pos ( cursor_pos.x, cursor_pos.y + 4 ), tfont, title, color ( theme.title_text.r, theme.title_text.g, theme.title_text.b, 150 ), false );

			if ( extend_separator )
				binds::fill_rect ( { cursor_pos.x + text_bounds.w + theme.spacing / 2, cursor_pos.y + 13, window_viewport.w - text_bounds.w - theme.spacing / 2, 2 }, color ( theme.text.r, theme.text.g, theme.text.b, 45 ) );
			else
				binds::fill_rect ( { cursor_pos.x + text_bounds.w + theme.spacing / 2, cursor_pos.y + 13, area.w - text_bounds.w - theme.spacing / 2, 2 }, color ( theme.text.r, theme.text.g, theme.text.b, 45 ) );
		}
	}

	if ( fractions [ 0 ] != 0.0f ) {
		binds::fill_rect ( { cursor_pos.x, cursor_pos.y + 26 + theme.spacing / 2, 2, area.h - theme.spacing - 26 }, color ( theme.text.r, theme.text.g, theme.text.b, 45 ) );
	}

	/* move all objects inside group */
	cursor_pos.x += theme.spacing;
	cursor_pos.y += theme.spacing * 2;

	/* draw group objects */
	binds::clip ( rect ( og_cursor_pos.x, og_cursor_pos.y + 26 + 1, area.w, area.h - 26 - 1 ), [ & ] ( ) {
		cursor_pos.y -= scroll_offset;

		std::for_each (
			objects.begin ( ),
			objects.end ( ),
			[ & ] ( std::shared_ptr< obj >& child ) {
			child->area = rect ( 0, 0, area.w - theme.spacing * 2, theme.spacing );

			child->draw_ex ( );
		}
		);
	} );

	/* too many items; we need to be able to scroll down on this groupbox */
	if ( max_height - area.h > 0 ) {
		/* let's draw a gradient next to the top to let people know that there are more items on the way on the top */
		if ( scroll_offset > 0 ) {
			//binds::gradient_rect ( rect ( og_cursor_pos.x + 1, og_cursor_pos.y + 26 + 1, area.w - 2, theme.spacing / 2 ), color ( 0, 0, 0, 100 ), color ( 0, 0, 0, 0 ), false );
			render::polygon ( { { og_cursor_pos.x + area.w / 2, og_cursor_pos.y + 22 }, {og_cursor_pos.x + area.w / 2 - 3, og_cursor_pos.y + 22 + 3}, {og_cursor_pos.x + area.w / 2 + 3, og_cursor_pos.y + 22 + 3} }, D3DCOLOR_RGBA( theme.title_text.r, theme.title_text.g, theme.title_text.b, theme.title_text.a ), false );
		}

		/* let's draw a gradient next to the bottom to let people know that there are more items on the way on the bottom */
		if ( scroll_offset < static_cast< double >( max_height - area.h ) ) {
			//binds::gradient_rect ( rect ( og_cursor_pos.x + 1, og_cursor_pos.y + area.h - theme.spacing / 2 - 1, area.w - 2, theme.spacing / 2 ), color ( 0, 0, 0, 0 ), color ( 0, 0, 0, 100 ), false );
			render::polygon ( { { og_cursor_pos.x + area.w / 2 - 3, og_cursor_pos.y + area.h - theme.spacing / 2 - 1 + 10 }, {og_cursor_pos.x + area.w / 2, og_cursor_pos.y + area.h - theme.spacing / 2 - 1 + 3 + 10}, {og_cursor_pos.x + area.w / 2 + 3, og_cursor_pos.y + area.h - theme.spacing / 2 - 1 + 10} }, D3DCOLOR_RGBA ( theme.title_text.r, theme.title_text.g, theme.title_text.b, theme.title_text.a ), false );
		}
	}
}