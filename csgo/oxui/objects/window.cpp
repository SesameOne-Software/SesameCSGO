#include "window.hpp"
#include "../panels/panel.hpp"
#include "shapes.hpp"
#include "../themes/purple.hpp"
#include "tab.hpp"
#include "../json/json.hpp"
#include <string>
#include <fstream>
#include "checkbox.hpp"
#include "slider.hpp"

void* oxui::window::find_obj( const str& tab_name, const str& group_name, const str& object_name, object_type type ) {
	for ( auto& _tab : objects ) {
		auto tab = std::static_pointer_cast< oxui::tab >( _tab );

		if ( tab->title == tab_name ) {
			for ( auto& _group : tab->objects ) {
				auto group = std::static_pointer_cast< oxui::group >( _group );

				if ( group->title == group_name ) {
					for ( auto& _control : group->objects ) {
						switch ( _control->type ) {
						case object_checkbox: {
							auto as_checkbox = std::static_pointer_cast< oxui::checkbox >( _control );
							if ( as_checkbox->label == object_name )
								return &as_checkbox->checked;
							break;
						}
						case object_slider: {
							auto as_slider = std::static_pointer_cast< oxui::slider >( _control );
							if ( as_slider->label == object_name )
								return &as_slider->value;
							break;
						}
						}
					}
				}
			}
		}
	}

	return nullptr;
}

void oxui::window::save_state( const str& file ) {
	nlohmann::json json;

	auto window_str = std::string( title.begin( ), title.end( ) );

	std::for_each( objects.begin( ), objects.end( ), [ & ] ( std::shared_ptr< obj >& tab_obj ) {
		auto tab = std::static_pointer_cast< oxui::tab >( tab_obj );
		auto tab_str = std::string( tab->title.begin( ), tab->title.end( ) );

		std::for_each( tab->objects.begin( ), tab->objects.end( ), [ & ] ( std::shared_ptr< obj >& group_obj ) {
			auto group = std::static_pointer_cast< oxui::group >( group_obj );
			auto group_str = std::string( group->title.begin( ), group->title.end( ) );

			std::for_each( group->objects.begin( ), group->objects.end( ), [ & ] ( std::shared_ptr< obj >& object ) {
				switch ( object->type ) {
				case object_checkbox: {
					auto as_checkbox = ( ( std::shared_ptr< checkbox >& ) object );
					auto obj_str = std::string( as_checkbox->label.begin( ), as_checkbox->label.end( ) );
					json [ window_str ][ tab_str ][ group_str ][ obj_str ] = as_checkbox->checked;
					break;
				}
				case object_slider: {
					auto as_slider = ( ( std::shared_ptr< slider >& ) object );
					auto obj_str = std::string( as_slider->label.begin( ), as_slider->label.end( ) );
					json [ window_str ][ tab_str ][ group_str ][ obj_str ] = as_slider->value;
					break;
				}
				}
			} );
		} );
	} );

	const auto dump = json.dump( 4 );

	std::ofstream ofile( file );
	ofile.write( dump.data( ), dump.length( ) );
	ofile.close( );
}

void oxui::window::load_state( const str& file ) {
	std::string dump;
	std::ifstream ofile( file );

	if ( !ofile.is_open( ) )
		return;

	ofile.seekg( 0, std::ios::end );
	const auto fsize = ofile.tellg( );
	ofile.seekg( 0, std::ios::beg );
	char* str = new char [ fsize ];
	ofile.read( str, fsize );
	dump = str;
	delete [ ] str;
	ofile.close( );

	nlohmann::json json = nlohmann::json::parse( dump );

	auto window_str = std::string( title.begin( ), title.end( ) );

	/* window doesn't exist */
	if ( !json.contains( window_str ) )
		return;

	std::for_each( objects.begin( ), objects.end( ), [ & ] ( std::shared_ptr< obj > tab_obj ) {
		auto tab = std::static_pointer_cast< oxui::tab >( tab_obj );
		auto tab_str = std::string( tab->title.begin( ), tab->title.end( ) );

		/* tab doesn't exist */
		if ( !json [ window_str ].contains( tab_str ) )
			return;

		std::for_each( tab->objects.begin( ), tab->objects.end( ), [ & ] ( std::shared_ptr< obj > group_obj ) {
			auto group = std::static_pointer_cast< oxui::group >( group_obj );
			auto group_str = std::string( group->title.begin( ), group->title.end( ) );

			/* group doesn't exist */
			if ( !json [ window_str ][ tab_str ].contains( group_str ) )
				return;

			std::for_each( group->objects.begin( ), group->objects.end( ), [ & ] ( std::shared_ptr< obj >& object ) {
				switch ( object->type ) {
				case object_checkbox: {
					auto as_checkbox = ( checkbox* ) object.get( );
					auto obj_str = std::string( as_checkbox->label.begin( ), as_checkbox->label.end( ) );

					/* control doesn't exist */
					if ( !json [ window_str ][ tab_str ][ group_str ].contains( obj_str ) )
						return;

					as_checkbox->checked = json [ window_str ][ tab_str ][ group_str ][ obj_str ].get< bool >( );
					break;
				}
				case object_slider: {
					auto as_slider = ( slider* ) object.get( );
					auto obj_str = std::string( as_slider->label.begin( ), as_slider->label.end( ) );

					/* control doesn't exist */
					if ( !json [ window_str ][ tab_str ][ group_str ].contains( obj_str ) )
						return;

					as_slider->value = json [ window_str ][ tab_str ][ group_str ][ obj_str ].get< double >( );
					break;
				}
				}
			} );
		} );
	} );
}

