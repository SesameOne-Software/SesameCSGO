#pragma once
#include "../utils/vfunc.hpp"

class c_surface {
public:
	void unlock_cursor( void ) {
		using unlockcusor_fn = void( __thiscall* )( void* );
		vfunc< unlockcusor_fn >( this, 66 )( this );
	}

	void lock_cursor( void ) {
		using lockcusor_fn = void( __thiscall* )( void* );
		vfunc< lockcusor_fn >( this, 67 )( this );
	}

	void get_cursor_pos( int& x, int& y ) {
		using get_cursor_pos_fn = void( __thiscall* )( void*, int&, int& );
		vfunc< get_cursor_pos_fn >( this, 100 )( this, x, y );
	}
};

class c_panel {
public:
	const char* get_name ( int ipanel ) {
		using getname_fn = const char* ( __thiscall* )( void*, int );
		return vfunc< getname_fn > ( this, 36 )( this, ipanel );
	}
};