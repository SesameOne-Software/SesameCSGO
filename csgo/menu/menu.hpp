#pragma once
#include <windows.h>
#include <cstdint>
#include <oxui.hpp>

namespace menu {
	long __stdcall wndproc( HWND hwnd, std::uint32_t msg, std::uintptr_t wparam, std::uint32_t lparam );

	bool open( );
	void* find_obj ( const oxui::str& item, oxui::object_type otype );

#define OPTION( type, object, item, type_name ) static auto& object = *( type* ) menu::find_obj( OSTR( item ), type_name )

	void load_default( );
	void init( );
	void destroy( );
	void reset( );
	void draw( );
}