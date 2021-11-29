#include "prediction.hpp"
#include "../globals.hpp"
#include "../hooks/in_prediction.hpp"
#include "exploits.hpp"

float predicted_curtime = 0.0f;

namespace prediction_util {
	float frametime;
	float curtime;
	flags_t flags;
	vec3_t velocity;
	uint8_t movedata [ 256 ];
	uintptr_t prediction_player;
	uintptr_t prediction_seed;
	bool first_time_pred;
	bool in_pred;

	bool start ( ucmd_t* ucmd ) {
		VMP_BEGINMUTATION ( );
		features::prediction::in_prediction = true;

		if ( !cs::i::engine->is_in_game ( ) || !ucmd || !g::local || !g::local->alive ( ) )
			return false;

		static auto using_standard_weapons_in_vehicle = pattern::search ( _ ( "client.dll" ), _ ( "E8 ? ? ? ? 8B 16 8B CE 8A D8" ) ).resolve_rip ( ).get< bool ( __thiscall* )( player_t* ) > ( );
		static auto unk_func = pattern::search ( _ ( "client.dll" ), _ ( "E8 ? ? ? ? EB 11 8B 86" ) ).resolve_rip ( ).get< bool ( __thiscall* )( player_t*, bool ) > ( );

		if ( !prediction_player || !prediction_seed ) {
			prediction_seed = pattern::search ( _ ( "client.dll" ), _ ( "8B 47 40 A3" ) ).add ( 4 ).deref ( ).get< uintptr_t > ( );
			prediction_player = pattern::search ( _ ( "client.dll" ), _ ( "0F 5B C0 89 35" ) ).add ( 5 ).deref ( ).get< uintptr_t > ( );
		}

		*reinterpret_cast< uintptr_t* >( reinterpret_cast< uintptr_t >( g::local ) + N ( 0x3348 ) ) = reinterpret_cast< uintptr_t >( ucmd );
		//*reinterpret_cast< ucmd_t* >( reinterpret_cast< uintptr_t >( g::local ) + N ( 0x3298 ) ) = *ucmd;

		*reinterpret_cast< int* >( prediction_seed ) = ucmd ? ucmd->m_randseed : -1;
		*reinterpret_cast< uintptr_t* >( prediction_player ) = reinterpret_cast< uintptr_t >( g::local );

		flags = g::local->flags ( );
		features::prediction::crouch_amount = g::local->crouch_amount ( );
		features::prediction::vel = g::local->vel ( );

		/* make sure footstep sounds only play once during prediction & player is simulated */
		first_time_pred = cs::i::pred->m_is_first_time_predicted;
		in_pred = cs::i::pred->m_in_prediction;
		
		cs::i::pred->m_is_first_time_predicted = false;
		cs::i::pred->m_in_prediction = true;

		curtime = cs::i::globals->m_curtime;
		frametime = cs::i::globals->m_frametime;

		cs::i::globals->m_curtime = predicted_curtime = cs::ticks2time ( g::local->tick_base ( ) );
		cs::i::globals->m_frametime = *reinterpret_cast< bool* >( reinterpret_cast< uintptr_t >( cs::i::pred ) + 10 ) ? 0.0f : cs::i::globals->m_ipt;

		/* forced buttons */
		ucmd->m_buttons |= *reinterpret_cast< buttons_t* > ( reinterpret_cast< uintptr_t >( g::local ) + N ( 0x3344 ) );
		ucmd->m_buttons &= ~*reinterpret_cast< buttons_t* > ( reinterpret_cast< uintptr_t >( g::local ) + N ( 0x3340 ) );

		cs::i::move->start_track_prediction_errors ( g::local );

		/* weapon selection */
		if ( ucmd->m_weaponselect > 0 && ucmd->m_weaponselect < 0x2000 ) {
			/* TODO */
			const auto weapon = cs::i::ent_list->get<weapon_t*> ( ucmd->m_weaponselect );

			//if ( weapon ) {
			//	const auto weapon_data = weapon->data ( );
			//
			//	if ( weapon_data )
			//		g::local->select_item ( weapon_data->m_weapon_name, ucmd->m_weaponsubtype );
			//}

			if ( weapon )
			{
				auto v10 = ( *( int ( __thiscall** )( weapon_t* ) )( *( DWORD* ) weapon + 28 ) )( weapon );
				if ( v10 )
				{
					auto v11 = ( *( int ( __thiscall** )( int ) )( *( DWORD* ) v10 + 668 ) )( v10 );
					auto v12 = v11;
					if ( v11 )
					{
						if ( ( *( int ( __thiscall** )( int ) )( *( DWORD* ) v11 + 1128 ) )( v11 ) == *( DWORD* ) ( reinterpret_cast< uintptr_t >( ucmd ) + 60 ) )
							( *( void ( __thiscall** )( player_t*, int ) )( *( DWORD* ) g::local + 1316 ) )( g::local, v12 );
					}
				}
			}
		}

		const auto vehicle = g::local->vehicle( );

		/* Vehicle impulse */
		if ( ucmd->m_impulse && ( !vehicle || using_standard_weapons_in_vehicle(g::local) ) )
			*reinterpret_cast< uint32_t* > ( reinterpret_cast< uintptr_t >( g::local ) + 0x320C ) = ucmd->m_impulse;

		/* UpdateButtonState */
		const auto v16 = ucmd->m_buttons;
		const auto v17 = v16 ^ *reinterpret_cast< buttons_t* > ( reinterpret_cast< uintptr_t >( g::local ) + 0x3208 );

		*reinterpret_cast< buttons_t* > ( reinterpret_cast< uintptr_t >( g::local ) + 0x31FC ) = *reinterpret_cast< buttons_t* > ( reinterpret_cast< uintptr_t >( g::local ) + 0x3208 );
		*reinterpret_cast< buttons_t* > ( reinterpret_cast< uintptr_t >( g::local ) + 0x3208 ) = v16;
		*reinterpret_cast< buttons_t* > ( reinterpret_cast< uintptr_t >( g::local ) + 0x3200 ) = v16 & v17;
		*reinterpret_cast< buttons_t* > ( reinterpret_cast< uintptr_t >( g::local ) + 0x3204 ) = v17 & ~v16;

		cs::i::pred->check_moving_ground ( g::local, cs::i::globals->m_frametime );
		
		//g::local->set_local_viewangles ( ucmd->m_angs );

		if ( g::local->physics_run_think ( 0 ) )
			g::local->pre_think ( );

		auto& next_think = *reinterpret_cast< uint32_t* > ( reinterpret_cast< uintptr_t >( g::local ) + N ( 0xFC ) );

		if ( next_think != -1 && next_think > 0 && next_think <= g::local->tick_base ( ) ) {
			next_think = -1;

			unk_func ( g::local, 0 );
			g::local->think ( );
		}

		cs::i::pred->setup_move ( g::local, ucmd, cs::i::move_helper, movedata );

		/* process movement */
		if ( !vehicle )
			cs::i::move->process_movement ( g::local, movedata );
		else
			vfunc< void ( __thiscall* )( entity_t*, player_t*, void* ) > ( vehicle, 5 ) ( vehicle, g::local, movedata );

		cs::i::pred->finish_move ( g::local, ucmd, movedata );

		const auto weapon = g::local->weapon ( );

		if ( weapon )
			weapon->update_accuracy ( );

		return true;
		VMP_END ( );
	}

