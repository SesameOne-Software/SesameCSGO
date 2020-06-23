#ifndef OXUI_WINDOW_HPP
#define OXUI_WINDOW_HPP

#include <memory>
#include <vector>
#include <functional>
#include "object.hpp"
#include "../types/types.hpp"
#include "tab.hpp"
#include "groupbox.hpp"

namespace oxui {
	/*
	*	INFO: Container for control objects.
	*
	*	USAGE: const auto main_panel = std::make_shared< oxui::panel >( oxui::rect( 0, 0, screen.w, screen.h ) );
	*/
	class window : public obj {
		pos click_offset = pos ( );
		pos not_clicked_offset = pos( );
		str title;
		int toggle_bind = 0;
		std::vector < std::function< void ( ) > > overlay_func;
		bool render_overlay = true;

	public:
		bool open = false;
		double scroll_delta = 0.0;
		std::array< bool, 5 > mouse_down { false };
		std::array< bool, 512 > key_down { false };
		pos cursor_pos;
		std::function< void( wchar_t ) > keyboard_handler_func;
		bool handle_keyboard = true;
		bool pressing_move_key = false;
		bool pressing_open_key = false;
		std::vector< std::shared_ptr< obj > > objects;

		long __stdcall wndproc( HWND hwnd, std::uint32_t msg, std::uintptr_t wparam, std::uint32_t lparam ) {
			if ( open ) {
				switch ( msg ) {
				case WM_LBUTTONDOWN: case WM_LBUTTONDBLCLK:
				case WM_RBUTTONDOWN: case WM_RBUTTONDBLCLK:
				case WM_MBUTTONDOWN: case WM_MBUTTONDBLCLK:
				case WM_XBUTTONDOWN: case WM_XBUTTONDBLCLK: {
					int button = 0;
					if ( msg == WM_LBUTTONDOWN || msg == WM_LBUTTONDBLCLK ) { button = 0; }
					if ( msg == WM_RBUTTONDOWN || msg == WM_RBUTTONDBLCLK ) { button = 1; }
					if ( msg == WM_MBUTTONDOWN || msg == WM_MBUTTONDBLCLK ) { button = 2; }
					if ( msg == WM_XBUTTONDOWN || msg == WM_XBUTTONDBLCLK ) { button = ( GET_XBUTTON_WPARAM( wparam ) == XBUTTON1 ) ? 3 : 4; }
					mouse_down [ button ] = true;
					return true;
				}
				case WM_LBUTTONUP:
				case WM_RBUTTONUP:
				case WM_MBUTTONUP:
				case WM_XBUTTONUP: {
					int button = 0;
					if ( msg == WM_LBUTTONUP ) { button = 0; }
					if ( msg == WM_RBUTTONUP ) { button = 1; }
					if ( msg == WM_MBUTTONUP ) { button = 2; }
					if ( msg == WM_XBUTTONUP ) { button = ( GET_XBUTTON_WPARAM( wparam ) == XBUTTON1 ) ? 3 : 4; }
					mouse_down [ button ] = false;
					return true;
				}
				case WM_KEYDOWN:
				case WM_SYSKEYDOWN:
					if ( wparam < 256 )
						key_down [ wparam ] = true;
					return true;
				case WM_KEYUP:
				case WM_SYSKEYUP:
					if ( wparam < 256 )
						key_down [ wparam ] = false;
					return true;
				case WM_CHAR:
					if ( wparam > 0 && wparam < ( 1 << 16 ) && handle_keyboard )
						keyboard_handler_func( wchar_t( wparam ) );
					return true;
				case WM_MOUSEWHEEL:
					scroll_delta += GET_WHEEL_DELTA_WPARAM( wparam ) > 0 ? -1.0 : 1.0;
					return true;
				}

				return true;
			}

			return false;
		}

		window( const rect& area, const str& title ) {
			this->area = area;
			this->title = title;
			type = object_window;
		}
		~window( ) {}

		void bind_key( int key ) {
			toggle_bind = key;
		}

		void add_element( const std::shared_ptr< obj >& new_obj ) {
			new_obj->parent = this;
			objects.push_back( new_obj );
		}

		void add_tab( const std::shared_ptr< tab >& new_tab ) {
			add_element( new_tab );
		}

		void add_group( const std::shared_ptr< group >& new_group ) {
			add_element( new_group );
		}

		void think( );
		void draw( ) override;

		void draw_overlay( const std::function< void( ) >& overlay_renderer ) {
			overlay_func.push_back ( overlay_renderer );
			render_overlay = true;
		}

		void* find_obj( const str& option, object_type type );

		void save_state( const str& file );
		void load_state( const str& file );
	};
}

#endif // OXUI_WINDOW_HPP