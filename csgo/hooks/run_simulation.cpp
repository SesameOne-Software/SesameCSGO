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
		localplayer->set_abs_origin ( localplayer->origin() );
		localplayer->tick_base ( )++;
		MUTATE_END;
		return;
	}

	MUTATE_START;
	const auto backup_vel_mod = localplayer->velocity_modifier ( );
	const auto backup_curtime = cs::i::globals->m_curtime;
	const auto backup_tickbase = localplayer->tick_base ( );

	const auto backup_vel_modifier = localplayer->velocity_modifier ( );
	localplayer->velocity_modifier ( ) = features::prediction::vel_modifier;

	if ( current_command == exploits::tickshift [ current_command % exploits::tickshift.size ( ) ].first ) {
		localplayer->tick_base ( ) -= exploits::tickshift [ current_command % exploits::tickshift.size ( ) ].second;
		//exploits::adjust_player_time_base ( localplayer, exploits::tickshift [ current_command % exploits::tickshift.size ( ) ].second );
	}

	auto curtime = cs::i::globals->m_curtime = cs::ticks2time ( localplayer->tick_base ( ) );
	__asm movss xmm2, curtime
	cs::i::globals->m_curtime = backup_curtime;

	if ( current_command > last_cmd_num )
		features::prediction::fix_netvars ( cs::i::client_state->last_command_ack ( ) - 1, true );

	old::run_simulation ( REG_OUT, current_command, cmd, localplayer );

	if ( current_command > last_cmd_num )
		features::prediction::fix_netvars ( cs::i::client_state->last_command_ack ( ) );

	localplayer->velocity_modifier ( ) = backup_vel_modifier;

	features::prediction::fix_viewmodel ( true );

	/* local anims */
	if ( current_command > last_cmd_num ) {
		if ( localplayer->alive ( ) )
			anims::update_anims ( localplayer, lby::in_update ? g::sent_cmd.m_angs : g::angles );
		else /* reset fake */
			anims::manage_fake ( );

		last_cmd_num = current_command;
	}

	MUTATE_END;
}