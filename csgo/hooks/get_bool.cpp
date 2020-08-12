#include "get_bool.hpp"

decltype( &hooks::get_bool ) hooks::old::get_bool = nullptr;

bool __fastcall hooks::get_bool ( REG ) {
	static auto cam_think = pattern::search ( _ ( "client.dll" ), _ ( "85 C0 75 30 38 86" ) ).get< void* > ( );

	if ( !ecx )
		return false;

	if ( _ReturnAddress ( ) == cam_think )
		return true;

	return old::get_bool ( REG_OUT );
}