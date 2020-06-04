#include "tab.hpp"
#include "../panels/panel.hpp"
#include "shapes.hpp"
#include "../themes/purple.hpp"
#include "subtab.hpp"

void oxui::tab::draw( ) {
	if ( !selected )
		return;

	/* reset draw cursor pos */
	auto& parent_window = find_parent< window > ( object_window );
	auto& parent_panel = find_parent< panel >( object_panel );
	auto& cursor_pos = parent_window.cursor_pos;
	auto& window_dimensions = parent_window.area;

	/* backup cursor information */
	auto backup_pos = cursor_pos;

	//const auto rows = divider.columns_per_row.size( );
	//
	//if ( !rows ) {
	//	/* draw tab objects */
	//	std::for_each(
	//		objects.begin( ),
	//		objects.end( ),
	//		[ ] ( std::shared_ptr< obj > child ) {
	//			child->draw( );
	//		}
	//	);
	//
	//	return;
	//}

	auto subtab_count = 0;
	auto subtab_tabs_w = 0;

	/* get group data */
	std::for_each (
		objects.begin ( ),
		objects.end ( ),
		[ & ] ( std::shared_ptr< obj > child ) {
		if ( child->type == object_subtab ) {
			rect bounds;
			binds::text_bounds ( parent_panel.fonts [ OSTR ( "group" ) ], std::dynamic_pointer_cast< subtab >( child )->title, bounds );
			subtab_tabs_w += bounds.w + theme.spacing + 4;
			subtab_count++;
		}
	} );

	const auto subtab_tab_offset_x = window_dimensions.x + window_dimensions.w / 2 - subtab_tabs_w / 2;
	const auto subtab_tab_offset_y = window_dimensions.y + 16;
	auto current_tab_x = 0;

	/* draw subtabs */
	std::for_each (
		objects.begin ( ),
		objects.end ( ),
		[ & ] ( std::shared_ptr< obj > child ) {
		if ( child->type == object_subtab ) {
			auto as_subtab = std::dynamic_pointer_cast< subtab >( child );

			rect bounds;
			binds::text_bounds ( parent_panel.fonts [ OSTR ( "group" ) ], as_subtab->title, bounds );

			rect selectable_bounds { subtab_tab_offset_x + current_tab_x + 2, subtab_tab_offset_y, bounds.w + theme.spacing, 26 };

			if ( shapes::clicking ( selectable_bounds ) ) {
				/* update click timer */
				as_subtab->time = parent_panel.time;

				/* deselect all */
				std::for_each (
					objects.begin ( ),
					objects.end ( ),
					[ & ] ( std::shared_ptr< obj > child ) {
					auto as_subtab = std::dynamic_pointer_cast< subtab >( child );

					if ( child->type == object_subtab ) {
						if ( as_subtab->selected )
							as_subtab->time = parent_panel.time;

						as_subtab->selected = false;
					}
				} );

				/* select clicked tab */
				as_subtab->selected = true;
			}

			if ( as_subtab->selected ) {
				binds::rounded_rect ( selectable_bounds, selectable_bounds.h / 2, 16, { 0, 0, 0, 90 }, false );
				binds::rounded_rect ( selectable_bounds, selectable_bounds.h / 2, 16, { 0, 0, 0, 90 }, true );
			}

			binds::text ( { selectable_bounds.x + theme.spacing / 2, subtab_tab_offset_y + selectable_bounds.h / 2 - bounds.h / 2 }, parent_panel.fonts [ OSTR ( "group" ) ], as_subtab->title, as_subtab->selected ? color ( theme.title_text.r, theme.title_text.g, theme.title_text.b, 150 ) : color ( theme.text.r, theme.text.g, theme.text.b, 150 ), as_subtab->selected );

			as_subtab->draw ( );

			current_tab_x += selectable_bounds.w + 2;
		}
	} );

	/* restore to original location */
	cursor_pos = backup_pos;
}