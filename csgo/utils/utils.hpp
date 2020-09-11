#pragma once
#include <array>
#include "detours.hpp"
#include "patternscanner.hpp"
#include "vfunc.hpp"
#include "padding.hpp"
#include "registers.hpp"
#include "../security/xorstr.hpp"

extern std::array< bool, 5 > mouse_down;
extern std::array< bool, 512 > key_down;
extern std::array< bool, 512 > key_toggled;
extern std::array< bool, 512 > last_key_toggled;

namespace utils {
	__forceinline bool key_state( int vkey ) {
		switch ( vkey ) {
			case VK_LBUTTON: return mouse_down [ 0 ];
			case VK_RBUTTON: return mouse_down [ 1 ];
			case VK_MBUTTON: return mouse_down [ 2 ];
			case VK_XBUTTON1: return mouse_down [ 3 ];
			case VK_XBUTTON2: return mouse_down [ 4 ];
		}

		return key_down [ vkey ];
	}

	__forceinline bool last_key_state( int vkey ) {
		return last_key_toggled [ vkey ];
	}

	__forceinline void update_key_toggles( ) {
		for ( auto i = 0; i < 512; i++ ) {
			if ( !key_state( i ) && last_key_state( i ) )
				key_toggled [ i ] = !key_toggled [ i ];

			last_key_toggled [ i ] = key_state( i );
		}
	}

	__forceinline bool keybind_active( int vkey, int mode ) {
		switch ( mode ) {
			case 0: return vkey && key_state( vkey ); break;
			case 1: return vkey && key_toggled [ vkey ]; break;
			case 2: return true; break;
		}

		return false;
	}
}

static void dbg_print( const char* msg, ... ) {
	if ( !msg )
		return;

	static void( __cdecl * msg_fn )( const char*, va_list ) = ( decltype( msg_fn ) )( LI_FN( GetProcAddress )( LI_FN( GetModuleHandleA )( _( "tier0.dll" ) ), _( "Msg" ) ) ); //This gets the address of export "Msg" in the dll "tier0.dll". The static keyword means it's only called once and then isn't called again (but the variable is still there)
	char buffer [ 989 ];
	va_list list;
	va_start( list, msg );
	vsprintf( buffer, msg, list );
	va_end( list );
	msg_fn( buffer, list );
}

#define _CALL_SAFE_TO_STR( s ) __CALL_SAFE_TO_STR( s )
#define __CALL_SAFE_TO_STR( s ) #s

#define RUN_SAFE( label, code ) \
	code
	//try { \
	//	code \
	//} \
	//catch ( const std::exception & ex ) { \
	//	dbg_print( _( "Caught exception in %s: %s" ), _( label ), ex.what( ) ); \
	//}