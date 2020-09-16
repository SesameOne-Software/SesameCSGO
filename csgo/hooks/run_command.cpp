#include "run_command.hpp"
#include "../globals.hpp"
#include "../features/prediction.hpp"
#include "create_move.hpp"
#include "../animations/animation_system.hpp"

decltype( &hooks::run_command ) hooks::old::run_command = nullptr;

void __fastcall hooks::run_command( REG, player_t* ent, ucmd_t* cmd, c_move_helper* move_helper ) {
	if ( !ent->valid( ) || ent != g::local )
		return old::run_command( REG_OUT, ent, cmd, move_helper );

	if ( vars::in_refresh ) {
		cmd->m_hasbeenpredicted = true;
		return;
	}

	auto backup_tb = ent->tick_base( );
	auto backup_curtime = csgo::i::globals->m_curtime;

	if ( g::shifted_tickbase == cmd->m_cmdnum ) {
		const auto latency_ticks = std::max<int>( 0, csgo::time2ticks( csgo::i::engine->get_net_channel_info( )->get_latency( 0 ) ) );

		//ent->tick_base( ) = g::tickbase_at_shift /*+ latency_ticks*/ - g::shifted_amount + 1;
		//ent->tick_base( )++;

		ent->tick_base( ) = features::prediction::shift( ent->tick_base( ) );

		csgo::i::globals->m_curtime = csgo::ticks2time( ent->tick_base( ) );
	}

	old::run_command( REG_OUT, ent, cmd, move_helper );

	/* TODO: run local animfix here */
	anims::animate_local ( );

	if ( g::shifted_tickbase == cmd->m_cmdnum ) {
		ent->tick_base( ) = backup_tb;
		csgo::i::globals->m_curtime = backup_curtime;
	}
}