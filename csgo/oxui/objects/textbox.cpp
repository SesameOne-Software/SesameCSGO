#include "textbox.hpp"
#include "../panels/panel.hpp"
#include "shapes.hpp"
#include "../themes/purple.hpp"

void oxui::textbox::handle_input( wchar_t key ) {
	auto& parent_window = find_parent< window >( object_window );

	switch ( key ) {
	case VK_TAB:
	case VK_ESCAPE:
	case VK_RETURN: /* anything that will toggle selection */
		g_input = true;
		opened = false;
		opened_shortcut_menu = false;
		parent_window.handle_keyboard = false;
		return;
	case VK_BACK: /* backspace */
		if ( !buf.empty( ) ) buf.pop_back( );
		return;
	default: /* handle text input */ {
		/* keyboard shortcuts */
		if ( ( parent_window.key_down [ VK_CONTROL ] || parent_window.key_down [ VK_LCONTROL ] ) && parent_window.key_down [ 'V' ] ) {
			HGLOBAL global = nullptr;

			OpenClipboard( nullptr );

			if ( global = GetClipboardData( CF_UNICODETEXT ) ) {
				auto wglobal = ( wchar_t* ) GlobalLock( global );

				if ( wglobal && GlobalSize( global ) ) {
					buf += str( wglobal + 0, wglobal + GlobalSize( global ) / 2 - 1 );

					if ( buf.length( ) >= len_max )
						buf.erase( len_max );
				}

				GlobalUnlock( global );
			}

			CloseClipboard( );

			return;
		}

		if ( buf.length( ) < len_max )
			buf.push_back( key );
	} return;
	}
}

