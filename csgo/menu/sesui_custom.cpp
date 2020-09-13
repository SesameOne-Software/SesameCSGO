#include <windows.h>
#include "sesui_custom.hpp"
#include "../renderer/d3d9.hpp"
#include "logo.hpp"
#include "../globals.hpp"

extern std::string g_pfp_data;
extern std::wstring g_username;

int current_subtab = 0;
bool disabled_input_for_subtab_click = false;

bool sesui::custom::tab_open = false;
float sesui::custom::tab_open_timer = 0.0f;
sesui::font sesui::custom::tab_font = sesui::font( L"sesame_icons", 26, 400, false );

bool sesui::custom::begin_group( const ses_string& name, const rect& fraction, const rect& extra ) {
	const auto window = globals::window_ctx.find( globals::cur_window );

	if ( window == globals::window_ctx.end( ) )
		throw "Current window context not valid.";

	const auto parts = split( name.get( ) );
	const auto title = ses_string( parts.first.data( ) );
	const auto& id = parts.second;

	window->second.cur_group = parts.first + id;

	const auto same_line_backup = window->second.cursor_stack.back( );

	if ( window->second.same_line ) {
		window->second.cursor_stack.back( ).y -= window->second.last_cursor_offset;
		window->second.cursor_stack.back( ).x += style.same_line_offset;
	}

	auto bounds = rect( window->second.cursor_stack.back( ).x + scale_dpi( extra.x ), window->second.cursor_stack.back( ).y + scale_dpi( extra.y ), window->second.main_area.w * fraction.w + extra.w, window->second.main_area.h * fraction.h + extra.h );

	bounds.x += scale_dpi( window->second.main_area.w * fraction.x );
	bounds.y += scale_dpi( window->second.main_area.h * fraction.y );

	const auto titlebar_rect = rect( bounds.x, bounds.y, bounds.w, bounds.h * style.titlebar_height );
	const auto window_rect = rect( titlebar_rect.x, titlebar_rect.y + scale_dpi( titlebar_rect.h ), bounds.w, bounds.h - titlebar_rect.h );
	const auto calc_height1 = window->second.group_ctx [ window->second.cur_group ].calc_height + scale_dpi( style.initial_offset.y * 2.0f );
	const auto max_height = window_rect.h > calc_height1 ? window_rect.h : calc_height1;

	if ( input::mouse_in_region( bounds ) && input::scroll_amount != 0.0f ) {
		window->second.group_ctx [ window->second.cur_group ].scroll_amount_target -= input::scroll_amount * 18.0f;
		window->second.group_ctx [ window->second.cur_group ].scroll_amount_target = std::clamp< float >( window->second.group_ctx [ window->second.cur_group ].scroll_amount_target, 0.0f, max_height - window_rect.h );
	}

	const auto delta = window->second.group_ctx [ window->second.cur_group ].scroll_amount_target - window->second.group_ctx [ window->second.cur_group ].scroll_amount;
	window->second.group_ctx [ window->second.cur_group ].scroll_amount += delta * style.animation_speed * 2.0f * draw_list.get_frametime( );

	const auto percentage_scrolled = window->second.group_ctx [ window->second.cur_group ].scroll_amount / ( max_height - window_rect.h );

	/* window rect */
	draw_list.add_rect( titlebar_rect, style.window_node_desc, true );
	draw_list.add_rect( window_rect, style.window_foreground, true );

	vec2 text_size;
	draw_list.get_text_size( style.control_font, title, text_size );
	draw_list.add_text( vec2( bounds.x + scale_dpi( bounds.w ) * 0.5f - text_size.x * 0.5f, bounds.y + scale_dpi( titlebar_rect.h - 6.0f ) * 0.5f - text_size.y * 0.5f ), style.control_font, title, true, color( 0.78f, 0.78f, 0.78f, 1.0f ) );

	//window->second.cursor_stack.back ( ).x += window->second.main_area.w * fraction.w + style.spacing;
	//window->second.last_cursor_offset = window->second.main_area.h * fraction.h + style.spacing;
	//window->second.cursor_stack.back ( ).y += window->second.last_cursor_offset;

	window->second.cursor_stack.push_back( vec2( bounds.x + scale_dpi( style.initial_offset.x ), bounds.y + scale_dpi( titlebar_rect.h + style.initial_offset.y - window->second.group_ctx [ window->second.cur_group ].scroll_amount ) ) );
	window->second.cursor_stack.push_back( vec2( bounds.x + scale_dpi( style.initial_offset.x ), bounds.y + scale_dpi( titlebar_rect.h + style.initial_offset.y - window->second.group_ctx [ window->second.cur_group ].scroll_amount ) ) );

	if ( window->second.same_line ) {
		window->second.cursor_stack.back( ) = same_line_backup;
		window->second.same_line = false;
	}

	const auto clip_area = rect( window_rect.x + scale_dpi( style.initial_offset.x ), window_rect.y + scale_dpi( style.initial_offset.y ), window_rect.w - style.initial_offset.x * 2.0f, window_rect.h - style.initial_offset.y * 2.0f );

	/* scales controls according to the room we have inside the group */
	style.slider_size.x = clip_area.w - style.initial_offset.x;
	style.button_size.x = clip_area.w - style.initial_offset.x;
	style.same_line_offset = clip_area.w - style.initial_offset.x - style.inline_button_size.x;

	const auto viewable_ratio = scale_dpi( clip_area.h ) / scale_dpi( window->second.group_ctx [ window->second.cur_group ].calc_height );
	const auto scrollbar_area = scale_dpi( clip_area.h ) - scale_dpi( style.scroll_arrow_height ) * 2.0f;
	const auto thumb_height = scrollbar_area * viewable_ratio;
	const auto track_space = window->second.group_ctx [ window->second.cur_group ].calc_height - scale_dpi( clip_area.h );
	const auto thumb_space = scale_dpi( clip_area.h ) - thumb_height;
	const auto scroll_jump = track_space / thumb_space;

	draw_list.add_rect( rect( clip_area.x + scale_dpi( clip_area.w ) - 2.0f, clip_area.y, unscale_dpi( 2.0f ), clip_area.h ), color( 0.0f, 0.0f, 0.0f, 0.2f ), true );
	draw_list.add_rect( rect( clip_area.x + scale_dpi( clip_area.w ) - 2.0f, clip_area.y + percentage_scrolled * ( scale_dpi( clip_area.h ) - thumb_height ), unscale_dpi( 2.0f ), unscale_dpi( thumb_height ) ), style.control_accent, true );

	draw_list.add_clip( clip_area );

	return true;
}

