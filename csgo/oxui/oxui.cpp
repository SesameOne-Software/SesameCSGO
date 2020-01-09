#include "oxui.h"
#include "../sdk/sdk.h"

/* gui styles */
std::map< std::string_view, oxui::color > oxui::style::colors = {
	{ "tab_area", oxui::color{ 38, 38, 38, 255 } },
	{ "window_frame", oxui::color{ 133, 16, 216, 80 } },
	{ "window_title", oxui::color{ 255, 255, 255, 255 } },
	{ "window_border", oxui::color{ 54, 54, 54, 255 } },
	{ "control", oxui::color{ 44, 44, 44, 255 } },
	{ "window", oxui::color{ 38, 38, 38, 255 } },
	{ "border", oxui::color{ 122, 122, 122, 200 } },
	{ "tab", oxui::color{ 133, 16, 216, 255 } },
	{ "tab_1", oxui::color{ 144, 49, 212, 255 } },
	{ "selection", oxui::color { 133, 16, 216, 255 } },
	{ "button", oxui::color { 30, 30, 30, 255 } },
	{ "button_selected", oxui::color { 50, 50, 50, 255 } },
	{ "selected_item", oxui::color { 133, 16, 216, 255 } },
	{ "category", oxui::color { 100, 100, 100, 255 } },
	{ "scrollbar_track", oxui::color { 117, 117, 117, 255 } },
	{ "scrollbar_handle", oxui::color { 92, 92, 92, 255 } },
	{ "text", oxui::color { 210, 210, 210, 255 } }
};

/* gui variables */
float oxui::vars::scroll_amount = 0.0f;
bool oxui::vars::opening_dropdown = false;
oxui::point oxui::vars::dropdown_point = oxui::point( 0, 0 );
bool oxui::vars::disable_menu_input = false;
int* oxui::vars::selected_dropdown_item_ptr = nullptr;
std::vector< std::string_view > oxui::vars::selected_dropdown_items { };
std::map< std::string_view, oxui::vars::option > oxui::vars::items { };
oxui::dimension oxui::vars::last_categories_dim;
int oxui::vars::iter_tab_num = 0;
int oxui::vars::cur_tab_num = 1;
int oxui::vars::tab = 0;
int oxui::vars::clicked_tab = 0;
float oxui::vars::tab_anim_start_time = 0.0f;
float oxui::vars::anim_time = 0.0f;
int oxui::vars::tab_count = 0;
oxui::point oxui::vars::click_start = oxui::point( 0, 0 );
oxui::point oxui::vars::window_start = oxui::point( 0, 0 );
oxui::rect oxui::vars::tab_window;
oxui::rect oxui::vars::selected_tab_area;
oxui::point oxui::vars::next_tab_pos;
oxui::point oxui::vars::next_item_pos;
oxui::point oxui::vars::click_delta = oxui::point( 0, 0 );
oxui::dimension oxui::vars::last_menu_dim;
oxui::point oxui::vars::pos = oxui::point( 0, 0 );
bool oxui::vars::open_pressed = false;
bool oxui::vars::mouse1_pressed = false;
bool oxui::vars::clicked = false;
bool oxui::vars::open = false;
std::map< std::string_view, void* > oxui::vars::fonts = { };

/* gui implementation functions */
oxui::impl::create_font_func		oxui::impl::create_font;
oxui::impl::destroy_font_func		oxui::impl::destroy_font;
oxui::impl::draw_text_func			oxui::impl::draw_text;
oxui::impl::draw_rect_func			oxui::impl::draw_rect;
oxui::impl::draw_filled_rect_func	oxui::impl::draw_filled_rect;
oxui::impl::text_scale_func			oxui::impl::text_scale;
oxui::impl::key_pressed_func		oxui::impl::key_pressed;
oxui::impl::curser_pos_func			oxui::impl::cursor_pos;
oxui::impl::clip_func				oxui::impl::clip;
oxui::impl::draw_circle_func		oxui::impl::draw_circle;
oxui::impl::remove_clip_func		oxui::impl::remove_clip;
oxui::impl::draw_line_func			oxui::impl::draw_line;

/* set a new cursor position for framework */
void oxui::set_cursor_pos( point new_pos ) {
	vars::next_item_pos = new_pos;
}

/* detects if mouse is hovering rectangle */
bool oxui::hovering( rect rect ) {
	const auto cursor_pos = impl::cursor_pos( );
	return !vars::disable_menu_input && cursor_pos.x >= rect.x && cursor_pos.y >= rect.y && cursor_pos.x <= rect.x + rect.w && cursor_pos.y <= rect.y + rect.h;
}

