#include "prediction.hpp"
#include "../globals.hpp"
#include "../hooks/in_prediction.hpp"

float predicted_curtime = 0.0f;

namespace prediction_util {
	float frametime;
	float curtime;
	int tickcount;
	int flags;
	vec3_t velocity;
	void* movedata;
	uintptr_t prediction_player;
	uintptr_t prediction_seed;

	void start( ucmd_t* ucmd ) {
		auto local = csgo::i::ent_list->get< player_t* >( csgo::i::engine->get_local_player( ) );

		if ( !csgo::i::engine->is_in_game( ) || !ucmd || !local || !local->alive( ) )
			return;

		if ( !movedata )
			movedata = std::malloc( 256 ); // 182

		if ( !prediction_player || !prediction_seed ) {
			prediction_seed = pattern::search( _( "client.dll" ), _( "8B 47 40 A3" ) ).add( 4 ).deref( ).get< std::uintptr_t >( );
			prediction_player = pattern::search( _( "client.dll" ), _( "0F 5B C0 89 35" ) ).add( 5 ).deref( ).get< std::uintptr_t >( );
		}

		if ( csgo::i::client_state->choked ( ) > 0 ) {
			//hooks::prediction::disable_sounds = true;
			//
			csgo::i::pred->update(
				csgo::i::client_state->delta_tick( ),
				csgo::i::client_state->delta_tick( ) > 0,
				csgo::i::client_state->last_command_ack( ),
				csgo::i::client_state->last_outgoing_cmd( ) + csgo::i::client_state->choked( )
			);
			//
			//hooks::prediction::disable_sounds = false;
		}

		const auto first_time_pred = csgo::i::pred->m_is_first_time_predicted;
		const auto in_pred = csgo::i::pred->m_in_prediction;

		*reinterpret_cast< int* >( prediction_seed ) = ucmd ? ucmd->m_randseed : -1;
		*reinterpret_cast< int* >( prediction_player ) = reinterpret_cast< int >( local );
		*reinterpret_cast< ucmd_t** >( std::uintptr_t( local ) + 0x3338 ) = ucmd;

		flags = local->flags( );
		velocity = local->vel( );

		curtime = csgo::i::globals->m_curtime;
		frametime = csgo::i::globals->m_frametime;
		tickcount = csgo::i::globals->m_tickcount;

		csgo::i::globals->m_curtime = predicted_curtime = csgo::ticks2time( local->tick_base( ) );
		csgo::i::globals->m_frametime = csgo::i::pred->m_engine_paused ? 0.0f : csgo::i::globals->m_ipt;
		csgo::i::globals->m_tickcount = local->tick_base( );

		csgo::i::pred->m_is_first_time_predicted = false;
		csgo::i::pred->m_in_prediction = true;

		if ( ucmd->m_impulse )
			*reinterpret_cast< std::uint32_t* >( std::uintptr_t( local ) + 0x31FC ) = ucmd->m_impulse;

		ucmd->m_buttons |= ( *reinterpret_cast< int* >( uintptr_t( local ) + 0x3334 ) );
		ucmd->m_buttons &= ~( *reinterpret_cast< int* >( uintptr_t( local ) + 0x3330 ) );

		const auto v16 = ucmd->m_buttons;
		const auto unk02 = reinterpret_cast< int* >( std::uintptr_t( local ) + 0x31F8 );
		const auto v17 = v16 ^ *unk02;

		*reinterpret_cast< int* >( std::uintptr_t( local ) + 0x31EC ) = *unk02;
		*unk02 = v16;
		*reinterpret_cast< int* >( std::uintptr_t( local ) + 0x31F0 ) = v16 & v17;
		*reinterpret_cast< int* >( std::uintptr_t( local ) + 0x31F4 ) = v17 & ~v16;

		csgo::i::move_helper->set_host( local );
		csgo::i::move->start_track_prediction_errors( local );
		csgo::i::pred->setup_move( local, ucmd, csgo::i::move_helper, movedata );
		csgo::i::move->process_movement( local, movedata );
		csgo::i::pred->finish_move( local, ucmd, movedata );
		csgo::i::move_helper->process_impacts( );
		csgo::i::move->finish_track_prediction_errors( local );
		csgo::i::move_helper->set_host( nullptr );

		csgo::i::pred->m_is_first_time_predicted = first_time_pred;
		csgo::i::pred->m_in_prediction = in_pred;
	}

	void end( ucmd_t* ucmd ) {
		auto local = csgo::i::ent_list->get< player_t* >( csgo::i::engine->get_local_player( ) );

		if ( !csgo::i::engine->is_in_game( ) || !ucmd || !local || !local->alive( ) )
			return;

		csgo::i::globals->m_curtime = curtime;
		csgo::i::globals->m_frametime = frametime;
		csgo::i::globals->m_tickcount = tickcount;

		*reinterpret_cast< std::uint32_t* >( reinterpret_cast< std::uintptr_t >( local ) + 0x3338 ) = 0;
		*reinterpret_cast< int* >( prediction_seed ) = -1;
		*reinterpret_cast< int* >( prediction_player ) = 0;

		csgo::i::move->reset( );
	}
}

int features::prediction::shift_tickbase ( ) {
	return 0;
}

float features::prediction::curtime( ) {
	return predicted_curtime;
}

void features::prediction::update( int stage ) {
	//if ( m_stored_variables.m_flVelocityModifier < 1.0 ) {
	//	*reinterpret_cast< int* >( uintptr_t( csgo::i::pred ) + 36 ) = 1;
	//}

	if ( !csgo::i::engine->is_in_game( ) || !csgo::i::engine->is_connected( ) || stage != 4 )
		return;
}

void features::prediction::run( const std::function< void( ) >& fn ) {
	prediction_util::start( g::ucmd );
	fn( );
	prediction_util::end( g::ucmd );
}