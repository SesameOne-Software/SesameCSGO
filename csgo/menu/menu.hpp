#pragma once
#include <windows.h>
#include <cstdint>
#include <oxui.hpp>

namespace menu {
	long __stdcall wndproc( HWND hwnd, std::uint32_t msg, std::uintptr_t wparam, std::uint32_t lparam );
	
	bool open( );
	void* find_obj( const oxui::str& tab_name, const oxui::str& group_name, const oxui::str& object_name, oxui::object_type otype );

#define FIND( type, object, tab_name, group_name, object_name, type_name ) static auto& object = *( type* ) menu::find_obj( OSTR( tab_name ), OSTR( group_name ), OSTR( object_name ), type_name )

	void load_default( );
	void init( );
	void destroy( );
	void reset( );
	void draw( );
}