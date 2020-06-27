#include "keybind.hpp"
#include "../panels/panel.hpp"
#include "shapes.hpp"
#include "../themes/purple.hpp"

#define make_str(x) case x: as_str = OSTR( #x ); break;

oxui::str key_to_str ( int vk ) {
	if ( vk <= 0 )
		return OSTR ( "-" );

	if ( vk >= '0' && vk <= '9' ) { return oxui::str ( 1, wchar_t ( vk ) ); }
	if ( vk >= 'A' && vk <= 'Z' ) { return oxui::str ( 1, wchar_t ( vk ) ); }

	oxui::str as_str;
	
	switch ( vk ) {
		make_str ( VK_LBUTTON );
		make_str ( VK_RBUTTON );
		make_str ( VK_CANCEL );
		make_str ( VK_MBUTTON );  
		make_str ( VK_XBUTTON1 ); 
		make_str ( VK_XBUTTON2 ); 
		make_str ( VK_BACK );
		make_str ( VK_TAB );
		make_str ( VK_CLEAR );
		make_str ( VK_RETURN );
		make_str ( VK_SHIFT );
		make_str ( VK_CONTROL );
		make_str ( VK_MENU );
		make_str ( VK_PAUSE );
		make_str ( VK_CAPITAL );
		make_str ( VK_KANA );
		make_str ( VK_JUNJA );
		make_str ( VK_FINAL );
		make_str ( VK_KANJI );
		make_str ( VK_ESCAPE );
		make_str ( VK_CONVERT );
		make_str ( VK_NONCONVERT );
		make_str ( VK_ACCEPT );
		make_str ( VK_MODECHANGE );
		make_str ( VK_SPACE );
		make_str ( VK_PRIOR );
		make_str ( VK_NEXT );
		make_str ( VK_END );
		make_str ( VK_HOME );
		make_str ( VK_LEFT );
		make_str ( VK_UP );
		make_str ( VK_RIGHT );
		make_str ( VK_DOWN );
		make_str ( VK_SELECT );
		make_str ( VK_PRINT );
		make_str ( VK_EXECUTE );
		make_str ( VK_SNAPSHOT );
		make_str ( VK_INSERT );
		make_str ( VK_DELETE );
		make_str ( VK_HELP );
		make_str ( VK_LWIN );
		make_str ( VK_RWIN );
		make_str ( VK_APPS );
		make_str ( VK_SLEEP );
		make_str ( VK_NUMPAD0 );
		make_str ( VK_NUMPAD1 );
		make_str ( VK_NUMPAD2 );
		make_str ( VK_NUMPAD3 );
		make_str ( VK_NUMPAD4 );
		make_str ( VK_NUMPAD5 );
		make_str ( VK_NUMPAD6 );
		make_str ( VK_NUMPAD7 );
		make_str ( VK_NUMPAD8 );
		make_str ( VK_NUMPAD9 );
		make_str ( VK_MULTIPLY );
		make_str ( VK_ADD );
		make_str ( VK_SEPARATOR );
		make_str ( VK_SUBTRACT );
		make_str ( VK_DECIMAL );
		make_str ( VK_DIVIDE );
		make_str ( VK_F1 );
		make_str ( VK_F2 );
		make_str ( VK_F3 );
		make_str ( VK_F4 );
		make_str ( VK_F5 );
		make_str ( VK_F6 );
		make_str ( VK_F7 );
		make_str ( VK_F8 );
		make_str ( VK_F9 );
		make_str ( VK_F10 );
		make_str ( VK_F11 );
		make_str ( VK_F12 );
		make_str ( VK_F13 );
		make_str ( VK_F14 );
		make_str ( VK_F15 );
		make_str ( VK_F16 );
		make_str ( VK_F17 );
		make_str ( VK_F18 );
		make_str ( VK_F19 );
		make_str ( VK_F20 );
		make_str ( VK_F21 );
		make_str ( VK_F22 );
		make_str ( VK_F23 );
		make_str ( VK_F24 );
		make_str ( VK_NUMLOCK );
		make_str ( VK_SCROLL );
		make_str ( VK_OEM_NEC_EQUAL );  
		make_str ( VK_OEM_FJ_MASSHOU ); 
		make_str ( VK_OEM_FJ_TOUROKU ); 
		make_str ( VK_OEM_FJ_LOYA );    
		make_str ( VK_OEM_FJ_ROYA );    
		make_str ( VK_LSHIFT );
		make_str ( VK_RSHIFT );
		make_str ( VK_LCONTROL );
		make_str ( VK_RCONTROL );
		make_str ( VK_LMENU );
		make_str ( VK_RMENU );
		make_str ( VK_BROWSER_BACK );
		make_str ( VK_BROWSER_FORWARD );
		make_str ( VK_BROWSER_REFRESH );
		make_str ( VK_BROWSER_STOP );
		make_str ( VK_BROWSER_SEARCH );
		make_str ( VK_BROWSER_FAVORITES );
		make_str ( VK_BROWSER_HOME );
		make_str ( VK_VOLUME_MUTE );
		make_str ( VK_VOLUME_DOWN );
		make_str ( VK_VOLUME_UP );
		make_str ( VK_MEDIA_NEXT_TRACK );
		make_str ( VK_MEDIA_PREV_TRACK );
		make_str ( VK_MEDIA_STOP );
		make_str ( VK_MEDIA_PLAY_PAUSE );
		make_str ( VK_LAUNCH_MAIL );
		make_str ( VK_LAUNCH_MEDIA_SELECT );
		make_str ( VK_LAUNCH_APP1 );
		make_str ( VK_LAUNCH_APP2 );
		make_str ( VK_OEM_1 );     
		make_str ( VK_OEM_PLUS );  
		make_str ( VK_OEM_COMMA ); 
		make_str ( VK_OEM_MINUS ); 
		make_str ( VK_OEM_PERIOD );
		make_str ( VK_OEM_2 ); 
		make_str ( VK_OEM_3 ); 
		make_str ( VK_OEM_4 ); 
		make_str ( VK_OEM_5 ); 
		make_str ( VK_OEM_6 ); 
		make_str ( VK_OEM_7 ); 
		make_str ( VK_OEM_8 );
		make_str ( VK_OEM_AX );  
		make_str ( VK_OEM_102 ); 
		make_str ( VK_ICO_HELP );
		make_str ( VK_ICO_00 );  
		make_str ( VK_PROCESSKEY );
		make_str ( VK_ICO_CLEAR );
		make_str ( VK_PACKET );
		default: return OSTR( "-" );
	}

	as_str.erase ( 0, 3 );

	auto char_idx = 0;

	for ( auto& character : as_str ) {
		if ( character == '_' ) {
			character = ' ';
			continue;
		}

		if ( character >= 65 && character <= 90 && !( !char_idx || as_str [ char_idx - 1 ] == ' ' ) )
			character = std::tolower ( character );

		char_idx++;
	}

	return as_str;
}

