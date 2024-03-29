﻿#include "in_prediction.hpp"
#include "../globals.hpp"

bool hooks::prediction::disable_sounds = false;

decltype( &hooks::in_prediction ) hooks::old::in_prediction = nullptr;

bool __fastcall hooks::in_prediction( REG ) {
	static const auto return_to_maintain_sequence_transitions = pattern::search( _("client.dll"), _("84 C0 74 17 8B 87") ).get < void* >( );
	
	if ( !cs::i::engine->is_in_game( ) || !cs::i::engine->is_connected( ) )
		return old::in_prediction( REG_OUT );

	if ( _ReturnAddress( ) == return_to_maintain_sequence_transitions )
		return false;

	//if ( _ReturnAddress( ) == return_to_play_step_sound && hooks::prediction::disable_sounds )
	//	return true;

	return old::in_prediction( REG_OUT );
}