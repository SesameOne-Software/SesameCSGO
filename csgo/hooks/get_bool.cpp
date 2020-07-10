#include "get_bool.hpp"

decltype( &hooks::get_bool ) hooks::old::get_bool = nullptr;

bool __fastcall hooks::get_bool ( REG ) {
	//static auto cl_interpolate = pattern::search( _( "client.dll" ), _( "85 C0 BF ? ? ? ? 0F 95 C3" ) ).get< void* >( );
	static auto cam_think = pattern::search ( _ ( "client.dll" ), _ ( "85 C0 75 30 38 86" ) ).get< void* > ( );
	//static auto hermite_fix = pattern::search( _( "client.dll" ), _( "0F B6 15 ? ? ? ? 85 C0" ) ).get< void* >( );
	//static auto cl_extrapolate = pattern::search ( _ ( "client.dll" ), _ ( "85 C0 74 22 8B 0D ? ? ? ? 8B 01 8B" ) ).get< void* > ( );
	//static auto color_static_props = pattern::search ( _ ( "engine.dll" ), _ ( "85 C0 0F 84 ? ? ? ? 8D 4B 32" ) ).get< void* > ( );
	//static auto color_static_props1 = pattern::search ( _ ( "engine.dll" ), _ ( "85 C0 75 74 8B 0D" ) ).get< void* > ( );

	if ( !ecx )
		return false;

	if ( _ReturnAddress ( ) == cam_think /*|| _ReturnAddress ( ) == hermite_fix || _ReturnAddress ( ) == hermite_fix || _ReturnAddress ( ) == cl_extrapolate*/ )
		return true;

	return old::get_bool ( REG_OUT );
}