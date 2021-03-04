#include "run_simulation.hpp"

#include "../animations/resolver.hpp"

#include "../animations/anims.hpp"
#include "../features/exploits.hpp"
#include "../features/prediction.hpp"

decltype( &hooks::run_simulation ) hooks::old::run_simulation = nullptr;

namespace lby {
	extern bool in_update;
}

int old_tickbase = 0;

/* cbrs */
void __fastcall hooks::run_simulation ( REG, int current_command, ucmd_t* cmd, player_t* localplayer ) {
	if ( !localplayer || localplayer != g::local || !g::local )
		return old::run_simulation ( REG_OUT, current_command, cmd, localplayer );

	if ( exploits::is_recharging() ) {
		MUTATE_START
		cmd->m_hasbeenpredicted = true;
		localplayer->tick_base ( )++;
		localplayer->set_abs_origin( localplayer->origin() );
		MUTATE_END
		return;
	}

	MUTATE_START
		const auto backup_tick_base = localplayer->tick_base( );

	if ( exploits::shifted_command ( ) == current_command )
		localplayer->tick_base ( ) -= exploits::shifted_tickbase ( );

	auto curtime = cs::i::globals->m_curtime = cs::ticks2time ( localplayer->tick_base ( ) );
	__asm movss xmm2, curtime

	//if ( cs::i::client_state ) {
	//	if ( current_command == cs::i::client_state->last_command_ack( ) + 1 ) {
	//		if ( !features::prediction::fix_netvars( current_command ) ) /* needs to repredict */ {
	//			*reinterpret_cast< bool* >( reinterpret_cast< uintptr_t >( cs::i::pred ) + 0x24 ) = true;
	//		}
	//
	//		//dbg_print( _( "REPREDICTING.\n" ) );
	//	}
	//	else {
	//		if ( features::prediction::fix_netvars( current_command, true ) ) {
	//			*reinterpret_cast< bool* >( reinterpret_cast< uintptr_t >( cs::i::pred ) + 0x24 ) = true;
	//		}
	//	}
	//}

	// backup
	//features::prediction::fix_netvars( current_command );

	old::run_simulation ( REG_OUT, current_command, cmd, localplayer );

	// restore

	if ( exploits::shifted_command ( ) == current_command )
		localplayer->tick_base ( ) = backup_tick_base;

	//anims::update_anims( localplayer , lby::in_update ? g::sent_cmd.m_angs : g::angles , false );

	features::prediction::fix_viewmodel ( true );
	MUTATE_END
}