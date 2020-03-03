#pragma once
#include "detours.hpp"
#include "patternscanner.hpp"
#include "vfunc.hpp"
#include "padding.hpp"
#include "registers.hpp"
#include "../security/xorstr.hpp"

static void dbg_print( const char* msg, ... ) {
	if ( !msg )
		return;

	static void( __cdecl * msg_fn )( const char*, va_list ) = ( decltype( msg_fn ) ) ( LI_FN( GetProcAddress )( LI_FN( GetModuleHandleA )( _( "tier0.dll" ) ), _( "Msg" ) ) ); //This gets the address of export "Msg" in the dll "tier0.dll". The static keyword means it's only called once and then isn't called again (but the variable is still there)
	char buffer [ 989 ];
	va_list list;
	va_start( list, msg );
	vsprintf( buffer, msg, list );
	va_end( list );
	msg_fn( buffer, list );
}