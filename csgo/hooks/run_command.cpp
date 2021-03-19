#include "run_command.hpp"
#include "../globals.hpp"

#include "../features/prediction.hpp"
#include "../animations/anims.hpp"

decltype( &hooks::run_command ) hooks::old::run_command = nullptr;

extern bool in_cm;

namespace lby {
	extern bool in_update;
}

int last_cmd_num = 0;

void __fastcall hooks::run_command( REG, player_t* ent, ucmd_t* ucmd, c_move_helper* move_helper ) {
	if ( !g::local || !g::local->alive( ) )
		last_cmd_num = 0;

	if (!ent || !g::local || ent != g::local || !ucmd || !g::local->alive() )
		return old::run_command( REG_OUT , ent , ucmd , move_helper );

	const auto backup_vel_mod = ent->velocity_modifier( );
	
	if ( in_cm && cs::i::client_state && ucmd->m_cmdnum == cs::i::client_state->last_command_ack( ) + 1 )
		ent->velocity_modifier( ) = features::prediction::vel_modifier;

	old::run_command( REG_OUT , ent , ucmd , move_helper );
	
	if ( !in_cm )
		ent->velocity_modifier( ) = backup_vel_mod;

	/* local anims */
	if ( ucmd->m_cmdnum > last_cmd_num ) {
		if ( g::local && g::local->alive( ) ) {
			//const auto backup_flags = g::local->flags( );
			//g::local->flags( ) = anims::createmove_flags;
			anims::update_anims( g::local , lby::in_update ? g::sent_cmd.m_angs : g::angles );
			//g::local->flags( ) = backup_flags;
		}
		else /* reset fake */
			anims::manage_fake( );

		last_cmd_num = ucmd->m_cmdnum;
	}

	features::prediction::fix_netvars( g::local->tick_base() , true );
	features::prediction::fix_viewmodel( true );
}