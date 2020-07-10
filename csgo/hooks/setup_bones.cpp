#include "setup_bones.hpp"

decltype( &hooks::setup_bones ) hooks::old::setup_bones = nullptr;

bool __fastcall hooks::setup_bones ( REG, matrix3x4_t* out, int max_bones, int mask, float curtime ) {
	static auto setup_bones_ret = pattern::search ( _ ( "client.dll" ), _ ( "E8 ? ? ? ? 5E 5D C2 10 00 32 C0" ) ).add ( 5 ).get< int* > ( );

	if ( _ReturnAddress ( ) == setup_bones_ret )
		return false;

	return old::setup_bones ( REG_OUT, out, max_bones, mask, curtime );
}