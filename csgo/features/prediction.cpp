﻿#include "prediction.hpp"
#include "../globals.hpp"
#include "../hooks/in_prediction.hpp"

float predicted_curtime = 0.0f;

namespace prediction_util {
	float frametime;
	float curtime;
	int tickcount;
	flags_t flags;
	vec3_t velocity;
	void* movedata;
	uintptr_t prediction_player;
	uintptr_t prediction_seed;
	bool first_time_pred;
	bool in_pred;

	void start( ucmd_t* ucmd ) {
		auto local = cs::i::ent_list->get< player_t* >( cs::i::engine->get_local_player( ) );

		if ( !cs::i::engine->is_in_game( ) || !ucmd || !local || !local->alive( ) )
			return;

		if ( !movedata )
			movedata = std::malloc( 256 ); // 182

		if ( !prediction_player || !prediction_seed ) {
			prediction_seed = pattern::search( _( "client.dll" ), _( "8B 47 40 A3" ) ).add( 4 ).deref( ).get< std::uintptr_t >( );
			prediction_player = pattern::search( _( "client.dll" ), _( "0F 5B C0 89 35" ) ).add( 5 ).deref( ).get< std::uintptr_t >( );
		}

		first_time_pred = cs::i::pred->m_is_first_time_predicted;
		in_pred = cs::i::pred->m_in_prediction;

		*reinterpret_cast< int* >( prediction_seed ) = ucmd ? ucmd->m_randseed : -1;
		*reinterpret_cast< uintptr_t* >( prediction_player ) = reinterpret_cast< uintptr_t >( local );
		*reinterpret_cast< ucmd_t** >( std::uintptr_t( local ) + 0x3338 ) = ucmd;

		flags = local->flags( );
		velocity = local->vel( );

		curtime = cs::i::globals->m_curtime;
		frametime = cs::i::globals->m_frametime;
		tickcount = cs::i::globals->m_tickcount;

		cs::i::globals->m_curtime = predicted_curtime = cs::ticks2time( local->tick_base( ) );
		cs::i::globals->m_frametime = cs::i::pred->m_engine_paused ? 0.0f : cs::i::globals->m_ipt;

		cs::i::pred->m_in_prediction = true;

		if ( ucmd->m_impulse )
			*reinterpret_cast< uint8_t* >( std::uintptr_t( local ) + 0x31FC ) = ucmd->m_impulse;
		
		ucmd->m_buttons |= *reinterpret_cast< buttons_t* >( uintptr_t( local ) + 0x3334 );
		ucmd->m_buttons &= ~*reinterpret_cast< buttons_t* >( uintptr_t( local ) + 0x3330 );
		
		const auto v16 = ucmd->m_buttons;
		const auto unk02 = reinterpret_cast< buttons_t* >( std::uintptr_t( local ) + 0x31F8 );
		const auto v17 = v16 ^ *unk02;
		
		*reinterpret_cast< buttons_t* >( std::uintptr_t( local ) + 0x31EC ) = *unk02;
		*unk02 = v16;
		*reinterpret_cast< buttons_t* >( std::uintptr_t( local ) + 0x31F0 ) = v16 & v17;
		*reinterpret_cast< buttons_t* >( std::uintptr_t( local ) + 0x31F4 ) = v17 & ~v16;

		cs::i::move_helper->set_host ( local );
		cs::i::move->start_track_prediction_errors ( local );
		
		cs::i::pred->setup_move( local, ucmd, cs::i::move_helper, movedata );

		cs::i::move->process_movement( local, movedata );
		cs::i::pred->finish_move ( local, ucmd, movedata );
		
	}

	void end( ucmd_t* ucmd ) {
		auto local = cs::i::ent_list->get< player_t* >( cs::i::engine->get_local_player( ) );

		if ( !cs::i::engine->is_in_game( ) || !ucmd || !local || !local->alive( ) )
			return;

		cs::i::move->finish_track_prediction_errors ( local );
		cs::i::move_helper->set_host ( nullptr );
		cs::i::move->reset ( );

		//cs::i::globals->m_curtime = curtime;
		cs::i::globals->m_frametime = frametime;

		*reinterpret_cast< uint32_t* >( reinterpret_cast< std::uintptr_t >( local ) + 0x3338 ) = 0;
		*reinterpret_cast< int* >( prediction_seed ) = -1;
		*reinterpret_cast< int* >( prediction_player ) = 0;

		cs::i::pred->m_in_prediction = in_pred;
	}
}

int features::prediction::shift_tickbase ( ) {
	return 0;
}

float features::prediction::curtime( ) {
	return predicted_curtime;
}

void features::prediction::update( int stage ) {
	if ( !cs::i::engine->is_in_game( ) || !cs::i::engine->is_connected( ) || stage != 4 )
		return;

	//cs::i::pred->update (
	//	cs::i::client_state->delta_tick ( ),
	//	cs::i::client_state->delta_tick ( ) > 0,
	//	cs::i::client_state->last_command_ack ( ),
	//	cs::i::client_state->last_outgoing_cmd ( ) + cs::i::client_state->choked ( )
	//);
}

void features::prediction::fix_viewmodel ( bool store ) {
	if ( !g::local || !g::local->alive() || g::local->viewmodel_handle ( ) == 0xFFFFFFFF )
		return;

	const auto viewmodel = cs::i::ent_list->get_by_handle< weapon_t* > ( g::local->viewmodel_handle ( ));

	if ( !viewmodel )
		return;

	static float viewmodel_anim_time = 0.0f;
	static float viewmodel_cycle = 0.0f;

	if ( store ) {
		viewmodel_anim_time = viewmodel->anim_time ( );
		viewmodel_cycle = viewmodel->cycle ( );
	}
	else {
		viewmodel->anim_time ( ) = viewmodel_anim_time;
		viewmodel->cycle ( ) = viewmodel_cycle;
	}
}

void features::prediction::run( const std::function< void( ) >& fn ) {
	prediction_util::start( g::ucmd );
	fn( );
	prediction_util::end( g::ucmd );
}