void sesui::custom::end_group( ) {
	const auto window = globals::window_ctx.find( globals::cur_window );

	if ( window == globals::window_ctx.end( ) )
		throw "Current window context not valid.";

	window->second.group_ctx [ window->second.cur_group ].calc_height = unscale_dpi( window->second.cursor_stack.at( window->second.cursor_stack.size( ) - 1 ).y - window->second.cursor_stack.at( window->second.cursor_stack.size( ) - 2 ).y );
	window->second.cur_group = L"";
	window->second.cursor_stack.pop_back( );
	window->second.cursor_stack.pop_back( );

	draw_list.remove_clip( );
}

bool sesui::custom::begin_subtabs( int count ) {
	const auto window = globals::window_ctx.find( globals::cur_window );

	if ( window == globals::window_ctx.end( ) )
		throw "Current window context not valid.";

	window->second.cur_subtab_index = 0;

	return true;
}

void sesui::custom::subtab( const ses_string& name, const ses_string& desc, const rect& fraction, const rect& extra, int& selected ) {
	const auto window = globals::window_ctx.find( globals::cur_window );

	if ( window == globals::window_ctx.end( ) )
		throw "Current window context not valid.";

	auto titlebar_rect = rect( window->second.bounds.x, window->second.bounds.y, window->second.bounds.w, window->second.bounds.h * style.titlebar_height );

	if ( input::mouse_in_region( rect( titlebar_rect.x, titlebar_rect.y, titlebar_rect.h, titlebar_rect.h ) ) && input::key_pressed( VK_LBUTTON ) )
		selected = -1;

	current_subtab = selected;

	/* make sure to only use subtabs if we arent in a subtab yet */
	if ( selected != -1 )
		return;

	auto bounds = rect( window->second.cursor_stack.back( ).x + scale_dpi( extra.x ), window->second.cursor_stack.back( ).y + scale_dpi( extra.y ), window->second.main_area.w * fraction.w + extra.w, window->second.main_area.h * fraction.h + extra.h );

	bounds.x += scale_dpi( window->second.main_area.w * fraction.x );
	bounds.y += scale_dpi( window->second.main_area.h * fraction.y );

	if ( input::mouse_in_region( bounds ) && input::key_pressed( VK_LBUTTON ) ) {
		input::enabled = false;
		disabled_input_for_subtab_click = true;
		input::mouse_pos = vec2( 0.0f, 0.0f );
		input::start_click_pos = vec2( 0.0f, 0.0f );

		selected = window->second.cur_subtab_index;

		globals::window_ctx [ globals::cur_window ].anim_time = std::array< float, 512 > { 0.0f };
	}

	current_subtab = selected;

	draw_list.add_rect( bounds, style.window_foreground, true );
	draw_list.add_text( vec2( bounds.x + scale_dpi( style.padding ) * 2.0f, bounds.y + scale_dpi( style.padding ) ), style.control_font, name, false, style.control_accent );

	auto desc_bounds = rect( bounds.x + scale_dpi( style.padding ) * 2.0f, bounds.y + scale_dpi( bounds.h ) * 0.5f - scale_dpi( bounds.h * 0.4f ) * 0.5f, bounds.w - style.padding * 2.0f * 2.0f, bounds.h * 0.4f );

	draw_list.add_rect( desc_bounds, style.window_node_desc, true );

	vec2 node_desc_text_size;
	draw_list.get_text_size( style.control_font, desc, node_desc_text_size );

	draw_list.add_text( vec2( desc_bounds.x + scale_dpi( style.padding ), desc_bounds.y + scale_dpi( desc_bounds.h ) * 0.5f - node_desc_text_size.y * 0.5f ), style.control_font, desc, false, style.control_text );

	draw_list.add_arrow( vec2( desc_bounds.x + scale_dpi( desc_bounds.w - style.padding - 6.0f ), desc_bounds.y + scale_dpi( desc_bounds.h ) * 0.5f ), 6.0f, 180.0f, style.control_accent, false );

	window->second.cur_subtab_index++;
}

