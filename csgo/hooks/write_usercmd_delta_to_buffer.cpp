#include "write_usercmd_delta_to_buffer.hpp"

#include "../features/ragebot.hpp"

decltype( &hooks::write_usercmd_delta_to_buffer ) hooks::old::write_usercmd_delta_to_buffer = nullptr;

class c_nc {
public:
	PAD( 23 );
	bool should_delete;
	int out_seq_nr;
	int in_seq_nr;
	int out_seq_nr_ack;
	int out_reliable_state;
	int in_reliable_state;
	int choked_packets;
};

bool __fastcall hooks::write_usercmd_delta_to_buffer( REG, int slot, void* buf, int from, int to, bool new_cmd ) {
	static auto write_ucmd = pattern::search( _( "client.dll" ), _( "55 8B EC 83 E4 F8 51 53 56 8B D9 8B 0D" ) ).get< void( __fastcall* )( void*, ucmd_t*, ucmd_t* ) >( );
	static auto cl_sendmove_ret = pattern::search( _( "engine.dll" ), _( "84 C0 74 04 B0 01 EB 02 32 C0 8B FE 46 3B F3 7E C9 84 C0 0F 84" ) ).get< void* >( );

	if ( /*_ReturnAddress( ) != cl_sendmove_ret ||*/ g::dt_ticks_to_shift <= 0 || !csgo::i::engine->is_in_game( ) || !csgo::i::engine->is_connected( ) || !g::local ) {
		return old::write_usercmd_delta_to_buffer( REG_OUT, slot, buf, from, to, new_cmd );
	}

	if ( from != -1 )
		return true;

	int* m_backup_commands = ( int* )( reinterpret_cast< uintptr_t >( buf ) - 0x30 );
	int* m_new_commands = ( int* )( reinterpret_cast< uintptr_t >( buf ) - 0x2C );

	const auto backup_new_commands = *m_new_commands;

	/* ez sit nn dog */
	//byte m_data [ 4000 ];
	//buf->start_writing( m_data, sizeof( m_data ) );

	int num_cmd = csgo::i::client_state->choked( ) + 1;
	int next_cmdnr = csgo::i::client_state->last_outgoing_cmd( ) + num_cmd;

	if ( num_cmd > 62 )
		num_cmd = 62;

	const auto shift_amount_clamped = std::clamp( g::dt_ticks_to_shift, 0, 16 );

	from = -1;

	for ( to = next_cmdnr - backup_new_commands + 1; to <= next_cmdnr; to++ ) {
		if ( !old::write_usercmd_delta_to_buffer( REG_OUT, slot, buf, from, to, true ) )
			return false;

		from = to;
	}

	auto get_ucmd = [ ] ( int m_slot, int m_from ) {
		typedef ucmd_t* ( __thiscall* o_fn )( void*, int, int );
		return ( *( o_fn** )csgo::i::input ) [ 8 ]( csgo::i::input, m_slot, m_from );
	};

	ucmd_t* m_lastrealcmd = get_ucmd( slot, from );
	ucmd_t m_fromcmd, m_tocmd;

	if ( m_lastrealcmd )
		m_fromcmd = *m_lastrealcmd;

	m_tocmd = m_fromcmd;
	m_tocmd.m_cmdnum++;
	m_tocmd.m_tickcount += static_cast< int > ( 3.0f * csgo::time2ticks( 1.0f ) );
	//m_tocmd.m_tickcount += 666;

	g::shifted_amount = g::dt_ticks_to_shift;

	for ( auto i = backup_new_commands; i <= shift_amount_clamped; i++ ) {
		write_ucmd( buf, &m_tocmd, &m_fromcmd );
		m_fromcmd = m_tocmd;
		m_tocmd.m_cmdnum++;
		m_tocmd.m_tickcount++;
	}

	//const auto nc = *reinterpret_cast< c_nc** >( uintptr_t( csgo::i::client_state ) + 0x9c );
//
	//if ( nc )
	//	nc->out_seq_nr += 666;

	g::dt_ticks_to_shift = 0;

	//if ( features::ragebot::active_config.dt_teleport )
	g::shifted_tickbase = m_tocmd.m_cmdnum;

	g::tickbase_at_shift = g::local->tick_base( );

	g::next_tickbase_shot = true;

	*m_new_commands = shift_amount_clamped;
	*m_backup_commands = 0;

	return true;
}