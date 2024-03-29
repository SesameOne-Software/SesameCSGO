﻿#include "events.hpp"
#include "../globals.hpp"

#include "../animations/resolver.hpp"

#include "../features/skinchanger.hpp"

hooks::c_event_handler::c_event_handler ( ) {
	cs::i::events->add_listener ( this, _ ( "player_death" ), false );
	cs::i::events->add_listener ( this, _ ( "weapon_fire" ), false );
	cs::i::events->add_listener ( this, _ ( "player_say" ), false );
	cs::i::events->add_listener ( this, _ ( "player_hurt" ), false );
	cs::i::events->add_listener ( this, _ ( "round_freeze_end" ), false );
	cs::i::events->add_listener ( this, _ ( "round_start" ), false );
	cs::i::events->add_listener ( this, _ ( "round_end" ), false );
	cs::i::events->add_listener ( this, _ ( "bullet_impact" ), true );
}

hooks::c_event_handler::~c_event_handler ( ) {
	cs::i::events->remove_listener ( this );
}

void hooks::c_event_handler::fire_game_event ( event_t* event ) {
	if ( !event || !g::local )
		return;

	//if ( !strcmp( event->get_name( ), _( "weapon_fire" ) ) )
	//	features::lagcomp::cache_shot( event );

	if ( !strcmp ( event->get_name ( ), _ ( "player_death" ) ) )
		features::skinchanger::process_death ( event );

	if ( !strcmp ( event->get_name ( ), _ ( "player_hurt" ) ) )
		anims::resolver::process_hurt ( event );

	if ( !strcmp ( event->get_name ( ), _ ( "bullet_impact" ) ) );
		anims::resolver::process_impact( event );

	if ( !strcmp ( event->get_name ( ), _ ( "round_freeze_end" ) ) )
		g::round = round_t::in_progress;

	if ( !strcmp ( event->get_name ( ), _ ( "round_start" ) ) )
		g::round = round_t::starting;

	if ( !strcmp ( event->get_name ( ), _ ( "round_end" ) ) )
		g::round = round_t::ending;
}

int hooks::c_event_handler::get_event_debug_id ( ) {
	return 42;
}