void sesui::custom::end_subtabs( ) {
	const auto window = globals::window_ctx.find( globals::cur_window );

	if ( window == globals::window_ctx.end( ) )
		throw "Current window context not valid.";

	window->second.cur_subtab_index = 0;
}

bool sesui::custom::begin_tabs( int count, float width ) {
	const auto window = globals::window_ctx.find( globals::cur_window );

	if ( window == globals::window_ctx.end( ) )
		throw "Current window context not valid.";

	const auto titlebar_rect = rect( window->second.bounds.x, window->second.bounds.y, window->second.bounds.w, window->second.bounds.h * style.titlebar_height );
	const auto window_rect = rect( titlebar_rect.x, titlebar_rect.y + scale_dpi( titlebar_rect.h ), window->second.bounds.w, window->second.bounds.h - titlebar_rect.h );

	window->second.tab_width = std::max< float >( titlebar_rect.h, tab_open_timer * ( window->second.bounds.w * static_cast< float > ( width ) ) );
	window->second.tab_count = count;

	const auto watermark_rect = rect( window_rect.x + scale_dpi( titlebar_rect.h ), window_rect.y + scale_dpi( window_rect.h - 20.0f ), window_rect.w - scale_dpi( titlebar_rect.h ), 20.0f );
	draw_list.add_rect( watermark_rect, style.window_foreground, true, true );
	vec2 watermark_text_size;
	draw_list.get_text_size( style.control_font, _( L"Sesame " SESAME_VERSION ), watermark_text_size );
	draw_list.add_text( vec2( watermark_rect.x + scale_dpi( 6.0f ), watermark_rect.y + scale_dpi( watermark_rect.h * 0.5f - watermark_text_size.y * 0.5f ) ), style.control_font, _( L"Sesame " SESAME_VERSION ), false, style.control_text, true );

	/* dim menu background */
	if ( tab_open_timer )
		draw_list.add_rect( window_rect, color( ).lerp( color( 0.0f, 0.0f, 0.0f, 0.5f ), tab_open_timer ), true, true );

	/* menu tabs bar */
	draw_list.add_rect( rect( window_rect.x, window_rect.y, window->second.tab_width, window_rect.h ), style.window_foreground, true, true );

	window->second.main_area = rect( window_rect.x + titlebar_rect.h + scale_dpi( style.initial_offset.x ), window->second.bounds.y + scale_dpi( titlebar_rect.h + style.initial_offset.y ), window->second.bounds.w - titlebar_rect.h - style.initial_offset.x * 2.0f, window->second.bounds.h - titlebar_rect.h - style.initial_offset.y * 2.0f - 20.0f );
	window->second.cursor_stack.back( ) = vec2( window->second.main_area.x, window->second.main_area.y );

	/* highlight selected tab */
	const auto selected_tab_pos = window_rect.y + window->second.selected_tab_offset;
	draw_list.add_rect( rect( window_rect.x + window->second.tab_width - scale_dpi( 6.0f ), selected_tab_pos, 6.0f, titlebar_rect.h ), style.window_accent, true, true );

	draw_list.add_clip( rect( window_rect.x, window_rect.y, window->second.tab_width, window_rect.h ), true );

	return true;
}

