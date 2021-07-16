#include "prediction.hpp"
#include "../globals.hpp"
#include "../hooks/in_prediction.hpp"
#include "exploits.hpp"

float predicted_curtime = 0.0f;

namespace prediction_util {
	float frametime;
	float curtime;
	int tickcount;
	flags_t flags;
	vec3_t velocity;
	uint8_t movedata[256];
	uintptr_t prediction_player;
	uintptr_t prediction_seed;
	bool first_time_pred;
	bool in_pred;

	void start( ucmd_t* ucmd ) {
		features::prediction::in_prediction = true;

		if ( !cs::i::engine->is_in_game ( ) || !ucmd || !g::local || !g::local->alive ( ) )
				return;

		if ( !prediction_player || !prediction_seed ) {
			prediction_seed = pattern::search( _( "client.dll" ), _( "8B 47 40 A3" ) ).add( 4 ).deref( ).get< std::uintptr_t >( );
			prediction_player = pattern::search( _( "client.dll" ), _( "0F 5B C0 89 35" ) ).add( 5 ).deref( ).get< std::uintptr_t >( );
		}

		if ( cs::i::client_state->delta_tick ( ) > 0 ) {
			cs::i::pred->update (
				cs::i::client_state->delta_tick ( ),
				cs::i::client_state->delta_tick ( ) > 0,
				cs::i::client_state->last_command_ack ( ),
				cs::i::client_state->last_outgoing_cmd ( ) + cs::i::client_state->choked ( )
			);
		}

		first_time_pred = cs::i::pred->m_is_first_time_predicted;
		in_pred = cs::i::pred->m_in_prediction;

		*reinterpret_cast< int* >( prediction_seed ) = ucmd ? ucmd->m_randseed : -1;
		*reinterpret_cast< uintptr_t* >( prediction_player ) = reinterpret_cast< uintptr_t >( g::local );

		*reinterpret_cast< uint32_t* >( std::uintptr_t ( g::local ) + 0x3338 ) = reinterpret_cast< uintptr_t >( ucmd );
		*reinterpret_cast< ucmd_t* >( std::uintptr_t ( g::local ) + 0x3288 ) = *ucmd;

		flags = g::local->flags( );
		features::prediction::crouch_amount = g::local->crouch_amount ( );
		features::prediction::vel = g::local->vel( );

		curtime = cs::i::globals->m_curtime;
		frametime = cs::i::globals->m_frametime;
		tickcount = cs::i::globals->m_tickcount;

		cs::i::globals->m_curtime = predicted_curtime = cs::ticks2time( g::local->tick_base( ) );
		cs::i::globals->m_frametime = cs::i::pred->m_engine_paused ? 0.0f : cs::i::globals->m_ipt;
		cs::i::globals->m_tickcount = g::local->tick_base ( );

		cs::i::pred->m_is_first_time_predicted = false;
		cs::i::pred->m_in_prediction = true;

		if ( *reinterpret_cast< uint8_t* > ( reinterpret_cast< uintptr_t >( ucmd ) + 52 ) )
			*reinterpret_cast< uint32_t* >( reinterpret_cast< uintptr_t >( g::local ) + 0x31FC ) = *reinterpret_cast< uint8_t* > ( reinterpret_cast< uintptr_t >( ucmd ) + 52 );

		*reinterpret_cast<uint32_t*> ( reinterpret_cast< uintptr_t >( ucmd ) + 48 ) |= *reinterpret_cast< uint32_t* > ( reinterpret_cast< uintptr_t >( g::local ) + 0x3334 );
		*reinterpret_cast<uint32_t*> ( reinterpret_cast< uintptr_t >( ucmd ) + 48 ) &= ~*reinterpret_cast< uint32_t* > ( reinterpret_cast< uintptr_t >( g::local ) + 0x3330 );

		const auto v16 = *reinterpret_cast< uint32_t* > ( reinterpret_cast< uintptr_t >( ucmd ) + 48 );
		const auto v17 = v16 ^ *reinterpret_cast< uint32_t* > ( reinterpret_cast< uintptr_t >( g::local ) + 0x31F8 );
		*reinterpret_cast<uint32_t*> ( reinterpret_cast<uintptr_t>(g::local) + 0x31EC ) = *reinterpret_cast< uint32_t* > ( reinterpret_cast< uintptr_t >( g::local ) + 0x31F8 );
		*reinterpret_cast<uint32_t*> ( reinterpret_cast<uintptr_t>(g::local) + 0x31F8 ) = v16;
		*reinterpret_cast<uint32_t*> ( reinterpret_cast<uintptr_t>(g::local) + 0x31F0 ) = v16 & v17;
		*reinterpret_cast<uint32_t*> ( reinterpret_cast<uintptr_t>(g::local) + 0x31F4 ) = v17 & ~v16;
		
		// set host player
		cs::i::move_helper->set_host ( g::local );

		cs::i::move->start_track_prediction_errors ( g::local );

		cs::i::pred->check_moving_ground ( g::local, cs::i::globals->m_frametime );

		// copy angles from command to player
		//cs::i::pred->set_local_viewangles ( ucmd->m_angs );

		// run prethink
		if ( g::local->physics_run_think ( 0 ) )
			g::local->pre_think ( );

		// run think
		auto& next_think = *reinterpret_cast< uint32_t* > ( reinterpret_cast< uintptr_t >( g::local ) + 252 );

		if ( next_think > 0 && next_think <= g::local->tick_base ( ) ) {
			next_think = -1;
			g::local->think ( );
		}

		// setup move
		memset ( movedata, 0, sizeof ( movedata ) );
		cs::i::pred->setup_move ( g::local, ucmd, cs::i::move_helper, movedata );
		cs::i::move->process_movement ( g::local, movedata );
		cs::i::pred->finish_move ( g::local, ucmd, movedata );
		
		const auto weapon = g::local->weapon ( );

		if ( weapon )
			weapon->update_accuracy ( );

		//cs::i::move_helper->process_impacts ( );

		// run post think
		//g::local->post_think ( );
	}