void oxui::keybind::think ( ) {
	window& parent_window = find_parent< window > ( object_window );
	pos& cursor_pos = parent_window.cursor_pos;
	animate ( rect ( cursor_pos.x, cursor_pos.y, area.w, area.h ) );

	if ( shapes::clicking ( rect ( cursor_pos.x, cursor_pos.y, area.w, area.h ), false, true ) && !opened_shortcut_menu )
		searching = true;

	if ( shapes::hovering ( rect ( cursor_pos.x, cursor_pos.y, area.w, area.h ), false, true ) && !utils::key_state ( VK_RBUTTON ) && last_rkey ) {
		opened_shortcut_menu = true;
		binds::mouse_pos ( rclick_pos );
	}

	last_rkey = utils::key_state ( VK_RBUTTON );

	if ( searching ) {
		for ( int i = 0; i < 255; i++ ) {
			if ( GetAsyncKeyState ( i ) ) {
				/* we can't bind left/right click */
				if ( i == VK_LBUTTON || i == VK_RBUTTON ) 
					continue;

				key = ( i == VK_ESCAPE ) ? -1 : i;
				shapes::finished_input_frame = searching = false;
			}
		}
	}

	if ( opened_shortcut_menu ) {
		std::vector< str > rclick_menu_items { OSTR ( "Hold" ), OSTR ( "Toggle" ), OSTR ( "Always" ) };

		/* background of the list */
		rect list_pos ( rclick_pos.x, rclick_pos.y, 100, theme.spacing );

		pos mouse_pos;
		binds::mouse_pos ( mouse_pos );

		hovered_index = 0;

		auto index = 0;
		auto selected = -1;

		for ( const auto& it : rclick_menu_items ) {
			const auto backup_input_clip_area = g_oxui_input_clip_area;

			/* ignore clipping */
			g_oxui_input_clip_area = false;

			/* check if we are clicking the thingy */
			if ( utils::key_state ( VK_LBUTTON ) && mouse_pos.x >= list_pos.x && mouse_pos.y >= list_pos.y && mouse_pos.x <= list_pos.x + list_pos.w && mouse_pos.y <= list_pos.y + list_pos.h ) {
				selected = index;

				/* remove if you want to close it manually instead of automatically (snek) */ {
					opened_shortcut_menu = false;
				}

				shapes::finished_input_frame = true;
				shapes::click_start = pos ( 0, 0 );
				g_input = true;
				searching = false;
			}
			else if ( mouse_pos.x >= list_pos.x && mouse_pos.y >= list_pos.y && mouse_pos.x <= list_pos.x + list_pos.w && mouse_pos.y <= list_pos.y + list_pos.h ) {
				hovered_index = index;
			}

			/* ignore clipping */
			g_oxui_input_clip_area = backup_input_clip_area;

			list_pos.y += theme.spacing;
			index++;
		}

		if ( selected != -1 ) {
			g_input = false;

			if ( selected == 2 )
				key = -1;
			
			mode = static_cast < keybind_mode > ( selected );
		}

		/* we clicked outside the right click menu, close */
		if ( utils::key_state ( VK_LBUTTON ) ) {
			pos mouse_pos;
			binds::mouse_pos ( mouse_pos );

			const auto hovered_area = mouse_pos.x >= list_pos.x && mouse_pos.y >= rclick_pos.y && mouse_pos.x <= list_pos.x + list_pos.w && mouse_pos.y <= list_pos.y;

			if ( !hovered_area ) {
				shapes::finished_input_frame = true;
				shapes::click_start = pos ( 0, 0 );
				g_input = true;
				searching = false;
				opened_shortcut_menu = false;
			}
		}
	}
}

