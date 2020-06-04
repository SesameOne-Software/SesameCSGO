#include "prediction.hpp"
#include "../globals.hpp"

/*
RUNCOMMAND HOOK
runcommand hook
	if ( exploits.data.exploit_cmd_nr == cmd->command_number ) {
		gl::local->tick_base ( ) = exploits.data.last_shot_tb;
		cs::globals->cur_time = gl::local->tick_base ( ) * cs::globals->interval_per_tick;
	}

	if ( cmd->tick_count == INT_MAX ) {
		cmd->hasbeenpredicted = true;
		return;
	}

	///  original

		///  restore local player shit

		don't need to recharge without teleport
		set layer 12 weight to 0 on fake (sway removal)
		calculate shoot matrix seperately (fix viewposition)

		oh clock correction is a cvar, not replicated and the default is 30ms
so you might add a slider for that or something idk

 if ( exploits.will_dtap ( ) ) {
		auto clock_correction = time_to_ticks ( 0.03f );
		auto latency_ticks = std::max<int> ( 0, time_to_ticks ( cs::engine->getnetchannelinfo ( )->getavglatency ( 0 ) ) );
		auto tick_base = cs::client_state->clock_drift ( ).server_tick + latency_ticks + 1;
		auto ideal_server_start = tick_base + clock_correction;

		//    this value will have to change once hide shots is added back in, look @ commented code above for reference
		auto expected_shift = std::min<int> ( exploits.data.behind_by, time_to_ticks ( gl::local->active_weapon ( )->data ( )->cycle_time ( ) ) );

		auto first_tick_in_batch_tc = ideal_server_start - std::min<int> ( 16, cs::client_state->choked_cmds ( ) + 1 + expected_shift ) + 1;

		tb = first_tick_in_batch_tc + cs::client_state->choked_cmds ( );
	}
*/

