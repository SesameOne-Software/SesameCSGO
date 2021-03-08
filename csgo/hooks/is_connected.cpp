#include "is_connected.hpp"

decltype( &hooks::is_connected ) hooks::old::is_connected = nullptr;

bool __fastcall hooks::is_connected( REG ) {
	static std::uintptr_t loadout_allowed_return = ( pattern::search( _( "client.dll" ) , _( "75 04 B0 01 5F" ) ) ).sub( 0x2 ).get<uintptr_t>( );

	if ( reinterpret_cast< uintptr_t >( _ReturnAddress( ) ) == loadout_allowed_return )
		return false;

	return old::is_connected( REG_OUT );
}