/* detects if started clicking in rectangle */
bool oxui::clicking( rect rect ) {
	return !vars::disable_menu_input && ( vars::click_start.x != 0 || vars::click_start.y != 0 ) && vars::click_start.x >= rect.x && vars::click_start.y >= rect.y && vars::click_start.x <= rect.x + rect.w && vars::click_start.y <= rect.y + rect.h;
}

/* destory all windows and objects */
void oxui::destroy( ) {
	/* destroy all fonts */
	for ( const auto& font : vars::fonts )
		impl::destroy_font( font.second );

	vars::fonts.clear( );
}

/* update animation times */
void oxui::update_anims( float time ) {
	vars::anim_time = time;
}

/* begin drawing a window */
bool oxui::begin( const dimension area, const std::string_view& title, const window_flags flags ) {
	if ( impl::key_pressed( keys::insert ) && !vars::open_pressed ) {
		vars::open_pressed = true;
	}
	else if ( !impl::key_pressed( keys::insert ) && vars::open_pressed ) {
		vars::open = !vars::open;
		vars::open_pressed = false;
	}

	/* check if menu is open before doing anything */
	if ( !vars::open )
		return false;

	/* updating menu */
	vars::last_menu_dim = area;

	/* menu dragging logic */
	if ( impl::key_pressed( keys::mouse1 )
		&& vars::click_delta.x != 0
		&& vars::click_delta.y != 0
		&& vars::window_start.x != 0
		&& vars::window_start.y != 0
		&& ( !vars::disable_menu_input && vars::click_start.x >= vars::window_start.x && vars::click_start.y >= vars::window_start.y - 30 && vars::click_start.x <= vars::window_start.x + area.w && vars::click_start.y <= vars::window_start.y ) ) {
		vars::pos = point(
			impl::cursor_pos( ).x - vars::click_delta.x,
			impl::cursor_pos( ).y - vars::click_delta.y
		);
	}

	/* replace with panel size */
	int w, h;
	csgo::i::engine->get_screen_size( w, h );
	
	if ( !vars::pos.x && !vars::pos.y )
		vars::pos = point( w / 2 - area.w / 2, h / 2 - area.h / 2 );

	const auto title_scale = impl::text_scale( vars::fonts [ "window" ], title );
	const auto title_w = title_scale.w;
	const auto title_h = title_scale.h;

	impl::draw_text( point( vars::pos.x + area.w / 2 - title_w / 2, vars::pos.y - 12 - title_h / 2 ), oxui::color { 255, 255, 255, 255 }, vars::fonts [ "window" ], title, true );

	/* renerding menu panel */
	impl::draw_filled_rect( rect( vars::pos.x - 3, vars::pos.y - 30, area.w + 6, area.h + 30 + 3 ), style::colors [ "window_frame" ] );
	impl::draw_filled_rect( rect( vars::pos.x, vars::pos.y, area.w, area.h ), style::colors [ "window" ] );

	/* renerding menu outline */
	if ( !( int( flags ) & int( window_flags::no_border ) ) ) {
		impl::draw_rect( rect( vars::pos.x - 3, vars::pos.y - 30, area.w + 6, area.h + 30 + 3 ), style::colors [ "window_border" ] );
		impl::draw_rect( rect( vars::pos.x, vars::pos.y, area.w, area.h ), style::colors [ "window_border" ] );
	}

	vars::next_item_pos = point( vars::pos.x + 20, vars::pos.y + 40 );
	vars::clicked = false;

	if ( impl::key_pressed( keys::mouse1 ) && !vars::mouse1_pressed ) {
		vars::mouse1_pressed = true;
		vars::click_start = impl::cursor_pos( );
		vars::click_delta = point( vars::click_start.x - vars::pos.x, vars::click_start.y - vars::pos.y );
		vars::window_start = vars::pos;
	}
	else if ( !impl::key_pressed( keys::mouse1 ) && vars::mouse1_pressed ) {
		vars::clicked = true;
		vars::click_start = point( 0, 0 );
		vars::mouse1_pressed = false;
		vars::click_delta = point( 0, 0 );
		vars::window_start = point( 0, 0 );
	}

	return true;
}

bool oxui::begin_tabs( int tab_count ) {
	vars::tab_count = tab_count;
	vars::next_tab_pos = vars::pos;
	vars::iter_tab_num = 0;

	return true;
}

