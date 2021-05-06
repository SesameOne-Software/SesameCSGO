#include "run_simulation.hpp"

#include "../animations/resolver.hpp"

#include "../animations/anims.hpp"
#include "../features/exploits.hpp"
#include "../features/prediction.hpp"

#undef min
#undef max

decltype( &hooks::run_simulation ) hooks::old::run_simulation = nullptr;

extern bool in_cm;

namespace lby {
	extern bool in_update;
}

int last_cmd_num = 0;

void __fastcall hooks::run_simulation ( REG, int current_command, ucmd_t* cmd, player_t* localplayer ) {
	if ( !g::local || !g::local->alive ( ) )
		last_cmd_num = 0;

	if ( !localplayer || localplayer != g::local || !g::local )
		return old::run_simulation ( REG_OUT, current_command, cmd, localplayer );

	if ( cmd->m_tickcount > cs::i::globals->m_tickcount + cs::time2ticks ( 1.0f ) ) {
		MUTATE_START;
		cmd->m_hasbeenpredicted = true;
		localplayer->tick_base ( )++;
		MUTATE_END;
		return;
	}

	MUTATE_START;
	const auto backup_vel_mod = localplayer->velocity_modifier ( );
	const auto backup_curtime = cs::i::globals->m_curtime;
	const auto backup_tickbase = localplayer->tick_base ( );

	if ( cs::i::client_state && in_cm && current_command == cs::i::client_state->last_command_ack() + 1 ) {
		localplayer->velocity_modifier ( ) = features::prediction::vel_modifier;
	}

	if ( current_command == exploits::shifted_command ( ) ) {
		localplayer->tick_base ( ) -= exploits::last_shifted_amount ( );
		localplayer->tick_base ( )++;
	}

	auto curtime = cs::i::globals->m_curtime = cs::ticks2time ( localplayer->tick_base ( ) );
	__asm movss xmm2, curtime

	old::run_simulation ( REG_OUT, current_command, cmd, localplayer );

	//cs::i::globals->m_curtime = backup_curtime;

	if ( current_command == exploits::shifted_command ( ) ) {
		localplayer->tick_base ( ) = backup_tickbase;
	}

	if ( !in_cm ) {
		localplayer->velocity_modifier ( ) = backup_vel_mod;
	}

	/* local anims */
	if ( cmd->m_cmdnum > last_cmd_num ) {
		if ( localplayer->alive ( ) )
			anims::update_anims ( localplayer, g::angles );
		else /* reset fake */
			anims::manage_fake ( );
	
		last_cmd_num = cmd->m_cmdnum;
	}

	features::prediction::fix_netvars ( localplayer->tick_base ( ), true );
	features::prediction::fix_viewmodel ( true );

	MUTATE_END;
}