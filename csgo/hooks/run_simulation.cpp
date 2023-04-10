#include "run_simulation.hpp"

#include "../animations/resolver.hpp"

#include "../animations/anims.hpp"
#include "../features/exploits.hpp"
#include "../features/prediction.hpp"

#include "../features/antiaim.hpp"

#undef min
#undef max

decltype( &hooks::run_simulation ) hooks::old::run_simulation = nullptr;

extern bool in_cm;

namespace lby {
	extern bool in_update;
}

int last_cmd_num = 0;

int __fastcall hooks::run_simulation ( REG, int current_command, ucmd_t* cmd, player_t* localplayer ) {
	if ( !g::local || !g::local->alive ( ) )
		last_cmd_num = N ( 0 );
	
	if ( !localplayer || localplayer != g::local || !g::local )
		return old::run_simulation ( REG_OUT, current_command, cmd, localplayer );

	static auto using_standard_weapons_in_vehicle = pattern::search ( _ ( "client.dll" ), _ ( "E8 ? ? ? ? 8B 16 8B CE 8A D8" ) ).resolve_rip ( ).get< bool ( __thiscall* )( player_t* ) > ( );

	if ( cmd->m_tickcount == std::numeric_limits<int>::max ( ) ) {
		cmd->m_hasbeenpredicted = true;
		localplayer->tick_base ( )++;

		return 0;
	}
	
	const auto backup_vel_mod = localplayer->velocity_modifier ( );
	const auto backup_tickbase = localplayer->tick_base ( );
	
	const auto backup_vel_modifier = localplayer->velocity_modifier ( );
	localplayer->velocity_modifier ( ) = features::prediction::vel_modifier;
	
	if ( cmd->m_cmdnum == exploits::tickshift [ cmd->m_cmdnum % exploits::tickshift.size ( ) ].first )
		exploits::adjust_player_time_base ( localplayer, exploits::tickshift [ cmd->m_cmdnum % exploits::tickshift.size ( ) ].second );
	
	auto curtime = cs::i::globals->m_curtime = cs::ticks2time ( localplayer->tick_base ( ) );
	__asm movss xmm2, curtime
	
	if ( cmd->m_cmdnum > last_cmd_num )
		features::prediction::fix_netvars ( cs::i::client_state->last_command_ack ( ) - N ( 1 ), true );
	
	old::run_simulation ( REG_OUT, cmd->m_cmdnum, cmd, localplayer );
	
	if ( cmd->m_cmdnum > last_cmd_num )
		features::prediction::fix_netvars ( cs::i::client_state->last_command_ack ( ) );
	
	localplayer->velocity_modifier ( ) = backup_vel_modifier;
	
	features::prediction::fix_viewmodel ( true );
	
	/* local anims */
	if ( cmd->m_cmdnum > last_cmd_num ) {
		if ( g::send_packet )
			anims::update_anims ( localplayer, g::sent_cmd.m_angs );
		else
			anims::manage_fake ( );

		last_cmd_num = cmd->m_cmdnum;
	}
}