float features::prediction::predicted_curtime = 0.0f;

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
		static auto post_think_vphysics = pattern::search( _( "client.dll" ), _( "55 8B EC 83 E4 F8 81 EC ? ? ? ? 53 8B D9 56 57 83 BB ? ? ? ? ? 75 50" ) ).get< bool( __thiscall* )( entity_t* ) >( );
		static auto simulate_player_simulated_entities = pattern::search( _( "client.dll" ), _( "56 8B F1 57 8B BE ? ? ? ? 83 EF 01 78 72 90 8B 86" ) ).get< void( __thiscall* )( entity_t* ) >( );

		//MDLCACHE_CRITICAL_SECTION( );

		if ( local->health( ) > 0 || *reinterpret_cast< std::uint32_t* >( reinterpret_cast< std::uint32_t >( local ) + 0x3A81 ) ) {
			vfunc< void( __thiscall* )( void* ) >( local, 339 )( local );

			if ( local->flags( ) & 1 )
				*reinterpret_cast< std::uintptr_t* >( std::uintptr_t( local ) + 0x3014 ) = 0;

			if ( *reinterpret_cast< int* >( std::uintptr_t( local ) + 0x28BC ) == -1 )
				vfunc< void( __thiscall* )( void*, int ) >( local, 218 )( local, 0 );

			vfunc< void( __thiscall* )( void* ) >( local, 219 )( local );

			post_think_vphysics( local );
		}

		simulate_player_simulated_entities( local );

		vfunc< int ( __thiscall* )( void* ) > ( csgo::i::mdl_cache, 34 )( csgo::i::mdl_cache );
	}

	void start( ucmd_t* ucmd ) {
		auto local = csgo::i::ent_list->get< player_t* >( csgo::i::engine->get_local_player( ) );

		if ( !csgo::i::engine->is_in_game( ) || !ucmd || !local || !local->alive( ) )
			return;

		//if ( csgo::i::client_state->delta_tick( ) > 0 ) {
		//	csgo::i::pred->update(
		//		csgo::i::client_state->delta_tick( ),
		//		csgo::i::client_state->delta_tick( ) > 0,
		//		csgo::i::client_state->last_command_ack( ),
		//		csgo::i::client_state->last_outgoing_cmd( ) + csgo::i::client_state->choked( ) );
		//}

		if ( !movedata )
			movedata = std::malloc( 182 );

		if ( !prediction_player || !prediction_seed ) {
			prediction_seed = pattern::search( _( "client.dll" ), _( "8B 47 40 A3" ) ).add( 4 ).deref( ).get< std::uintptr_t >( );
			prediction_player = pattern::search( _( "client.dll" ), _( "0F 5B C0 89 35" ) ).add( 5 ).deref( ).get< std::uintptr_t >( );
		}

		*reinterpret_cast< int* >( prediction_seed ) = ucmd ? ucmd->m_randseed : -1;
		*reinterpret_cast< int* >( prediction_player ) = reinterpret_cast< int >( local );
		*reinterpret_cast< ucmd_t** >( std::uintptr_t( local ) + 0x3338 ) = ucmd;

		flags = local->flags( );
		velocity = local->vel( );

		curtime = csgo::i::globals->m_curtime;
		frametime = csgo::i::globals->m_frametime;
		tickcount = csgo::i::globals->m_tickcount;

		const int old_tickbase = local->tick_base( );
		const bool old_in_prediction = csgo::i::pred->m_in_prediction;
		const bool old_first_prediction = csgo::i::pred->m_is_first_time_predicted;

		csgo::i::globals->m_curtime = features::prediction::predicted_curtime;
		csgo::i::globals->m_frametime = csgo::i::pred->m_engine_paused ? 0 : csgo::i::globals->m_ipt;
		csgo::i::globals->m_tickcount = csgo::time2ticks( features::prediction::predicted_curtime );

		csgo::i::pred->m_is_first_time_predicted = false;
		csgo::i::pred->m_in_prediction = true;

		if ( ucmd->m_impulse )
			*reinterpret_cast< std::uint32_t* >( std::uintptr_t( local ) + 0x31FC ) = ucmd->m_impulse;

		ucmd->m_buttons |= ( *reinterpret_cast< int* >( uintptr_t ( local ) + 0x3330 ) );
		ucmd->m_buttons &= ~( *reinterpret_cast< int* >( uintptr_t ( local ) + 0x3334 ) );

		const auto v16 = ucmd->m_buttons;
		const auto unk02 = reinterpret_cast< int* >( std::uintptr_t( local ) + 0x31F8 );
		const auto v17 = v16 ^ *unk02;

		*reinterpret_cast< int* >( std::uintptr_t( local ) + 0x31EC ) = *unk02;
		*reinterpret_cast< int* >( std::uintptr_t( local ) + 0x31F8 ) = v16;
		*reinterpret_cast< int* >( std::uintptr_t( local ) + 0x31F0 ) = v16 & v17;
		*reinterpret_cast< int* >( std::uintptr_t( local ) + 0x31F4 ) = v17 & ~v16;

		csgo::i::pred->check_moving_ground( local, csgo::i::globals->m_frametime );

		// local->set_local_viewangles( g::last_real );

		/*
		if ( local->physics_run_think( 0 ) )
			local->pre_think( );
			*/
		const auto next_think = reinterpret_cast< int* >( std::uintptr_t( local ) + 0xFC );
		
		if ( *next_think != -1 && *next_think > 0 && *next_think <= local->tick_base ( ) ) {
			*next_think = -1;

			static auto unknown_fn = pattern::search ( _ ( "client.dll" ), _ ( "55 8B EC 56 57 8B F9 8B B7 ? ? ? ? 8B" ) ).get< void ( __thiscall* )( int ) > ( );
			unknown_fn ( 0 );

			local->think( );
		}

		csgo::i::move->start_track_prediction_errors( local );

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

		if ( !csgo::i::engine->is_in_game( ) || !ucmd || !local || !local->alive ( ) )
			return;

		csgo::i::move->finish_track_prediction_errors( local );
		csgo::i::move_helper->set_host( nullptr );
		csgo::i::move->reset ( );

		if ( csgo::i::globals->m_frametime > 0.0f )
			local->tick_base ( )++;

		csgo::i::globals->m_curtime = curtime;
		csgo::i::globals->m_frametime = frametime;
		csgo::i::globals->m_tickcount = tickcount;

		*reinterpret_cast< std::uint32_t* >( reinterpret_cast< std::uintptr_t >( local ) + 0x3334 ) = 0;
		*reinterpret_cast< int* >( prediction_seed ) = 0xffffffff;
		*reinterpret_cast< int* >( prediction_player ) = 0;
	}
}

void features::prediction::update_curtime( ) {
	static int g_tick = 0;
	static ucmd_t* last_cmd = nullptr;

	if ( !g::local )
		return;

	g_tick = ( !last_cmd || last_cmd->m_hasbeenpredicted ) ? g::local->tick_base( ) : ( g_tick + 1 );
	last_cmd = g::ucmd;
	predicted_curtime = csgo::ticks2time( g_tick );

}

void features::prediction::run( const std::function< void( ) >& fn ) {
	prediction_util::start( g::ucmd );
	fn( );
	prediction_util::end( g::ucmd );
}