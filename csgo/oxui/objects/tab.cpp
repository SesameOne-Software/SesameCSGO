#include "tab.hpp"
#include "../panels/panel.hpp"
#include "shapes.hpp"
#include "../themes/purple.hpp"

void oxui::tab::draw( ) {
	if ( !selected )
		return;

	/* reset draw cursor pos */
	auto& parent_window = find_parent< window >( object_window );
	auto& cursor_pos = parent_window.cursor_pos;
	auto& window_dimensions = parent_window.area;

	/* backup cursor information */
	auto backup_pos = cursor_pos;

	const auto rows = divider.columns_per_row.size( );

	if ( !rows ) {
		/* draw tab objects */
		std::for_each(
			objects.begin( ),
			objects.end( ),
			[ ] ( std::shared_ptr< obj >& child ) {
				child->draw( );
			}
		);

		return;
	}

	/* calculate column width with information */
	auto total_column_padding = theme.spacing * ( rows + 1 );
	auto space_per_column = window_dimensions.w / rows;
	auto calculated_column_width = 0;

	if ( rows > 1 )
		calculated_column_width = space_per_column - theme.spacing - double( theme.spacing ) / double( rows );
	else
		calculated_column_width = space_per_column - theme.spacing * 2.0;

	/* draw all the groups in all the rows and columns, automatically calculate the width and height of each groupbox */
	for ( auto row = 0; row < rows; row++ ) {
		auto selected_group = 0;

		auto previous_group_count = 0;
		auto next_group_count = 0;
		auto item_count_inside_row = 0;

		auto row_counter = 0;

		/* find amount of columns behind the last row */
		for ( auto& column_counter : divider.columns_per_row ) {
			if ( row_counter < row ) {
				previous_group_count += column_counter;
			}

			if ( row_counter <= row ) {
				next_group_count = previous_group_count + column_counter;
				item_count_inside_row = column_counter;
			}

			row_counter++;
		}

		/* calculate column height with previous information */
		auto total_column_padding_h = theme.spacing * ( item_count_inside_row + 1 );
		auto space_per_column_h = window_dimensions.h / item_count_inside_row;
		auto calculated_column_height = 0;

		if ( item_count_inside_row > 1 )
			calculated_column_height = space_per_column_h - theme.spacing - double( theme.spacing ) / double( item_count_inside_row );
		else
			calculated_column_height = space_per_column_h - theme.spacing * 2.0;

		/* draw groups that are in the corresponding columns */
		std::for_each(
			objects.begin( ),
			objects.end( ),
			[ & ] ( std::shared_ptr< obj >& child ) {
				if ( child->type == object_group ) {
					if ( selected_group >= previous_group_count && selected_group < next_group_count ) {
						/* set group dimension parameters accordingly */
						child->area.w = calculated_column_width;
						child->area.h = calculated_column_height;

						auto backup_pos_child = cursor_pos;

						child->draw_ex( );

						cursor_pos.x = backup_pos_child.x;
						cursor_pos.y = backup_pos_child.y;

						/* move down to next group */
						cursor_pos.y += calculated_column_height + theme.spacing;
					}

					selected_group++;
				}
			}
		);

		/* reset cursor y value */
		cursor_pos.y = backup_pos.y;
		/* move to the next row */
		cursor_pos.x += calculated_column_width + theme.spacing;
	}

	/* restore to original location */
	cursor_pos = backup_pos;
}