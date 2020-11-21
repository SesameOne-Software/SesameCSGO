#include "cl_fireevents.hpp"

decltype( &hooks::cl_fireevents ) hooks::old::cl_fireevents = nullptr;

void __stdcall hooks::cl_fireevents ( ) {
	static auto event_off = pattern::search ( _("engine.dll"), _("8B BB ? ? ? ? 85 FF 0F 84") ).add(2).deref().get<uint32_t>();
	
	if( cs::i::client_state )
		for ( auto ei = *reinterpret_cast< void** >( uintptr_t ( cs::i::client_state ) + event_off ); ei; ei = *reinterpret_cast< void** >( uintptr_t ( ei ) + 56 ) )
			*reinterpret_cast< float* >( uintptr_t ( ei ) + 4 ) = 0.0f;

	old::cl_fireevents ( );
}