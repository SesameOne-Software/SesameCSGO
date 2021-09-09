#include "get_bool.hpp"

decltype( &hooks::get_int ) hooks::old::get_int = nullptr;

bool __fastcall hooks::get_int ( REG ) {
	static auto cam_think = pattern::search( _( "client.dll" ), _( "85 C0 75 30 38 86" ) ).get< void* >( );
	static auto net_earliertempents_fireevents = pattern::search(_("engine.dll"), _("85 C0 74 05 E8 ? ? ? ? 84 DB")).get< void* >();

	if ( !ecx )
		return old::get_int ( REG_OUT );


	if ( _ReturnAddress ( ) == cam_think
		|| _ReturnAddress ( ) == net_earliertempents_fireevents )
		return 1;

	return old::get_int ( REG_OUT );
}