#include "run_command.hpp"
#include "../globals.hpp"
#include "../features/prediction.hpp"
#include "create_move.hpp"
#include "../animations/animation_system.hpp"

#include "../features/exploits.hpp"

decltype( &hooks::run_command ) hooks::old::run_command = nullptr;

void __fastcall hooks::run_command( REG, player_t* ent, ucmd_t* cmd, c_move_helper* move_helper ) {
	if ( !ent->valid( ) || ent != g::local )
		return old::run_command( REG_OUT, ent, cmd, move_helper );

	if ( exploits::is_recharging() ) {
		cmd->m_hasbeenpredicted = true;
		return;
	}

	const auto backup_tickbase = g::local->tick_base ( );

	if ( cmd->m_cmdnum == exploits::shifted_command ( ) )
		g::local->tick_base ( ) = exploits::shifted_tickbase ( );

	old::run_command( REG_OUT, ent, cmd, move_helper );

	if ( cmd->m_cmdnum == exploits::shifted_command ( ) )
		g::local->tick_base ( ) = backup_tickbase;

	/* TODO: run local animfix here */
	anims::animate_local ( );
}