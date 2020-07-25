#include "run_command.hpp"
#include "../globals.hpp"
#include "../features/prediction.hpp"
#include "create_move.hpp"

decltype( &hooks::run_command ) hooks::old::run_command = nullptr;

void __fastcall hooks::run_command ( REG, player_t* ent, ucmd_t* cmd, c_move_helper* move_helper ) {
	if ( !ent->valid ( ) || ent != g::local )
		return old::run_command ( REG_OUT, ent, cmd, move_helper );

	if ( vars::in_refresh ) {
		cmd->m_hasbeenpredicted = true;
		return;
	}

	auto backup_tb = ent->tick_base ( );

	if ( g::shifted_tickbase == cmd->m_cmdnum )
		g::local->tick_base ( ) = features::prediction::shift ( g::local->tick_base ( ) );

	old::run_command ( REG_OUT, ent, cmd, move_helper );

	// local_anims.run_command ( cmd );

	if ( g::shifted_tickbase == cmd->m_cmdnum ) {
		g::local->tick_base ( ) = backup_tb + 1;
		csgo::i::globals->m_curtime = g::local->tick_base ( ) * csgo::i::globals->m_ipt;
	}
}