bool oxui::tab( const std::string_view& title, bool selected, color color ) {
	if ( vars::tab != 0 )
		return false;

	vars::iter_tab_num++;
	
	const auto w = vars::last_menu_dim.w / vars::tab_count;
	const auto h = 18;
	const auto text_sz = impl::text_scale( vars::fonts[ "tab" ], title );
	const auto text_w = text_sz.w;
	const auto text_h = text_sz.h;

	/* if we click the tab */
	if ( hovering( rect( vars::next_tab_pos.x, vars::next_tab_pos.y, w, h ) ) && vars::clicked ) {
		vars::clicked_tab = true;
		vars::cur_tab_num = vars::iter_tab_num;
		vars::tab_anim_start_time = vars::anim_time;
	}

	/* if current tab is one that was clicked */
	if ( vars::cur_tab_num == vars::iter_tab_num ) {
		vars::selected_tab_area = rect( vars::next_tab_pos.x, vars::next_tab_pos.y - h, h, h );
	}
	else {
		impl::draw_line( point( vars::next_tab_pos.x, vars::next_tab_pos.y + h ), point( vars::next_tab_pos.x + w, vars::next_tab_pos.y + h ), style::colors [ "category" ] );
		impl::draw_line( point( vars::next_tab_pos.x, vars::next_tab_pos.y + h + 1 ), point( vars::next_tab_pos.x + w, vars::next_tab_pos.y + h + 1 ), style::colors [ "category" ] );
	}

	impl::draw_line( point( vars::next_tab_pos.x, vars::next_tab_pos.y ), point( vars::next_tab_pos.x + w, vars::next_tab_pos.y ), style::colors [ "category" ] );
	impl::draw_line( point( vars::next_tab_pos.x, vars::next_tab_pos.y ), point( vars::next_tab_pos.x, vars::next_tab_pos.y + h ), style::colors [ "category" ] );
	impl::draw_line( point( vars::next_tab_pos.x + w, vars::next_tab_pos.y ), point( vars::next_tab_pos.x + w, vars::next_tab_pos.y + h ), style::colors [ "category" ] );

	impl::draw_filled_rect( rect( vars::next_tab_pos.x + 1, vars::next_tab_pos.y + 1, w - 1, h / 2 - 2 + 2 ), style::colors [ "tab" ] );
	impl::draw_filled_rect( rect( vars::next_tab_pos.x + 1, vars::next_tab_pos.y + 1 + h / 2, w - 1, h / 2 - 2 + 1 ), style::colors [ "tab_1" ] );

	impl::draw_text( point( vars::next_tab_pos.x + w / 2 - text_w / 2, vars::next_tab_pos.y + h / 2 - text_h / 2 ), style::colors [ "text" ], vars::fonts [ "window" ], title, true );

	vars::next_tab_pos.x += w;

	return false;
}

void oxui::end_tabs( ) {
	impl::draw_line( point( vars::next_tab_pos.x, vars::tab_window.y ), point( vars::tab_window.x + vars::tab_window.w, vars::tab_window.y ), style::colors [ "category" ] );
	impl::draw_line( point( vars::tab_window.x, vars::tab_window.y ), point( vars::tab_window.x, vars::tab_window.y + vars::tab_window.h ), style::colors [ "category" ] );
	impl::draw_line( point( vars::tab_window.x + vars::tab_window.w, vars::tab_window.y ), point( vars::tab_window.x + vars::tab_window.w, vars::tab_window.y + vars::tab_window.h ), style::colors [ "category" ] );
	impl::draw_line( point( vars::tab_window.x, vars::tab_window.y + vars::tab_window.h ), point( vars::tab_window.x + vars::tab_window.w, vars::tab_window.y + vars::tab_window.h ), style::colors [ "category" ] );
}