void oxui::textbox::think( ) {
	area.h = theme.spacing * 2 + theme.spacing / 2;

	auto& parent_panel = find_parent< panel >( object_panel );
	auto& parent_window = find_parent< window >( object_window );
	auto& cursor_pos = parent_window.cursor_pos;
	animate( rect( cursor_pos.x, cursor_pos.y, area.w, area.h ) );

	if ( shapes::clicking( rect( cursor_pos.x, cursor_pos.y, area.w, area.h ), false, opened ) && !opened_shortcut_menu && g_input ) {
		g_input = ( opened = !opened );
	}

	/* check if clicking outside of area (disable input)*/
	if ( !shapes::hovering( rect( cursor_pos.x, cursor_pos.y, area.w, area.h ) ) && parent_window.key_down [ VK_LBUTTON ] && !opened_shortcut_menu ) {
		g_input = true;
		opened = false;
		parent_window.handle_keyboard = false;
	}

	if ( shapes::hovering( rect( cursor_pos.x, cursor_pos.y, area.w, area.h ), false, true ) && !GetAsyncKeyState( VK_RBUTTON ) && last_rkey ) {
		opened_shortcut_menu = true;
		binds::mouse_pos( rclick_pos );
	}

	last_rkey = GetAsyncKeyState( VK_RBUTTON );

	if ( opened ) {
		/* set input handler */
		parent_window.keyboard_handler_func = [ & ] ( wchar_t key ) { handle_input( key ); };
		parent_window.handle_keyboard = true;

		/* set animation */
		fade_timer = theme.animation_speed / 2.0;
		end_hover_time = parent_panel.time;
		fading_animation_timer = theme.animation_speed / 2.0;
	}

	if ( opened_shortcut_menu ) {
		std::vector< str > rclick_menu_items { OSTR( "Paste" ), OSTR( "Copy" ), OSTR( "Cut" ), OSTR( "Erase" ), OSTR( "Close" ) };

		/* background of the list */
		rect list_pos( rclick_pos.x, rclick_pos.y, 100, theme.spacing );

		hovered_index = 0;

		auto index = 0;
		auto selected = -1;

		for ( const auto& it : rclick_menu_items ) {
			const auto backup_input_clip_area = g_oxui_input_clip_area;

			/* ignore clipping */
			g_oxui_input_clip_area = false;

			/* check if we are clicking the thingy */
			if ( shapes::clicking( list_pos, false, true ) ) {
				selected = index;

				/* remove if you want to close it manually instead of automatically (snek) */ {
					opened_shortcut_menu = false;
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

		if ( selected != -1 ) {
			/* re-open (it will try to close itself) */
			g_input = false;
			opened = true;

			switch ( selected + 1 ) {
			case 0:
				if ( !buf.empty( ) )
					buf.erase( );
				break;
			case 1: {
				HGLOBAL global = nullptr;

				OpenClipboard( nullptr );

				if ( global = GetClipboardData( CF_UNICODETEXT ) ) {
					auto wglobal = ( wchar_t* ) GlobalLock( global );

					if ( wglobal && GlobalSize( global ) ) {
						buf += str( wglobal + 0, wglobal + GlobalSize( global ) / 2 - 1 );

						if ( buf.length( ) >= len_max )
							buf.erase( len_max );
					}

					GlobalUnlock( global );
				}

				CloseClipboard( );
			} break;
			case 2: {
				if ( !buf.empty( ) ) {
					HGLOBAL global = nullptr;

					HGLOBAL mem = GlobalAlloc( GMEM_MOVEABLE, buf.length( ) * sizeof( wchar_t ) + 1 );
					memcpy( GlobalLock( mem ), buf.data( ), buf.length( ) * sizeof( wchar_t ) + 1 );
					GlobalUnlock( mem );

					OpenClipboard( nullptr );

					EmptyClipboard( );
					SetClipboardData( CF_UNICODETEXT, mem );

					CloseClipboard( );
				}
			} break;
			case 3: {
				if ( !buf.empty( ) ) {
					HGLOBAL global = nullptr;

					HGLOBAL mem = GlobalAlloc( GMEM_MOVEABLE, buf.length( ) * sizeof( wchar_t ) + 1 );
					memcpy( GlobalLock( mem ), buf.data( ), buf.length( ) * sizeof( wchar_t ) + 1 );
					GlobalUnlock( mem );

					OpenClipboard( nullptr );

					EmptyClipboard( );
					SetClipboardData( CF_UNICODETEXT, mem );

					CloseClipboard( );

					if ( !buf.empty( ) )
						buf.erase( );
				}
			} break;
			case 4:
				buf.erase ( );
				break;
			case 5:
				break;
			}
		}
	}
}

void oxui::textbox::draw( ) {
	think( );

	auto& parent_panel = find_parent< panel >( object_panel );
	auto& parent_window = find_parent< window >( object_window );

	auto& font = parent_panel.fonts [ OSTR( "object" ) ];

	auto& cursor_pos = parent_window.cursor_pos;

	rect text_size;
	binds::text_bounds( font, label, text_size );

	auto input_text = buf;

	/* replace visible characters with stars (*)*/
	if ( hide_input ) {
		input_text.erase( );
		std::for_each( buf.begin( ), buf.end( ), [ & ] ( wchar_t character ) { input_text += OSTR( "●" ); } );
	}

	const auto fade_alpha = fade_timer > theme.animation_speed ? 90 : int ( fade_timer * ( 1.0 / theme.animation_speed ) * 90.0 );

	/* highlighted selection */
	if ( fade_alpha ) {
		binds::rounded_rect ( { cursor_pos.x, cursor_pos.y, area.w, area.h }, 8, 16, { 0, 0, 0, fade_alpha }, false );
		binds::rounded_rect ( { cursor_pos.x, cursor_pos.y, area.w, area.h }, 8, 16, { 0, 0, 0, fade_alpha }, true );
	}

	/* highlighted selection */
	binds::rounded_rect ( { cursor_pos.x, cursor_pos.y + theme.spacing, area.w, theme.spacing }, 8, 16, { 0, 0, 0, fade_alpha }, false );
	binds::rounded_rect ( { cursor_pos.x, cursor_pos.y + theme.spacing, area.w, theme.spacing }, 8, 16, { 0, 0, 0, fade_alpha }, true );

	binds::text( pos( cursor_pos.x + theme.spacing / 2, cursor_pos.y + theme.spacing / 2 - text_size.h / 2 - 1 ), font, label, theme.text, false );
	binds::text( pos( cursor_pos.x + theme.spacing / 2, cursor_pos.y + theme.spacing / 2 - text_size.h / 2 - 1 + theme.spacing ), font, input_text, theme.text, false );

	/* input cursor */
	if ( opened ) {
		rect input_size;
		binds::text_bounds( font, input_text, input_size );
		binds::line( pos( cursor_pos.x + theme.spacing / 2 + input_size.w + 2, cursor_pos.y + theme.spacing / 2 + theme.spacing - theme.spacing / 3 ), pos( cursor_pos.x + theme.spacing / 2 + input_size.w + 2, cursor_pos.y + theme.spacing / 2 + theme.spacing + theme.spacing / 3 ), color( theme.text.r, theme.text.g, theme.text.b, static_cast< int >( std::sin( parent_panel.time * 2.0 * 3.141f ) * 127.5 + 127.5 ) ) );
	}

	/* draw rclick menu items */
	if ( opened_shortcut_menu ) {
		parent_window.draw_overlay( [ = ] ( ) {
			std::vector< str > rclick_menu_items { OSTR( "Paste" ), OSTR( "Copy" ), OSTR( "Cut" ), OSTR( "Erase" ), OSTR( "Close" ) };

			/* render the items name */
			auto index = 0;

			/* background of the list */
			binds::rounded_rect ( { rclick_pos.x, rclick_pos.y, 100, theme.spacing + 8 }, 8, 16, hovered_index == index ? theme.bg : theme.container_bg, false );
			binds::rounded_rect ( { rclick_pos.x, rclick_pos.y + theme.spacing * ( static_cast< int > ( rclick_menu_items.size ( ) ) - 1 ) - 8, 100, theme.spacing + 8 }, 8, 16, hovered_index == ( rclick_menu_items.size ( ) - 1 ) ? theme.bg : theme.container_bg, false );

			for ( const auto& it : rclick_menu_items ) {
				/* get the text size */
				rect item_text_size;
				binds::text_bounds ( font, it, item_text_size );

				/* render the square background if not first or last (middle of rounded rectangles) */
				if ( index && index != rclick_menu_items.size ( ) - 1 )
					binds::fill_rect ( { rclick_pos.x, rclick_pos.y + theme.spacing * index, 100, theme.spacing }, hovered_index == index ? theme.bg : theme.container_bg );

				/* render the name */
				binds::text ( { rclick_pos.x + 50 - item_text_size.w / 2 - 1, rclick_pos.y + theme.spacing * index + theme.spacing / 2 - item_text_size.h / 2 }, font, it, hovered_index == index ? theme.main : theme.text, hovered_index == index );

				index++;
			}

			/* outline of the list */
			binds::rounded_rect ( { rclick_pos.x, rclick_pos.y, 100, theme.spacing * static_cast< int > ( rclick_menu_items.size ( ) ) }, 8, 16, { 0, 0, 0, 90 }, true );
			} );
	}

	cursor_pos.y += theme.spacing + theme.spacing / 2;
}