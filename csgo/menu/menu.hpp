#pragma once
#include <windows.h>
#include <cstdint>
#include <oxui.hpp>

namespace menu {
	long __stdcall wndproc( HWND hwnd, std::uint32_t msg, std::uintptr_t wparam, std::uint32_t lparam );

	bool open( );
	void* find_obj ( const oxui::str& item, oxui::object_type otype );

#define OPTION( type, object, item, type_name ) static auto& object = *( type* ) menu::find_obj( OSTR( item ), type_name )

#define _MAKE_KEYBIND( n ) keybind_obj_##n
#define _MAKE_KEYBIND_TOGGLE( n ) keybind_toggle_##n
#define MAKE_KEYBIND( n ) _MAKE_KEYBIND( n )
#define MAKE_KEYBIND_TOGGLE( n ) _MAKE_KEYBIND_TOGGLE( n )
#define KEYBIND( object, item ) \
	static auto MAKE_KEYBIND( object ) = reinterpret_cast < oxui::keybind* > ( menu::find_obj( OSTR( item ), oxui::object_keybind ) ); \
	static auto MAKE_KEYBIND_TOGGLE( object ) = false; \
	static auto object = false; \
	switch ( MAKE_KEYBIND( object )->mode ) { \
		case oxui::keybind_mode::hold: { object = MAKE_KEYBIND( object )->key != -1 && utils::key_state( MAKE_KEYBIND( object )->key ); } break; \
		case oxui::keybind_mode::toggle: { \
			if ( MAKE_KEYBIND( object )->key != -1 ) { \
				if ( MAKE_KEYBIND_TOGGLE( object ) && !utils::key_state ( MAKE_KEYBIND( object )->key ) ) \
					object = !object; \
				MAKE_KEYBIND_TOGGLE( object ) = utils::key_state ( MAKE_KEYBIND( object )->key ); \
			} \
			else object = false; \
		} break; \
		case oxui::keybind_mode::always: { object = true; } break; \
	}

	void load_default( );
	void init( );
	void destroy( );
	void reset( );
	void draw( );
}