void sesui::custom::tab( const ses_string& icon, const ses_string& name, int& selected ) {
	const auto window = globals::window_ctx.find( globals::cur_window );

	if ( window == globals::window_ctx.end( ) )
		throw "Current window context not valid.";

	const auto titlebar_rect = rect( window->second.bounds.x, window->second.bounds.y, window->second.bounds.w, window->second.bounds.h * style.titlebar_height );
	const auto window_rect = rect( titlebar_rect.x, titlebar_rect.y + scale_dpi( titlebar_rect.h ), window->second.bounds.w, window->second.bounds.h - titlebar_rect.h );

	const auto tab_dim = vec2( window->second.tab_width, titlebar_rect.h );
	const auto tab_pos = window_rect.y + scale_dpi( tab_dim.y * window->second.cur_tab_index );
	const auto text_pos = tab_pos + scale_dpi( tab_dim.y ) / 2.0f;

	const auto parts = split( name.get( ) );
	const auto title = ses_string( parts.first.data( ) );
	const auto& id = parts.second;

	vec2 text_size;
	draw_list.get_text_size( style.tab_font, title, text_size );

	if ( input::mouse_in_region( rect( window_rect.x, tab_pos, tab_dim.x, tab_dim.y ), true ) && input::key_state [ VK_LBUTTON ] && !input::old_key_state [ VK_LBUTTON ] ) {
		selected = window->second.cur_tab_index;

		/* reset animations */
		for ( auto& anim_time : window->second.anim_time )
			anim_time = 0.0f;

		globals::window_ctx [ globals::cur_window ].anim_time = std::array< float, 512 > { 0.0f };
	}

	if ( selected == window->second.cur_tab_index ) {
		const auto delta = std::fabsf( ( tab_pos - window_rect.y ) - window->second.selected_tab_offset );

		if ( window->second.selected_tab_offset < tab_pos - window_rect.y )
			window->second.selected_tab_offset += delta * style.animation_speed * 3.0f * draw_list.get_frametime( );
		else
			window->second.selected_tab_offset -= delta * style.animation_speed * 3.0f * draw_list.get_frametime( );
	}

	draw_list.add_text( vec2( window_rect.x + scale_dpi( titlebar_rect.h + 6.0f ), text_pos - text_size.y / 2.0f ), style.tab_font, title, true, style.control_text, true );

	vec2 icon_text_size;
	draw_list.get_text_size( tab_font, icon, icon_text_size );
	draw_list.add_text( vec2( window_rect.x + scale_dpi( titlebar_rect.h * 0.5f ) - icon_text_size.x * 0.5f, text_pos - icon_text_size.y / 2.0f ), tab_font, icon, true, style.control_text, true );

	window->second.cur_tab_index++;
}