bool oxui::begin_category( const std::string_view& title, dimension dimensions ) {
	vars::last_categories_dim = dimensions;

	const auto text_w = impl::text_scale( vars::fonts [ "window" ], title ).w;
	const auto text_h = impl::text_scale( vars::fonts [ "window" ], title ).h;

	/* clip all items outside of category box */
	/* TODO: add scrollbar feature */
	impl::draw_text( point( vars::next_item_pos.x + 9, vars::next_item_pos.y - text_h / 2 ), style::colors["text"], vars::fonts [ "window" ], title, true );
	
	impl::draw_line( point( vars::next_item_pos.x, vars::next_item_pos.y ), point( vars::next_item_pos.x + 6, vars::next_item_pos.y ), style::colors [ "category" ] );
	impl::draw_line( point( vars::next_item_pos.x + 6 + text_w + 3, vars::next_item_pos.y ), point( vars::next_item_pos.x + dimensions.w, vars::next_item_pos.y ), style::colors [ "category" ] );
	impl::draw_line( point( vars::next_item_pos.x, vars::next_item_pos.y ), point( vars::next_item_pos.x, vars::next_item_pos.y + dimensions.h ), style::colors [ "category" ] );
	impl::draw_line( point( vars::next_item_pos.x, vars::next_item_pos.y + dimensions.h ), point( vars::next_item_pos.x + dimensions.w, vars::next_item_pos.y + dimensions.h ), style::colors [ "category" ] );
	impl::draw_line( point( vars::next_item_pos.x + dimensions.w, vars::next_item_pos.y + dimensions.h ), point( vars::next_item_pos.x + dimensions.w, vars::next_item_pos.y ), style::colors [ "category" ] );

	impl::clip( rect( vars::next_item_pos.x, vars::next_item_pos.y, vars::last_categories_dim.w, vars::last_categories_dim.h ) );

	vars::next_item_pos.x += style::obj_offset_dist;
	vars::next_item_pos.y += text_h + 12;

	return true;
}

void oxui::end_category( ) {
	vars::last_categories_dim = oxui::dimension( 0, 0 );
	impl::remove_clip( );
}

bool oxui::start_control( const std::string_view& id ) {
	/* create option with default values if it doesnt exist already */
	if ( !vars::items.contains( id ) )
		vars::items [ id ] = vars::option( );

	return true;
}

void oxui::end_control( ) {
	vars::next_item_pos.y += style::obj_offset_dist;
}

bool oxui::button( const std::string_view& title, bool extra_space ) {
	const auto button_w = 125;
	const auto button_h = 20;

	const auto text_scale = impl::text_scale( vars::fonts [ "control" ], title );
	const auto text_w = text_scale.w;
	const auto text_h = text_scale.h;

	if ( hovering( rect( vars::next_item_pos.x, vars::next_item_pos.y, button_w, button_h ) ) ) {
		impl::draw_filled_rect( rect( vars::next_item_pos.x, vars::next_item_pos.y, button_w, button_h ), style::colors [ "button_selected" ] );

		/* button border */
		impl::draw_rect( rect( vars::next_item_pos.x, vars::next_item_pos.y, button_w, button_h ), style::colors [ "selection" ] );

		/* button title */
		impl::draw_text( point( vars::next_item_pos.x + button_w / 2 - text_w / 2, vars::next_item_pos.y + 20 - text_h - 2 ), style::colors [ "text" ], vars::fonts [ "control" ], title, true );

		if ( vars::clicked ) {
			vars::next_item_pos.y += button_h;
			end_control( );
			return true;
		}
	}
	else {
		impl::draw_filled_rect( rect( vars::next_item_pos.x, vars::next_item_pos.y, button_w, button_h ), style::colors [ "button" ] );

		/* button border */
		impl::draw_rect( rect( vars::next_item_pos.x, vars::next_item_pos.y, button_w, button_h ), style::colors [ "border" ] );

		/* button title */
		impl::draw_text( point( vars::next_item_pos.x + button_w / 2 - text_w / 2, vars::next_item_pos.y + 20 - text_h - 2 ), style::colors [ "text" ], oxui::vars::fonts [ "control" ], title, true );
	}

	vars::next_item_pos.y += button_h;
	end_control( );

	return false;
}

