#include "write_usercmd_delta_to_buffer.hpp"

#include "../features/ragebot.hpp"

decltype( &hooks::write_usercmd_delta_to_buffer ) hooks::old::write_usercmd_delta_to_buffer = nullptr;

/* remove choke limit */
bool __fastcall hooks::write_usercmd_delta_to_buffer( REG, int slot, void* buf, int from, int to, bool new_cmd ) {
	static auto CL_SendMove_ret_addr = pattern::search ( _ ( "engine.dll" ), _ ( "84 C0 74 04 B0 01 EB 02 32 C0 8B FE" ) ).get < void* > ( );

	/* in CL_SendMove call */
	if ( _ReturnAddress ( ) == CL_SendMove_ret_addr ) {

	}

	return old::write_usercmd_delta_to_buffer ( REG_OUT, slot, buf, from, to, new_cmd );
}