void sesui::custom::end_tabs( ) {
	const auto window = globals::window_ctx.find( globals::cur_window );

	if ( window == globals::window_ctx.end( ) )
		throw "Current window context not valid.";

	const auto titlebar_rect = rect( window->second.bounds.x, window->second.bounds.y, window->second.bounds.w, window->second.bounds.h * style.titlebar_height );
	const auto window_rect = rect( titlebar_rect.x, titlebar_rect.y + scale_dpi( titlebar_rect.h ), window->second.bounds.w, window->second.bounds.h - titlebar_rect.h );

	/* pfp */
	//const auto tabs_w = window_rect.w * static_cast< float > ( 0.2f/*width*/ );
	//draw_list.add_texture( std::vector< uint8_t >( g_pfp_data.data( ), g_pfp_data.data( ) + g_pfp_data.length( ) ), vec2( window->second.bounds.x + tabs_w * 0.5f - scale_dpi( 48.0f * 0.7f ) * 0.5f, window->second.bounds.y + scale_dpi( window->second.bounds.h - 100.0f ) - scale_dpi( 48.0f * 0.7f ) * 0.5f ), vec2( 48.0f, 48.0f ), vec2( 0.7f, 0.7f ), color( 1.0f, 1.0f, 1.0f, tab_open_timer ), true );
//
	////draw_list.add_rect ( rect( window->second.bounds.x + tabs_w * 0.5f - scale_dpi ( 48.0f * 0.7f ) * 0.5f, window->second.bounds.y + scale_dpi ( window->second.bounds.h - 100.0f ) - scale_dpi ( 48.0f * 0.7f ) * 0.5f, 48.0f * 0.7f, 48.0f * 0.7f ), color( 1.0f, 1.0f, 1.0f, 1.0f ), false, true );
//
	//vec2 username_text_size;
	//draw_list.get_text_size( style.control_font, g_username.data( ), username_text_size );
	//draw_list.add_text( vec2( window->second.bounds.x + tabs_w * 0.5f - username_text_size.x * 0.5f, window->second.bounds.y + scale_dpi( window->second.bounds.h - 100.0f ) - scale_dpi( 48.0f * 0.7f ) * 0.5f + scale_dpi( style.padding ) + scale_dpi( 48.0f ) ), style.control_font, g_username.data( ), false, color( 1.0f, 1.0f, 1.0f, tab_open_timer ), true );

	draw_list.remove_clip( true );

	window->second.cur_tab_index = 0;
	window->second.tab_count = 0;
}

