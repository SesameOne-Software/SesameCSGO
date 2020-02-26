#ifndef OXUI_WINDOW_HPP
#define OXUI_WINDOW_HPP

#include <memory>
#include <vector>
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
		pos click_offset = pos( );
		bool pressing_move_key = false;
		bool pressing_open_key = false;
		str title;
		int toggle_bind = 0;
		std::vector< std::shared_ptr< obj > > objects;

	public:
		bool open = true;
		double scroll_delta = 0.0;
		pos cursor_pos;

		long __stdcall wndproc( HWND hwnd, std::uint32_t msg, std::uintptr_t wparam, std::uint32_t lparam ) {
			if ( open ) {
				switch ( msg ) {
				case WM_MOUSEWHEEL:
					scroll_delta += GET_WHEEL_DELTA_WPARAM( wparam ) > 0 ? -1.0 : 1.0;
					return 0;
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

		void* find_obj( const str& tab_name, const str& group_name, const str& object_name, object_type type );

		void save_state( const str& file );
		void load_state( const str& file );
	};
}

#endif // OXUI_WINDOW_HPP