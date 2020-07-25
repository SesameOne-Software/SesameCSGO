#include "write_usercmd_delta_to_buffer.hpp"

#include "../features/ragebot.hpp"

decltype( &hooks::write_usercmd_delta_to_buffer ) hooks::old::write_usercmd_delta_to_buffer = nullptr;

bool __fastcall hooks::write_usercmd_delta_to_buffer ( REG, int slot, void* buf, int from, int to, bool new_cmd ) {
	static auto write_ucmd = pattern::search ( _ ( "client.dll" ), _ ( "55 8B EC 83 E4 F8 51 53 56 8B D9 8B 0D" ) ).get< void ( __fastcall* )( void*, ucmd_t*, ucmd_t* ) > ( );
	static auto cl_sendmove_ret = pattern::search ( _ ( "engine.dll" ), _ ( "84 C0 74 04 B0 01 EB 02 32 C0 8B FE 46 3B F3 7E C9 84 C0 0F 84" ) ).get< void* > ( );

	if ( _ReturnAddress ( ) != cl_sendmove_ret || g::dt_ticks_to_shift <= 0 ) {
		return old::write_usercmd_delta_to_buffer ( REG_OUT, slot, buf, from, to, new_cmd );
	}

	if ( from != -1 )
		return true;

	const auto new_commands = *reinterpret_cast< int* >( uintptr_t ( buf ) - 0x2C );
	const auto num_cmd = csgo::i::client_state->choked ( ) + 1;
	const auto next_cmd_num = csgo::i::client_state->last_outgoing_cmd ( ) + num_cmd;
	const auto total_new_cmds = std::clamp ( g::dt_ticks_to_shift, 0, csgo::is_valve_server ( ) ? 8 : 16 );

	from = -1;
	*reinterpret_cast< int* >( uintptr_t ( buf ) - 0x2C ) = total_new_cmds;
	*reinterpret_cast< int* >( uintptr_t ( buf ) - 0x30 ) = 0;

	for ( to = next_cmd_num - new_commands + 1; to <= next_cmd_num; to++ ) {
		if ( !old::write_usercmd_delta_to_buffer ( REG_OUT, slot, buf, from, to, true ) )
			return false;

		from = to;
	}

	const auto last_real_cmd = vfunc< ucmd_t * ( __thiscall* )( void*, int, int ) > ( csgo::i::input, 8 )( csgo::i::input, slot, from );
	ucmd_t cmd_from;

	if ( last_real_cmd )
		cmd_from = *last_real_cmd;

	ucmd_t cmd_to = cmd_from;
	cmd_to.m_cmdnum++;

	if ( features::ragebot::active_config.dt_teleport && !csgo::is_valve_server ( ) /* don't wanna set tickcount to int_max or else we will get banned on valve servers */ )
		cmd_to.m_tickcount++;
	else
		cmd_to.m_tickcount += 666;

	for ( auto i = new_commands; i <= total_new_cmds; i++ ) {
		write_ucmd ( buf, &cmd_to, &cmd_from );
		cmd_from = cmd_to;
		cmd_to.m_cmdnum++;
		cmd_to.m_tickcount++;
	}

	//if ( features::ragebot::active_config.dt_teleport )
	g::shifted_tickbase = cmd_to.m_cmdnum;
	g::shifted_amount = total_new_cmds;

	g::dt_ticks_to_shift = 0;
	g::next_tickbase_shot = true;

	return true;
}