bool sesui::custom::begin_window( const ses_string& name, const rect& bounds, bool& opened, uint32_t flags ) {
	sesui::draw_list.create_font( tab_font, globals::dpi != globals::last_dpi );

	if ( !opened )
		return false;

	const auto parts = split( name.get( ) );
	const auto title = ses_string( parts.first.data( ) );
	const auto& id = parts.second;

	/* set current window context */
	globals::cur_window = parts.first + id;

	auto window_entry = globals::window_ctx.find( title.get( ) );

	if ( window_entry == globals::window_ctx.end( ) ) {
		int top_layer = -1;

		/* add on top of all other windows create before this one */
		if ( !globals::window_ctx.empty( ) ) {
			for ( const auto& window : globals::window_ctx )
				if ( window.second.layer > top_layer )
					top_layer = window.second.layer;
		}

		if ( top_layer != -1 )
			globals::window_ctx [ globals::cur_window ].layer = top_layer + 1;
		else
			globals::window_ctx [ globals::cur_window ].layer = 0;

		globals::window_ctx [ globals::cur_window ].bounds = bounds;
		globals::window_ctx [ globals::cur_window ].anim_time = std::array< float, 512 > { 0.0f };
		globals::window_ctx [ globals::cur_window ].cur_index = 0;
		//globals::window_ctx [ globals::cur_window ].cur_tab_index = 0;
		globals::window_ctx [ globals::cur_window ].tab_count = 0;
		globals::window_ctx [ globals::cur_window ].selected_tab_offset = 0.0f;

		window_entry = globals::window_ctx.find( globals::cur_window );
	}

	if ( window_entry == globals::window_ctx.end( ) )
		throw "Current window context not valid.";

	window_entry->second.cur_group = L"";

	auto titlebar_rect = rect( window_entry->second.bounds.x, window_entry->second.bounds.y, window_entry->second.bounds.w, window_entry->second.bounds.h * style.titlebar_height );

	/* window moving behavior */
	if ( !( flags & window_flags::no_move ) ) {
		if ( !window_entry->second.moving && input::mouse_in_region( titlebar_rect ) && input::key_pressed( VK_LBUTTON ) ) {
			window_entry->second.moving = true;
			window_entry->second.click_offset = vec2( input::mouse_pos.x - window_entry->second.bounds.x, input::mouse_pos.y - window_entry->second.bounds.y );
		}
		else if ( window_entry->second.moving && input::key_down( VK_LBUTTON ) ) {
			window_entry->second.bounds.x = input::mouse_pos.x - window_entry->second.click_offset.x;
			window_entry->second.bounds.y = input::mouse_pos.y - window_entry->second.click_offset.y;
		}
		else {
			window_entry->second.moving = false;
		}
	}

	titlebar_rect = rect( window_entry->second.bounds.x, window_entry->second.bounds.y, window_entry->second.bounds.w, window_entry->second.bounds.h * style.titlebar_height );

	const auto resizing_area = rect( window_entry->second.bounds.x + scale_dpi( window_entry->second.bounds.w - style.resize_grab_radius ), window_entry->second.bounds.y + scale_dpi( window_entry->second.bounds.h - style.resize_grab_radius ), style.resize_grab_radius * 2.0f, style.resize_grab_radius * 2.0f );

	/* window resizing behavior */
	if ( !( flags & window_flags::no_resize ) ) {
		if ( !window_entry->second.resizing && input::mouse_in_region( resizing_area ) && input::key_pressed( VK_LBUTTON ) ) {
			window_entry->second.resizing = true;
		}
		else if ( window_entry->second.resizing && input::key_down( VK_LBUTTON ) ) {
			window_entry->second.bounds.w = std::max< float >( unscale_dpi( input::mouse_pos.x - window_entry->second.bounds.x ), style.window_min_size.x );
			window_entry->second.bounds.h = std::max< float >( unscale_dpi( input::mouse_pos.y - window_entry->second.bounds.y ), style.window_min_size.y );
		}
		else {
			window_entry->second.resizing = false;
		}
	}

	const auto window_rect = rect( titlebar_rect.x, titlebar_rect.y + scale_dpi( titlebar_rect.h ), window_entry->second.bounds.w, window_entry->second.bounds.h - titlebar_rect.h );
	const auto exit_rect = rect( window_entry->second.bounds.x + scale_dpi( window_entry->second.bounds.w - 16.0f - 8.0f ), window_entry->second.bounds.y + scale_dpi( titlebar_rect.h - 6.0f ) * 0.5f - scale_dpi( 8.0f ), 16.0f, 16.0f );

	draw_list.add_rect( titlebar_rect, color( style.window_foreground.r, style.window_foreground.g, style.window_foreground.b, style.window_foreground.a * 0.7f ), true );
	draw_list.add_rect( window_rect, style.window_background, true );

	/* menu bar */
	const auto menu_bar_rect = rect( titlebar_rect.x, titlebar_rect.y, titlebar_rect.h, titlebar_rect.h );
	draw_list.add_rect( menu_bar_rect, style.window_foreground.lerp( style.window_background, tab_open_timer * 0.76f ), true );

	if ( current_subtab == -1 ) {
		if ( tab_open )
			input::enable_input( false );

		auto old_tab_open = tab_open;

		/* close if opened and clicked outside of menu bar */
		if ( tab_open && input::key_state [ VK_LBUTTON ] && !input::old_key_state [ VK_LBUTTON ] && !input::mouse_in_region( menu_bar_rect, true ) && !input::mouse_in_region( rect( window_rect.x, window_rect.y, std::max< float >( titlebar_rect.h, tab_open_timer * ( window_rect.w * static_cast< float > ( 0.2f/*width*/ ) ) ), window_rect.h ), true ) )
			tab_open = false;

		if ( input::mouse_in_region( menu_bar_rect, true ) && input::key_state [ VK_LBUTTON ] && !input::old_key_state [ VK_LBUTTON ] )
			tab_open = !tab_open;

		if ( !tab_open && old_tab_open )
			input::enable_input( true );

		/* menu icon */
		vec2 icon_text_size;
		draw_list.get_text_size( tab_font, _( L"G" ), icon_text_size );
		draw_list.add_text( vec2( titlebar_rect.x + scale_dpi( titlebar_rect.h ) * 0.5f - icon_text_size.x * 0.5f, titlebar_rect.y + scale_dpi( titlebar_rect.h ) * 0.5f - icon_text_size.y * 0.5f ), tab_font, _( L"G" ), true, style.control_text );
	}
	else {
		/* back arrow */
		draw_list.add_arrow( vec2( titlebar_rect.x + scale_dpi( titlebar_rect.h ) * 0.5f + scale_dpi( 4.0f ), titlebar_rect.y + scale_dpi( titlebar_rect.h ) * 0.5f ), 8.0f, 0.0f, style.control_text, false );
		draw_list.add_arrow( vec2( titlebar_rect.x + scale_dpi( titlebar_rect.h ) * 0.5f + scale_dpi( 4.0f ) + unscale_dpi( 1.0f ), titlebar_rect.y + scale_dpi( titlebar_rect.h ) * 0.5f + unscale_dpi( 1.0f ) ), 8.0f, 0.0f, color( 0, 0, 0, 255 ), false );
	}

	/* tab opening easing and timer */ {
		const auto delta = std::fabsf( ( tab_open ? 1.0f : 0.0f ) - tab_open_timer );

		if ( tab_open_timer < ( tab_open ? 1.0f : 0.0f ) )
			tab_open_timer += delta * style.animation_speed * 3.0f * draw_list.get_frametime( );
		else
			tab_open_timer -= delta * style.animation_speed * 3.0f * draw_list.get_frametime( );
	}

	/* logo */
	draw_list.add_texture( resources::sesame_logo, vec2( titlebar_rect.x + scale_dpi( 10.0f + titlebar_rect.h ), titlebar_rect.y + scale_dpi( titlebar_rect.h * 0.5f - ( resources::sesame_logo_h * ( resources::sesame_logo_w / resources::sesame_logo_h * 0.25f ) ) * 0.5f ) ), vec2( resources::sesame_logo_w, resources::sesame_logo_h ), vec2( 1.0f * 0.25f, resources::sesame_logo_w / resources::sesame_logo_h * 0.25f ), color( 1.0f, 1.0f, 1.0f, 1.0f ) );

	/* window rect */
	//draw_list.add_rounded_rect ( titlebar_rect, style.rounding, style.window_accent, true );
	//draw_list.add_rounded_rect ( titlebar_rect, style.rounding, style.window_accent_borders, false );
	//draw_list.add_rounded_rect ( window_rect, style.rounding, style.window_background, true );
	//draw_list.add_rounded_rect ( window_rect, style.rounding, style.window_borders, false );
	//
	///* covering rounding for top part of main window */
	//draw_list.add_rect ( remove_rounding_rect, style.window_background, true );
	//draw_list.add_rect ( remove_rounding_rect, style.window_borders, false );
	//draw_list.add_rect ( remove_rounding_rect_filler, style.window_background, true );

	//if ( !( flags & window_flags::no_title ) ) {
	//	vec2 text_size;
	//	draw_list.get_text_size ( style.control_font, title, text_size );
	//
	//	draw_list.add_text ( vec2 ( window_entry->second.bounds.x + scale_dpi ( style.spacing ), window_entry->second.bounds.y + scale_dpi ( titlebar_rect.h - 6.0f ) * 0.5f - text_size.y * 0.5f ), style.control_font, title, true, color ( 0.78f, 0.78f, 0.78f, 1.0f ) );
	//}

	/* close menu button*/
	if ( !( flags & window_flags::no_closebutton ) ) {
		draw_list.add_rounded_rect( exit_rect, 6.0f, color( 0.11f, 0.11f, 0.11f, 1.0f ), true );
		draw_list.add_rounded_rect( exit_rect, 6.0f, style.window_accent, false ); /* rounding looks more smooth with outline for some reason wtf */

		if ( input::mouse_in_region( exit_rect ) )
			draw_list.add_rounded_rect( exit_rect, 3.0f, color( style.window_accent.r, style.window_accent.g, style.window_accent.b, 0.2f ), true );

		if ( input::mouse_in_region( exit_rect ) && input::key_pressed( VK_LBUTTON ) )
			opened = false;

		draw_list.add_line( vec2( exit_rect.x + scale_dpi( exit_rect.w ) * 0.333f, exit_rect.y + scale_dpi( exit_rect.h ) * 0.333f ), vec2( exit_rect.x + scale_dpi( exit_rect.w ) * 0.666f, exit_rect.y + scale_dpi( exit_rect.h ) * 0.666f ), color( 1.0f, 1.0f, 1.0f, 1.0f ) );
		draw_list.add_line( vec2( exit_rect.x + scale_dpi( exit_rect.w ) * 0.333f, exit_rect.y + scale_dpi( exit_rect.h ) * 0.666f ), vec2( exit_rect.x + scale_dpi( exit_rect.w ) * 0.666f, exit_rect.y + scale_dpi( exit_rect.h ) * 0.333f ), color( 1.0f, 1.0f, 1.0f, 1.0f ) );
	}

	if ( window_entry->second.resizing ) {
		draw_list.add_arrow( vec2( window_entry->second.bounds.x + scale_dpi( window_entry->second.bounds.w - 6.0f ), window_entry->second.bounds.y + scale_dpi( window_entry->second.bounds.h - 6.0f ) ), 6.0f, -135.0f, style.window_accent, true, true );
	}

	window_entry->second.main_area = rect( window_entry->second.bounds.x + scale_dpi( style.initial_offset.x ), window_entry->second.bounds.y + scale_dpi( titlebar_rect.h + style.initial_offset.y ), window_entry->second.bounds.w - style.initial_offset.x * 2.0f, window_entry->second.bounds.h - titlebar_rect.h - style.initial_offset.y * 2.0f );
	window_entry->second.cursor_stack.push_back( vec2( window_entry->second.main_area.x, window_entry->second.main_area.y ) );

	return true;
}