void oxui::checkbox( const std::string_view& title, const std::string_view& id ) {
	if ( start_control( id ) ) {
		const auto check_scale = 13;

		const auto text_scale = impl::text_scale( vars::fonts [ "control" ], title );
		const auto text_w = text_scale.w;
		const auto text_h = text_scale.h;

		/* checking logic */
		if ( hovering( rect( vars::next_item_pos.x, vars::next_item_pos.y, check_scale + text_w, check_scale ) ) && vars::clicked )
			vars::items [ id ].val.b = !vars::items [ id ].val.b;

		/* inside */
		impl::draw_filled_rect( rect( vars::next_item_pos.x + 1, vars::next_item_pos.y + 1, check_scale - 1, check_scale - 1), style::colors [ "control" ] );

		/* outline */
		if ( hovering( rect( vars::next_item_pos.x, vars::next_item_pos.y, check_scale + text_w, check_scale ) ) ) {
			impl::draw_rect( rect( vars::next_item_pos.x, vars::next_item_pos.y, check_scale, check_scale ), style::colors [ "selection" ] );
		}
		else {
			impl::draw_rect( rect( vars::next_item_pos.x, vars::next_item_pos.y, check_scale, check_scale ), style::colors [ "border" ] );
		}

		/* checkbox title */
		impl::draw_text( point( vars::next_item_pos.x + check_scale + 4, vars::next_item_pos.y + check_scale / 2 - text_h / 2 ), style::colors["text"], oxui::vars::fonts [ "control" ], title, true );

		/* if box is checked, draw check mark */
		if ( vars::items [ id ].val.b ) {
			impl::draw_line( point( vars::next_item_pos.x + 3, vars::next_item_pos.y + check_scale / 2 + 1 ), point( vars::next_item_pos.x + check_scale / 2, vars::next_item_pos.y + check_scale - 3 ), style::colors [ "selected_item" ] );
			impl::draw_line( point( vars::next_item_pos.x + check_scale / 2, vars::next_item_pos.y + check_scale - 3 ), point( vars::next_item_pos.x + check_scale - 2, vars::next_item_pos.y + 3 ), style::colors [ "selected_item" ] );
			// impl::draw_filled_rect( rect( vars::next_item_pos.x + 2, vars::next_item_pos.y + 2, check_scale - 3, check_scale - 3 ), style::colors [ "selection" ] );
		}

		end_control( );
	}
}

void oxui::slider( const std::string_view& title, const std::string_view& id, float min, float max, bool integer_slider ) {
	if ( start_control( id ) ) {
		const auto slider_scale = 4;
		const auto slider_scale_w = 125;

		const auto text_scale = impl::text_scale( vars::fonts [ "control" ], title );
		const auto text_w = text_scale.w;
		const auto text_h = text_scale.h;

		const auto cursor_pos = impl::cursor_pos( );

		/* slider label */
		impl::draw_text( point( vars::next_item_pos.x /* + 12 */, vars::next_item_pos.y ), style::colors [ "text" ], oxui::vars::fonts [ "control" ], title, true );

		/* + and - manual controls */
		// impl::draw_text( point( vars::next_item_pos.x, vars::next_item_pos.y + text_h - 6 ), style::colors [ "text" ], oxui::vars::fonts [ "control" ], "-", false );
		// impl::draw_text( point( vars::next_item_pos.x + 12 + slider_scale_w + 4, vars::next_item_pos.y + text_h - 6 ), style::colors [ "text" ], oxui::vars::fonts [ "control" ], "+", false );

		// if ( hovering( rect( vars::next_item_pos.x, vars::next_item_pos.y + text_h - 3, 10, 10 ) ) ) {
		// 	//impl::draw_text( point( vars::next_item_pos.x, vars::next_item_pos.y + text_h - 3 ), oxui::color { 255, 255, 255, 255 }, oxui::vars::fonts [ "control" ], "-", true );
		// 
		// 	if ( vars::clicked && !integer_slider )
		// 		vars::items [ id ].val.f = std::roundf( vars::items [ id ].val.f - 1.0f );
		// 	else if ( vars::clicked && integer_slider )
		// 		vars::items [ id ].val.i--;
		// }
		// else if ( hovering( rect( vars::next_item_pos.x /* + 12 */ + slider_scale_w + 4, vars::next_item_pos.y + text_h - 3, 10, 10 ) ) ) {
		// 	//impl::draw_text( point( vars::next_item_pos.x + 12 + slider_scale_w + 4, vars::next_item_pos.y + text_h - 3 ), oxui::color { 255, 255, 255, 255 }, oxui::vars::fonts [ "control" ], "+", true );
		// 
		// 	if ( vars::clicked && !integer_slider )
		// 		vars::items [ id ].val.f = std::roundf( vars::items [ id ].val.f + 1.0f );
		// 	else if ( vars::clicked && integer_slider )
		// 		vars::items [ id ].val.i++;
		// }

		/* started clicking in slider area */
		/* set value to clamped value */
		if ( clicking( rect( vars::next_item_pos.x /* + 12 */, vars::next_item_pos.y + text_h, slider_scale_w, slider_scale ) ) && integer_slider )
			vars::items [ id ].val.i = ( cursor_pos.x - ( vars::next_item_pos.x /* + 12 */ ) ) * ( ( max - min ) / ( float ) slider_scale_w );
		else if ( clicking( rect( vars::next_item_pos.x /* + 12 */, vars::next_item_pos.y + text_h, slider_scale_w, slider_scale ) ) && !integer_slider )
			vars::items [ id ].val.f = ( cursor_pos.x - ( vars::next_item_pos.x /* + 12 */ ) ) * ( ( max - min ) / ( float ) slider_scale_w );

		/* clamp value to min and max */
		if ( integer_slider )
			vars::items [ id ].val.i = std::clamp< int >( vars::items [ id ].val.i, min, max );
		else
			vars::items [ id ].val.f = std::clamp< float >( vars::items [ id ].val.f, min, max );

		const auto val = integer_slider ? vars::items [ id ].val.i : vars::items [ id ].val.f;

		/* render slider */
		/* outline */
		impl::draw_rect( rect( vars::next_item_pos.x /*+ 12*/, vars::next_item_pos.y + text_h, slider_scale_w, slider_scale ), style::colors [ "border" ] );

		/* slider slide */
		const auto slider_slide_w = ( float ) slider_scale_w * ( ( float ) ( integer_slider ? vars::items [ id ].val.i : vars::items [ id ].val.f ) / ( max - min ) );
		impl::draw_filled_rect( rect( vars::next_item_pos.x /*+ 12*/ + 1, vars::next_item_pos.y + text_h + 1, std::clamp< float >( slider_slide_w, 3.0f, slider_scale_w ) - 2, slider_scale - 1 ), style::colors [ "selection" ] );
		impl::draw_filled_rect( rect( vars::next_item_pos.x /*+ 12*/ + 2 + std::clamp< float >( slider_slide_w, 3.0f, slider_scale_w ) - 3 - 5, vars::next_item_pos.y + text_h + 2 + slider_scale - 3 - 4, 10, 8 ), style::colors [ "selection" ] );

		vars::next_item_pos.y += text_h;

		end_control( );
	}
}

