#include "subtab.hpp"
#include "../panels/panel.hpp"
#include "shapes.hpp"
#include "../themes/purple.hpp"

void oxui::subtab::think( ) {
	
}

void oxui::subtab::draw( ) {
	if ( !selected )
		return;

	think( );

	auto& parent_window = find_parent< window > ( object_window );
	auto& cursor_pos = parent_window.cursor_pos;
	auto& window_dimensions = parent_window.area;

	/* backup cursor information */
	auto backup_pos = cursor_pos;

	const auto window_margins = theme.spacing;
	const auto window_viewport = rect { window_dimensions.x + window_margins, window_dimensions.y + window_margins * 3, window_dimensions.w - window_margins * 2, window_dimensions.h - window_margins * 4 };

	/* draw groups that are in the corresponding columns */
	std::for_each (
		objects.begin ( ),
		objects.end ( ),
		[ & ] ( std::shared_ptr< obj >& child ) {
		if ( child->type == object_group ) {
			auto as_group = std::dynamic_pointer_cast< group >( child );

			cursor_pos.x = window_viewport.x + window_viewport.w * as_group->fractions [ 0 ];
			cursor_pos.y = window_viewport.y + window_viewport.h * as_group->fractions [ 1 ];

			child->area.w = window_viewport.w * as_group->fractions [ 2 ];
			child->area.h = window_viewport.h * as_group->fractions [ 3 ];

			child->draw ( );
		}
	} );

	/* restore to original location */
	cursor_pos = backup_pos;
}