void sesui::custom::end_window( ) {
	auto window_entry = globals::window_ctx.find( globals::cur_window );

	if ( window_entry == globals::window_ctx.end( ) )
		throw "Current window context not valid.";

	if ( disabled_input_for_subtab_click ) {
		sesui::input::enabled = true;
		disabled_input_for_subtab_click = false;
	}

	/* draw tooltip if we have one */
	if ( !window_entry->second.selected_tooltip.empty( ) ) {
		vec2 text_size;
		draw_list.get_text_size( style.control_font, window_entry->second.selected_tooltip.data( ), text_size );

		rect bounds = rect( input::mouse_pos.x - scale_dpi( style.padding ), input::mouse_pos.y - scale_dpi( style.padding ), unscale_dpi( text_size.x ) + style.padding * 2.0f, unscale_dpi( text_size.y ) + style.padding * 2.0f );

		bounds.y -= scale_dpi( bounds.h );

		draw_list.add_rounded_rect( bounds, style.rounding, color( style.window_foreground.r, style.window_foreground.g, style.window_foreground.b, 0.0f ).lerp( style.window_foreground, window_entry->second.tooltip_anim_time ), true, true );
		draw_list.add_rounded_rect( bounds, style.rounding, color( style.window_borders.r, style.window_borders.g, style.window_borders.b, 1.0f ).lerp( style.window_borders, window_entry->second.tooltip_anim_time ), false, true );
		draw_list.add_text( vec2( bounds.x + scale_dpi( style.padding ), bounds.y + scale_dpi( style.padding ) ), style.control_font, window_entry->second.selected_tooltip.data( ), false, color( style.control_text.r, style.control_text.g, style.control_text.b, 1.0f ).lerp( style.control_text, window_entry->second.tooltip_anim_time ), true );
	}
	else {
		window_entry->second.tooltip_anim_time = 0.0f;
	}

	window_entry->second.selected_tooltip.clear( );
	window_entry->second.cur_index = 0;
	window_entry->second.cur_tab_index = 0;
	window_entry->second.tab_count = 0;
	window_entry->second.cursor_stack.pop_back( );

	if ( !window_entry->second.cursor_stack.empty( ) )
		throw "Cursor stack was not empty at end of frame. Did you forget to call sesui::end_window or sesui::end_group?";

	globals::cur_window = L"";
}