bool oxui::combo_button( const std::string_view& title, bool opened, bool selected, dimension override_size ) {
	const auto button_w = override_size.w;
	const auto button_h = override_size.h;

	const auto text_scale = impl::text_scale( vars::fonts [ "control" ], title );
	const auto text_w = text_scale.w;
	const auto text_h = text_scale.h;

	if ( selected ) {
		impl::draw_filled_rect( rect( vars::next_item_pos.x, vars::next_item_pos.y, button_w, button_h ), style::colors [ "selected_item" ] );
		impl::draw_line( point( vars::next_item_pos.x, vars::next_item_pos.y ), point( vars::next_item_pos.x, vars::next_item_pos.y + button_h ), style::colors [ "border" ] );
		impl::draw_line( point( vars::next_item_pos.x + button_w, vars::next_item_pos.y ), point( vars::next_item_pos.x + button_w, vars::next_item_pos.y + button_h ), style::colors [ "border" ] );
		impl::draw_text( point( vars::next_item_pos.x + 4, vars::next_item_pos.y + 20 - text_h - 2 ), style::colors [ "text" ], vars::fonts [ "control" ], title, true );
		vars::next_item_pos.y += button_h;

		return false;
	}

	if ( hovering( rect( vars::next_item_pos.x, vars::next_item_pos.y, button_w, button_h ) ) ) {
		if ( opened ) {
			impl::draw_filled_rect( rect( vars::next_item_pos.x, vars::next_item_pos.y, button_w, button_h ), style::colors [ "selected_item" ] );
			impl::draw_line( point( vars::next_item_pos.x, vars::next_item_pos.y ), point( vars::next_item_pos.x, vars::next_item_pos.y + button_h ), style::colors [ "border" ] );
			impl::draw_line( point( vars::next_item_pos.x + button_w, vars::next_item_pos.y ), point( vars::next_item_pos.x + button_w, vars::next_item_pos.y + button_h ), style::colors [ "border" ] );
		}
		else {
			impl::draw_filled_rect( rect( vars::next_item_pos.x, vars::next_item_pos.y, button_w, button_h ), style::colors [ "button_selected" ] );

			/* button border */
			impl::draw_rect( rect( vars::next_item_pos.x, vars::next_item_pos.y, button_w, button_h ), style::colors [ "selection" ] );
		}

		/* button title */
		impl::draw_text( point( vars::next_item_pos.x + 4, vars::next_item_pos.y + 20 - text_h - 2 ), style::colors[ "text" ], vars::fonts [ "control" ], title, true );

		if ( vars::clicked ) {
			/* dropdown icon */
			if ( !opened ) {
				impl::draw_line( point( vars::next_item_pos.x - 13, vars::next_item_pos.y + 10 ), point( vars::next_item_pos.x - 9, vars::next_item_pos.y + 14 ), style::colors [ "border" ] );
				impl::draw_line( point( vars::next_item_pos.x - 9, vars::next_item_pos.y + 14 ), point( vars::next_item_pos.x - 6, vars::next_item_pos.y + 10 ), style::colors [ "border" ] );
			}

			if ( opened )
				vars::next_item_pos.y += button_h;
			else {
				vars::next_item_pos.y += button_h;
				end_control( );
			}

			return true;
		}
	}
	else {
		if ( opened ) {
			impl::draw_filled_rect( rect( vars::next_item_pos.x, vars::next_item_pos.y, button_w, button_h ), style::colors [ "control" ] );
			impl::draw_line( point( vars::next_item_pos.x, vars::next_item_pos.y ), point( vars::next_item_pos.x, vars::next_item_pos.y + button_h ), style::colors [ "border" ] );
			impl::draw_line( point( vars::next_item_pos.x + button_w, vars::next_item_pos.y ), point( vars::next_item_pos.x + button_w, vars::next_item_pos.y + button_h ), style::colors [ "border" ] );
		}
		else {
			impl::draw_filled_rect( rect( vars::next_item_pos.x, vars::next_item_pos.y, button_w, button_h ), style::colors [ "button" ] );
			
			/* button border */
			impl::draw_rect( rect( vars::next_item_pos.x, vars::next_item_pos.y, button_w, button_h ), style::colors [ "border" ] );
		}

		/* button title */
		impl::draw_text( point( vars::next_item_pos.x + 4, vars::next_item_pos.y + 20 - text_h - 2 ), style::colors [ "text" ], oxui::vars::fonts [ "control" ], title, true );
	}

	if ( !opened ) {
		const auto hovered = hovering( rect( vars::next_item_pos.x, vars::next_item_pos.y, button_w, button_h ) );

		/* dropdown arrow */
		impl::draw_line( point( vars::next_item_pos.x + button_w - 13, vars::next_item_pos.y + 10 ), point( vars::next_item_pos.x + button_w - 9, vars::next_item_pos.y + 13 ), hovered ? style::colors [ "selected_item" ] : style::colors [ "border" ] );
		impl::draw_line( point( vars::next_item_pos.x + button_w - 10, vars::next_item_pos.y + 13 ), point( vars::next_item_pos.x + button_w - 6, vars::next_item_pos.y + 10 ), hovered ? style::colors [ "selected_item" ] : style::colors [ "border" ] );

		vars::next_item_pos.y += button_h;
		end_control( );
	}
	else {
		vars::next_item_pos.y += button_h;
	}

	return false;
}

