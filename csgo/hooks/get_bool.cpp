#include "get_bool.hpp"

decltype( &hooks::get_int ) hooks::old::get_int = nullptr;

bool __fastcall hooks::get_int ( REG ) {
	static auto cam_think = pattern::search( _( "client.dll" ), _( "85 C0 75 30 38 86" ) ).get< void* >( );

	/*
	if ( cl_extrapolate.GetBool() && !engine->IsPaused() ) <-----------------
		context.EnableExtrapolation( true );
	*/
	static auto cl_extrapolate_ret = pattern::search( _( "client.dll" ), _( "85 C0 74 22 8B 0D ? ? ? ? 8B 01 8B" ) ).get< void* >( );
	static auto hermite_fix_ret = pattern::search( _( "client.dll" ), _( "0F B6 15 ? ? ? ? 85 C0" ) ).get< void* >( );
	static auto cl_interpolate_ret = pattern::search( _( "client.dll" ), _( "85 C0 BF ? ? ? ? 0F 95 C3" ) ).get< void* >( );

	if ( !ecx )
		return old::get_int ( REG_OUT );

	if ( _ReturnAddress( ) == cl_interpolate_ret || _ReturnAddress( ) == hermite_fix_ret || _ReturnAddress( ) == cl_extrapolate_ret )
		return 0;

	if ( _ReturnAddress( ) == cam_think )
		return 1;

	return old::get_int ( REG_OUT );
}