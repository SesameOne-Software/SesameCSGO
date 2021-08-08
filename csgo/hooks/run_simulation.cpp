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

int __fastcall hooks::run_simulation ( REG, int current_command, ucmd_t* cmd, player_t* localplayer ) {
	OBF_BEGIN;
	IF ( !g::local || !g::local->alive ( ) )
		last_cmd_num = N ( 0 ); ENDIF;
	
	IF ( !localplayer || localplayer != g::local || !g::local )
		RETURN ( old::run_simulation ( REG_OUT, current_command, cmd, localplayer ) ); ENDIF;
	
	IF ( cmd->m_tickcount == std::numeric_limits<int>::max ( ) ) {
		cmd->m_hasbeenpredicted = true;

		cs::i::move_helper->set_host ( g::local );

		IF ( *reinterpret_cast< uint8_t* > ( reinterpret_cast< uintptr_t >( cmd ) + N ( 52 ) ) )
			* reinterpret_cast< uint32_t* >( reinterpret_cast< uintptr_t >( g::local ) + ( g::is_legacy ? N ( 0x31EC ) : N ( 0x31FC ) ) ) = *reinterpret_cast< uint8_t* > ( reinterpret_cast< uintptr_t >( cmd ) + N ( 52 ) ); ENDIF;

		*reinterpret_cast< uint32_t* > ( reinterpret_cast< uintptr_t >( cmd ) + N ( 48 ) ) |= *reinterpret_cast< uint32_t* > ( reinterpret_cast< uintptr_t >( g::local ) + ( g::is_legacy ? N ( 0x3310 ) : N ( 0x3334 ) ) );
		*reinterpret_cast< uint32_t* > ( reinterpret_cast< uintptr_t >( cmd ) + N ( 48 ) ) &= ~*reinterpret_cast< uint32_t* > ( reinterpret_cast< uintptr_t >( g::local ) + ( g::is_legacy ? N ( 0x330C ) : N ( 0x3330 ) ) );

		const auto v16 = *reinterpret_cast< uint32_t* > ( reinterpret_cast< uintptr_t >( cmd ) + N ( 48 ) );
		const auto v17 = v16 ^ *reinterpret_cast< uint32_t* > ( reinterpret_cast< uintptr_t >( g::local ) + ( g::is_legacy ? N ( 0x31E8 ) : N ( 0x31F8 ) ) );

		*reinterpret_cast< uint32_t* > ( reinterpret_cast< uintptr_t >( g::local ) + ( g::is_legacy ? N ( 0x31DC ) : N ( 0x31EC ) ) ) = *reinterpret_cast< uint32_t* > ( reinterpret_cast< uintptr_t >( g::local ) + ( g::is_legacy ? N ( 0x31E8 ) : N ( 0x31F8 ) ) );
		*reinterpret_cast< uint32_t* > ( reinterpret_cast< uintptr_t >( g::local ) + ( g::is_legacy ? N ( 0x31E8 ) : N ( 0x31F8 ) ) ) = v16;
		*reinterpret_cast< uint32_t* > ( reinterpret_cast< uintptr_t >( g::local ) + ( g::is_legacy ? N ( 0x31E0 ) : N ( 0x31F0 ) ) ) = v16 & v17;
		*reinterpret_cast< uint32_t* > ( reinterpret_cast< uintptr_t >( g::local ) + ( g::is_legacy ? N ( 0x31E4 ) : N ( 0x31F4 ) ) ) = v17 & ~v16;

		localplayer->post_think ( );
		localplayer->tick_base ( )++;

		cs::i::move_helper->set_host ( nullptr );
		RETURN( 0 );
	} ENDIF;
	
	const auto backup_vel_mod = localplayer->velocity_modifier ( );
	const auto backup_tickbase = localplayer->tick_base ( );
	
	const auto backup_vel_modifier = localplayer->velocity_modifier ( );
	localplayer->velocity_modifier ( ) = features::prediction::vel_modifier;
	
	IF ( current_command == exploits::tickshift [ current_command % exploits::tickshift.size ( ) ].first )
		exploits::adjust_player_time_base ( localplayer, exploits::tickshift [ current_command % exploits::tickshift.size ( ) ].second ); ENDIF;
	
	auto curtime = cs::i::globals->m_curtime = cs::ticks2time ( localplayer->tick_base ( ) );
	__asm movss xmm2, curtime
	
	IF ( current_command > last_cmd_num )
		features::prediction::fix_netvars ( cs::i::client_state->last_command_ack ( ) - N ( 1 ), true ); ENDIF;
	
	old::run_simulation ( REG_OUT, current_command, cmd, localplayer );
	
	IF ( current_command > last_cmd_num )
		features::prediction::fix_netvars ( cs::i::client_state->last_command_ack ( ) ); ENDIF;
	
	localplayer->velocity_modifier ( ) = backup_vel_modifier;
	
	features::prediction::fix_viewmodel ( true );
	
	/* local anims */
	if ( current_command > last_cmd_num ) {
		if ( g::local && g::local->alive ( ) )
			anims::update_anims ( g::local, lby::in_update ? g::sent_cmd.m_angs : g::angles );
		else
			anims::manage_fake ( );

		last_cmd_num = current_command;
	}

	OBF_END;
}