void oxui::dropdown( const std::string_view& title, const std::string_view& id, const std::vector< std::string_view > items ) {
	/* if we press the dropdown button */
	const auto button_w = 125;
	const auto button_h = 20;

	/* button title */
	impl::draw_text( point( vars::next_item_pos.x, vars::next_item_pos.y ), style::colors [ "text" ], vars::fonts [ "control" ], title, true );

	vars::next_item_pos.y += impl::text_scale( vars::fonts [ "control" ], title ).h;

	if ( combo_button( items [ vars::items [ id ].val.i ] ) ) {
		/* draw items at render end and disable input */
		vars::opening_dropdown = false;
		vars::dropdown_point = vars::next_item_pos;
		vars::dropdown_point.y -= button_h;
		vars::disable_menu_input = true;
		vars::selected_dropdown_items = items;
		vars::selected_dropdown_item_ptr = &vars::items [ id ].val.i;
	}
}

void oxui::listbox( const std::string_view& title, const std::string_view& id, const std::vector< std::string_view > items, dimension size ) {
	auto button_w = size.w - 16;
	auto button_h = 20;
	
	auto button_num = 0;

	impl::draw_text( point( vars::next_item_pos.x, vars::next_item_pos.y ), style::colors [ "text" ], vars::fonts [ "control" ], title, true );

	vars::next_item_pos.y += impl::text_scale( vars::fonts [ "control" ], title ).h;

	const auto pos = vars::next_item_pos;

	auto in_list = hovering( rect( vars::next_item_pos.x, vars::next_item_pos.y, size.w, size.h ) );

	/* if we scrolled, update scroll amount */
	if ( vars::scroll_amount != 0.0f && in_list ) {
		vars::items [ id ].scrolled_amount += vars::scroll_amount * button_h;
	}

	if ( size.h / items.size( ) >= 20 )
		button_h = size.h / items.size( );

	vars::items [ id ].scrolled_amount = std::clamp( vars::items [ id ].scrolled_amount, 0, int( items.size( ) * button_h ) - size.h );

	impl::draw_rect( rect( vars::next_item_pos.x - 1, vars::next_item_pos.y - 1, size.w + 1, size.h + 1 ), style::colors [ "border" ] );

	impl::clip( rect( vars::next_item_pos.x, vars::next_item_pos.y, size.w, size.h ) );

	vars::next_item_pos.y -= vars::items [ id ].scrolled_amount;

	for ( const auto& item : items ) {
		auto y_offset = button_num * button_h;

		/* only draw items that are to be displayed on screen (increases framerate by a lot) */
		if ( combo_button( item, true, vars::items [ id ].val.i == button_num, dimension( button_w, button_h ) ) && in_list ) {
			vars::items [ id ].val.i = button_num;
		}

		button_num++;
	}

	impl::remove_clip( );

	impl::draw_filled_rect( rect( pos.x + size.w - 16, pos.y, 16, size.h ), style::colors [ "scrollbar_track" ] );

	const auto scrollbar_max_height = size.h - 16 * 2;
	const auto cursor_pos = impl::cursor_pos( );

	if ( clicking( rect( pos.x + size.w - 16, pos.y + 16, 16, scrollbar_max_height ) ) ) {
		auto scroll_delta = cursor_pos.y - ( pos.y + 16 );
		scroll_delta = std::clamp( scroll_delta, 0, scrollbar_max_height );
		vars::items [ id ].scrolled_amount = int( float( button_h * items.size( ) ) / float( scrollbar_max_height ) * scroll_delta );
	}

	auto max_size = items.size( ) * button_h;
	vars::items [ id ].scrolled_amount = std::clamp( vars::items [ id ].scrolled_amount, 0, int( items.size( ) * button_h ) - size.h );

	impl::draw_filled_rect( rect( pos.x + size.w - 16, pos.y + 16 + vars::items [ id ].scrolled_amount * ( float( scrollbar_max_height ) / float( button_h * items.size( ) ) ), 16, scrollbar_max_height * ( float( size.h ) / ( items.size( ) * button_h ) ) ) , style::colors [ "scrollbar_handle" ] );
}

