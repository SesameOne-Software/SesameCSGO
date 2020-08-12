#include "is_hltv.hpp"

decltype( &hooks::is_hltv ) hooks::old::is_hltv = nullptr;

bool __fastcall hooks::is_hltv ( REG ) {
	static const auto accumulate_layers_call = pattern::search ( _ ( "client.dll" ), _ ( "84 C0 75 0D F6 87" ) ).get< void* > ( );
	static const auto setupvelocity_call = pattern::search ( _ ( "client.dll" ), _ ( "84 C0 75 38 8B 0D ? ? ? ? 8B 01 8B 80 ? ? ? ? FF D0" ) ).get< void* > ( );

	if ( !csgo::i::engine->is_in_game ( ) || !csgo::i::engine->is_connected ( ) )
		return old::is_hltv ( REG_OUT );

	if ( _ReturnAddress ( ) == accumulate_layers_call /*|| _ReturnAddress ( ) == setupvelocity_call*/ )
		return true;

	return old::is_hltv ( REG_OUT );
}