void oxui::window::think( ) {
	if ( !pressing_move_key && GetAsyncKeyState( VK_LBUTTON ) && shapes::hovering( rect( area.x, area.y - 26, area.w, 26 ) ) ) {
		pressing_move_key = true;
		pos mouse_mpos;
		binds::mouse_pos( mouse_mpos );
		click_offset = pos( mouse_mpos.x - area.x, mouse_mpos.y - area.y );
	}
	else if ( pressing_move_key && GetAsyncKeyState( VK_LBUTTON ) ) {
		pos mouse_mpos;
		binds::mouse_pos( mouse_mpos );

		area.x = mouse_mpos.x - click_offset.x;
		area.y = mouse_mpos.y - click_offset.y;
	}
	else {
		pressing_move_key = false;
	}
}

void oxui::window::draw( ) {
	if ( toggle_bind ) {
		if ( !pressing_open_key && GetAsyncKeyState( toggle_bind ) ) {
			pressing_open_key = true;
		}
		else if ( pressing_open_key && !GetAsyncKeyState( toggle_bind ) ) {
			open = !open;
			pressing_open_key = false;
		}
	}

	if ( !open )
		return;

	auto& parent_panel = find_parent< panel >( object_panel );

	/* menu physics */
	think( );

	cursor_pos = pos( area.x + theme.spacing, area.y + theme.spacing );

	/* draw window rect */
	binds::fill_rect( area, theme.bg );

	/* title bar */
	shapes::box( rect( area.x, area.y - 26, area.w, 26 ), 0.0, false, false, true, false, false );
	binds::text( pos( area.x + 4, area.y - 28 + 6 ), parent_panel.fonts [ OSTR( "title") ], title, theme.text, true );

	std::vector< std::pair< std::shared_ptr< tab >, int > > tab_list;
	auto total_tabs_w = 0;
	std::for_each( objects.begin( ), objects.end( ), [ & ] ( std::shared_ptr< obj > object ) {
		if ( object->type == object_tab ) {
			auto as_tab = std::static_pointer_cast< tab >( object );

			rect bounds;
			binds::text_bounds( parent_panel.fonts [ OSTR( "object") ], as_tab->title, bounds );
			total_tabs_w += bounds.w + 4;

			tab_list.push_back( std::pair< std::shared_ptr< tab >, int >( as_tab, bounds.w ) );
		}
		} );
	auto last_tab_pos = pos( area.x + area.w - total_tabs_w - 6, area.y - 28 + 6 );
	std::for_each( tab_list.begin( ), tab_list.end( ), [ & ] ( std::pair< std::shared_ptr< tab >, int > object ) {
		if ( shapes::clicking( rect( last_tab_pos.x, last_tab_pos.y, object.second, 16 ) ) ) {
			/* update click timer */
			object.first->time = parent_panel.time;

			/* deselect all */
			std::for_each(
				tab_list.begin( ),
				tab_list.end( ),
				[ & ] ( std::pair< std::shared_ptr< tab >, int > child ) {
					/* re-run animation */
					if ( child.first->selected )
						child.first->time = parent_panel.time;

					child.first->selected = false;
				}
			);

			/* select clicked tab */
			object.first->selected = true;
		}

		auto time_since_click = std::clamp( parent_panel.time - object.first->time, 0.0, theme.animation_speed );
		auto bar_width = object.second - object.second * ( time_since_click * ( 1.0 / theme.animation_speed ) );
		auto text_height = 0;
		auto alpha = 255 - int( time_since_click * ( 1.0 / theme.animation_speed ) * 80.0 );

		if ( object.first->selected ) {
			alpha = 175 + int( time_since_click * ( 1.0 / theme.animation_speed ) * 80.0 );
			bar_width = object.second * ( time_since_click * ( 1.0 / theme.animation_speed ) );
			text_height = 2 * ( time_since_click * ( 1.0 / theme.animation_speed ) );
		}

		binds::text( pos( last_tab_pos.x, area.y - 28 + 6 - text_height ), parent_panel.fonts [ OSTR( "object") ], object.first->title, color( theme.text.r, theme.text .g, theme.text .b, alpha ), false );
		binds::line( pos( last_tab_pos.x + object.second / 2 - bar_width / 2, area.y - 28 + 6 + 16 ), pos( last_tab_pos.x + object.second / 2 + bar_width / 2, area.y - 28 + 6 + 16 ), theme.main );

		last_tab_pos.x += object.second + 4;
		} );

	/* draw window objects */
	binds::clip( area, [ & ] ( ) {
		std::for_each(
			objects.begin( ),
			objects.end( ),
			[ ] ( std::shared_ptr< obj >& child ) {
				child->draw( );
			}
		);
	} );
}
