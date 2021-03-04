#include "run_command.hpp"
#include "../globals.hpp"

#include "../features/prediction.hpp"
#include "../animations/anims.hpp"

decltype( &hooks::run_command ) hooks::old::run_command = nullptr;

extern bool in_cm;

namespace lby {
	extern bool in_update;
}

features::prediction::netvars_t backup_netvars;

void __fastcall hooks::run_command( REG, player_t* ent, ucmd_t* ucmd, c_move_helper* move_helper ) {
	if (!ent || !g::local || ent != g::local || !ucmd || !g::local->alive() )
		return old::run_command( REG_OUT , ent , ucmd , move_helper );

	//if ( in_cm ) {
	//	backup_netvars.store( ent );
	//	features::prediction::fix_netvars( cs::i::client_state->last_command_ack( ) - 1 );
	//}

	const auto backup_vel_mod = ent->velocity_modifier( );
	
	if ( in_cm && cs::i::client_state && ucmd->m_cmdnum == cs::i::client_state->last_command_ack( ) + 1 )
		ent->velocity_modifier( ) = features::prediction::vel_modifier;

	old::run_command( REG_OUT , ent , ucmd , move_helper );
	
	if ( !in_cm )
		ent->velocity_modifier( ) = backup_vel_mod;

	//anims::update_anims( ent , lby::in_update ? g::sent_cmd.m_angs : g::angles , false );

	//if ( in_cm )
	//	backup_netvars.reapply( ent );
	//else
	//	features::prediction::fix_netvars( cs::i::client_state->last_command_ack( ) , true );
}