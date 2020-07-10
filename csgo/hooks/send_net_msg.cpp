#include "send_net_msg.hpp"

decltype( &hooks::send_net_msg ) hooks::old::send_net_msg = nullptr;

bool __fastcall hooks::send_net_msg ( REG, void* msg, bool force_reliable, bool voice ) {
	if ( vfunc < int ( __thiscall* )( void* ) > ( msg, 8 )( msg ) == 9 )
		voice = true;

	return old::send_net_msg ( REG_OUT, msg, force_reliable, voice );
}