#include "prediction.hpp"
#include "../globals.hpp"

namespace prediction_util {
	float frametime;
	float curtime;
	int tickcount;
	int flags;
	vec3_t velocity;
	void* movedata;
	uintptr_t prediction_player;
	uintptr_t prediction_seed;

	void post_think( player_t* local ) {
		static auto post_think_vphysics = pattern::search( "client_panorama.dll", "55 8B EC 83 E4 F8 81 EC ? ? ? ? 53 8B D9 56 57 83 BB ? ? ? ? ? 75 50" ).get< bool( __thiscall* )( entity_t* ) >( );
		static auto simulate_player_simulated_entities = pattern::search( "client_panorama.dll", "56 8B F1 57 8B BE ? ? ? ? 83 EF 01 78 72 90 8B 86" ).get< void( __thiscall* )( entity_t* ) >( );

		MDLCACHE_CRITICAL_SECTION( );

		if ( local->valid( ) ) {
			vfunc< void( __thiscall* )( void* ) >( local, 338 )( local );

			if ( local->flags( ) & 1 )
				*reinterpret_cast< std::uintptr_t* >( std::uintptr_t( local ) + 0x3014 ) = 0;

			if ( *reinterpret_cast< int* >( std::uintptr_t( local ) + 0x28BC ) == -1 )
				vfunc< void( __thiscall* )( void*, int ) >( local, 218 )( local, 0 );

			vfunc< void( __thiscall* )( void* ) >( local, 219 )( local );

			post_think_vphysics( local );
		}

		simulate_player_simulated_entities( local );
	}

	void start( ucmd_t* ucmd ) {
		auto local = csgo::i::ent_list->get< player_t* >( csgo::i::engine->get_local_player( ) );

		if ( !csgo::i::engine->is_in_game( ) || !ucmd || !local )
			return;

		/*
		if ( csgo::i::client_state->delta_tick( ) > 0 ) {
			csgo::i::pred->update(
				csgo::i::client_state->delta_tick( ),
				csgo::i::client_state->delta_tick( ) > 0,
				csgo::i::client_state->last_command_ack( ),
				csgo::i::client_state->last_outgoing_cmd( ) + csgo::i::client_state->choked( ) );
		}
		*/

		if ( !movedata )
			movedata = std::malloc( 182 );

		if ( !prediction_player || !prediction_seed ) {
			prediction_seed = pattern::search( "client_panorama.dll", "8B 47 40 A3" ).add( 4 ).deref( ).get< std::uintptr_t >( );
			prediction_player = pattern::search( "client_panorama.dll", "0F 5B C0 89 35" ).add( 5 ).deref( ).get< std::uintptr_t >( );
		}

		*reinterpret_cast< int* >( prediction_seed ) = ucmd ? ucmd->m_randseed : -1;
		*reinterpret_cast< int* >( prediction_player ) = reinterpret_cast< int >( local );
		*reinterpret_cast< ucmd_t** >( std::uintptr_t( local ) + 0x3334 ) = ucmd;
		*reinterpret_cast< ucmd_t** >( std::uintptr_t( local ) + 0x3288 ) = ucmd;

		flags = local->flags( );
		velocity = local->vel( );

		curtime = csgo::i::globals->m_curtime;
		frametime = csgo::i::globals->m_frametime;
		tickcount = csgo::i::globals->m_tickcount;

		const int old_tickbase = local->tick_base( );
		const bool old_in_prediction = csgo::i::pred->m_in_prediction;
		const bool old_first_prediction = csgo::i::pred->m_is_first_time_predicted;

		csgo::i::globals->m_curtime = local->tick_base( ) * csgo::i::globals->m_ipt;
		csgo::i::globals->m_frametime = csgo::i::pred->m_engine_paused ? 0 : csgo::i::globals->m_ipt;
		csgo::i::globals->m_tickcount = local->tick_base( );

		csgo::i::pred->m_is_first_time_predicted = false;
		csgo::i::pred->m_in_prediction = true;

		if ( ucmd->m_impulse )
			*reinterpret_cast< std::uint32_t* >( std::uintptr_t( local ) + 0x31FC ) = ucmd->m_impulse;

		ucmd->m_buttons |= *reinterpret_cast< std::uint32_t* >( std::uintptr_t( local ) + 0x3330 );

		const auto v16 = ucmd->m_buttons;
		const auto unk02 = reinterpret_cast< int* >( std::uintptr_t( local ) + 0x31F8 );
		const auto v17 = v16 ^ *unk02;

		*reinterpret_cast< int* >( std::uintptr_t( local ) + 0x31EC ) = *unk02;
		*reinterpret_cast< int* >( std::uintptr_t( local ) + 0x31F8 ) = v16;
		*reinterpret_cast< int* >( std::uintptr_t( local ) + 0x31F0 ) = v16 & v17;
		*reinterpret_cast< int* >( std::uintptr_t( local ) + 0x31F4 ) = v17 & ~v16;

		csgo::i::pred->check_moving_ground( local, csgo::i::globals->m_frametime );

		// local->set_local_viewangles( g::last_real );

		if ( local->physics_run_think( 0 ) )
			local->pre_think( );

		const auto next_think = reinterpret_cast< int* >( std::uintptr_t( local ) + 0xFC );

		if ( *next_think > 0 && *next_think <= local->tick_base( ) ) {
			*next_think = -1;
			local->think( );
		}

		// csgo::i::move->start_track_prediction_errors( local );

		csgo::i::move_helper->set_host( local );
		csgo::i::pred->setup_move( local, ucmd, csgo::i::move_helper, movedata );
		csgo::i::move->process_movement( local, movedata );
		csgo::i::pred->finish_move( local, ucmd, movedata );
		csgo::i::move_helper->process_impacts( );

		local->post_think( );

		local->tick_base( ) = old_tickbase;

		csgo::i::pred->m_is_first_time_predicted = old_first_prediction;
		csgo::i::pred->m_in_prediction = old_in_prediction;
	}

	void end( ucmd_t* ucmd ) {
		auto local = csgo::i::ent_list->get< player_t* >( csgo::i::engine->get_local_player( ) );

		if ( !csgo::i::engine->is_in_game( ) || !ucmd || !local )
			return;

		// csgo::i::move->finish_track_prediction_errors( local );
		csgo::i::move_helper->set_host( nullptr );

		csgo::i::globals->m_curtime = curtime;
		csgo::i::globals->m_frametime = frametime;
		csgo::i::globals->m_tickcount = tickcount;

		*reinterpret_cast< std::uint32_t* >( reinterpret_cast< std::uintptr_t >( local ) + 0x3334 ) = 0;
		*reinterpret_cast< int* >( prediction_seed ) = 0xffffffff;
		*reinterpret_cast< int* >( prediction_player ) = 0;

		csgo::i::move->reset( );
	}
}

void features::prediction::run( const std::function< void( ) >& fn ) {
	prediction_util::start( g::ucmd );
	fn( );
	prediction_util::end( g::ucmd );
}