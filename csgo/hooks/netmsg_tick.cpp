#include "netmsg_tick.hpp"

decltype( &hooks::netmsg_tick ) hooks::old::netmsg_tick = nullptr;

bool __fastcall hooks::netmsg_tick ( REG, void* msg ) {
	g::server_tick = *reinterpret_cast< uint32_t*>( reinterpret_cast< uintptr_t >(msg) + 8 );

	return old::netmsg_tick ( REG_OUT, msg );
}