	void end( ucmd_t* ucmd ) {
		features::prediction::in_prediction = false;

		auto local = cs::i::ent_list->get< player_t* >( cs::i::engine->get_local_player( ) );

		if ( !cs::i::engine->is_in_game ( ) || !ucmd || !local || !local->alive ( ) )
				return;

		cs::i::move->finish_track_prediction_errors ( g::local );
		cs::i::move_helper->set_host ( nullptr );
		cs::i::move->reset ( );

		cs::i::globals->m_curtime = curtime;
		cs::i::globals->m_frametime = frametime;
		cs::i::globals->m_tickcount = tickcount;

		*reinterpret_cast< uint32_t* >( reinterpret_cast< std::uintptr_t >( local ) + 0x3338 ) = 0;
		*reinterpret_cast< int* >( prediction_seed ) = -1;
		*reinterpret_cast< int* >( prediction_player ) = 0;

		//if ( cs::i::globals->m_frametime > 0.0f )
		//	g::local->tick_base ( )++;

		cs::i::pred->m_is_first_time_predicted = first_time_pred;
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

std::array< features::prediction::netvars_t , 150> cmd_netvars {};

bool features::prediction::fix_netvars( int cmd, bool store ) {
	if ( !g::local || !g::local->alive ( ) || !cs::i::client_state ) {
		memset ( cmd_netvars.data(), 0, sizeof( cmd_netvars ) );
		return false;
	}

	auto& cur_rec = cmd_netvars[ cmd % cmd_netvars.size( ) ];

	if ( store )
		return cur_rec.store( g::local );

	if ( cur_rec.m_tick_base != 0 && g::local->tick_base( ) != cur_rec.m_tick_base )
		return false;

	return cur_rec.restore( g::local );
}

void features::prediction::run( const std::function< void( ) >& fn ) {
	prediction_util::start( g::ucmd );
	fn( );
	prediction_util::end( g::ucmd );
}

typedescription_t* find_flat_field_by_name ( datamap_t* datamap, std::string_view field_name ) {
	// E8 ?? ?? ?? ?? 89 47 24 + jmp
	//return csgo::memory::prediction::find_flat_field_by_name.as<type_description_t* ( __fastcall* )( const char* field_name, datamap_t* datamap )> ( )( field_name.data ( ), datamap );
	return nullptr;
}

void features::prediction::handle_prediction_errors ( ) {
	
}