void oxui::end( ) {
	if ( !vars::open )
		return;

	const auto button_w = 125;
	const auto button_h = 20;

	/* draw dropdown items */
	if ( !vars::selected_dropdown_items.empty( )
		&& vars::selected_dropdown_item_ptr
		&& vars::dropdown_point.x != 0
		&& vars::dropdown_point.y != 0
		&& vars::opening_dropdown ) {
		auto iter_num = 0;

		vars::next_item_pos = vars::dropdown_point;

		/* draw all dropdown items */
		for ( const auto& item : vars::selected_dropdown_items ) {
			/* draw new button for new option */
			vars::disable_menu_input = false;

			if ( combo_button( item, true ) ) {
				/* last item in iterator */
				if ( iter_num == vars::selected_dropdown_items.size( ) - 1 )
					impl::draw_line( point( vars::next_item_pos.x, vars::next_item_pos.y + button_h ), point( vars::next_item_pos.x + button_w, vars::next_item_pos.y + button_h ), style::colors [ "border" ] );

				vars::opening_dropdown = false;
				vars::dropdown_point = point( 0, 0 );
				*vars::selected_dropdown_item_ptr = iter_num;
				vars::selected_dropdown_items.clear( );
				break;
			}

			/* last item in iterator */
			if ( iter_num == vars::selected_dropdown_items.size( ) - 1 )
				impl::draw_line( point( vars::next_item_pos.x, vars::next_item_pos.y ), point( vars::next_item_pos.x + button_w, vars::next_item_pos.y ), style::colors [ "border" ] );

			vars::disable_menu_input = true;

			/* increase dropdown object iteration number */
			iter_num++;
		}
	}

	if ( !vars::opening_dropdown
		&& !vars::selected_dropdown_items.empty( ) ) {
		vars::opening_dropdown = true;
	}

	vars::scroll_amount = 0.0f;
}