void oxui::keybind::draw( ) {
	think( );

	auto& parent_panel = find_parent< panel >( object_panel );
	auto& parent_window = find_parent< window >( object_window );

	auto& font = parent_panel.fonts [ OSTR( "object" ) ];

	auto& cursor_pos = parent_window.cursor_pos;

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
	binds::rounded_rect ( { cursor_pos.x, cursor_pos.y, area.w, area.h }, 8, 16, searching ? color { 0, 0, 0, static_cast< int >( std::sin ( parent_panel.time * 2.0 * 3.141f ) * 60.0 + 60.0 ) } : color { 0, 0, 0, 90 }, true );

	auto text = key_to_str ( key );

	if ( !fade_alpha )
		text = label;

	if ( searching )
		text = OSTR ( "..." );

	rect text_size;
	binds::text_bounds( font, text, text_size );

	auto area_center_x = cursor_pos.x + area.w / 2;
	auto area_center_y = cursor_pos.y + theme.spacing / 2;

	/* centered text */
	binds::text ( pos ( area_center_x - text_size.w / 2 - 1, area_center_y - text_size.h / 2 - 1 ), font, text, lerp_color ( theme.text, { 56, 56, 56, 255 }, lerp_fraction ), false );

	/* draw rclick menu items */
	if ( opened_shortcut_menu ) {
		parent_window.draw_overlay ( [ = ] ( ) {
			std::vector< str > rclick_menu_items { OSTR ( "Hold" ), OSTR ( "Toggle" ), OSTR ( "Always" ) };

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
}