	bool end ( ucmd_t* ucmd ) {
		VMP_BEGINMUTATION ( );
		features::prediction::in_prediction = false;

		if ( !cs::i::engine->is_in_game ( ) || !ucmd || !g::local || !g::local->alive ( ) )
			return false;

		cs::i::move->finish_track_prediction_errors ( g::local );
		cs::i::move->reset ( );

		cs::i::move_helper->set_host ( nullptr );

		*reinterpret_cast< uint32_t* >( reinterpret_cast< uintptr_t >( g::local ) + 0x3348 ) = 0;

		*reinterpret_cast< int* >( prediction_seed ) = -1;
		*reinterpret_cast< int* >( prediction_player ) = 0;

		cs::i::globals->m_curtime = curtime;
		cs::i::globals->m_frametime = frametime;

		cs::i::pred->m_is_first_time_predicted = first_time_pred;
		cs::i::pred->m_in_prediction = in_pred;

		return true;
		VMP_END ( );
	}
}

void features::prediction::force_repredict ( ) {
	*reinterpret_cast< int* > ( reinterpret_cast< uintptr_t >( cs::i::pred ) + 0xC ) = -1;
	*reinterpret_cast< int* > ( reinterpret_cast< uintptr_t >( cs::i::pred ) + 0x1C ) = 0;
	*reinterpret_cast< int* > ( reinterpret_cast< uintptr_t >( cs::i::pred ) + 0x24 ) = 1;
}

float features::prediction::curtime ( ) {
	return predicted_curtime;
}

void features::prediction::update ( int stage ) {
	if ( !cs::i::engine->is_in_game ( ) || !cs::i::engine->is_connected ( ) || stage != 4 )
		return;

	//cs::i::pred->update (
	//	cs::i::client_state->delta_tick ( ),
	//	cs::i::client_state->delta_tick ( ) > 0,
	//	cs::i::client_state->last_command_ack ( ),
	//	cs::i::client_state->last_outgoing_cmd ( ) + cs::i::client_state->choked ( )
	//);
}

void features::prediction::fix_viewmodel ( bool store ) {
	if ( !g::local || !g::local->alive ( ) || g::local->viewmodel_handle ( ) == 0xFFFFFFFF )
		return;

	const auto viewmodel = cs::i::ent_list->get_by_handle< weapon_t* > ( g::local->viewmodel_handle ( ) );

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

std::array< features::prediction::netvars_t, 150> cmd_netvars {};

bool features::prediction::fix_netvars ( int cmd, bool store ) {
	if ( !g::local || !g::local->alive ( ) || !cs::i::client_state ) {
		memset ( cmd_netvars.data ( ), 0, sizeof ( cmd_netvars ) );
		return false;
	}

	auto& cur_rec = cmd_netvars [ cmd % cmd_netvars.size ( ) ];

	if ( store )
		return cur_rec.store ( g::local );

	if ( cur_rec.m_tick_base != 0 && g::local->tick_base ( ) != cur_rec.m_tick_base )
		return false;

	return cur_rec.restore ( g::local );
}

void features::prediction::run ( const std::function< void ( ) >& fn ) {
	prediction_util::start ( g::ucmd );
	fn ( );
	prediction_util::end ( g::ucmd );
}

typedescription_t* find_flat_field_by_name ( datamap_t* datamap, std::string_view field_name ) {
	// E8 ?? ?? ?? ?? 89 47 24 + jmp
	//return csgo::memory::prediction::find_flat_field_by_name.as<type_description_t* ( __fastcall* )( const char* field_name, datamap_t* datamap )> ( )( field_name.data ( ), datamap );
	return nullptr;
}

void features::prediction::handle_prediction_errors ( ) {

}