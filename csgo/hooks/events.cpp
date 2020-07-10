#include "events.hpp"
#include "../globals.hpp"

#include "../animations/resolver.hpp"

hooks::c_event_handler::c_event_handler ( ) {
	csgo::i::events->add_listener ( this, _ ( "weapon_fire" ), false );
	csgo::i::events->add_listener ( this, _ ( "player_say" ), false );
	csgo::i::events->add_listener ( this, _ ( "player_hurt" ), false );
	csgo::i::events->add_listener ( this, _ ( "bullet_impact" ), true );
	csgo::i::events->add_listener ( this, _ ( "round_freeze_end" ), false );
	csgo::i::events->add_listener ( this, _ ( "round_start" ), false );
	csgo::i::events->add_listener ( this, _ ( "round_end" ), false );
}

hooks::c_event_handler::~c_event_handler ( ) {
	csgo::i::events->remove_listener ( this );
}

void process_hurt_ex ( event_t* event ) {
	RUN_SAFE (
		"animations::resolver::process_hurt",
		animations::resolver::process_hurt ( event );
	);
}

void process_impact_ex ( event_t* event ) {
	RUN_SAFE (
		"animations::resolver::process_impact",
		animations::resolver::process_impact ( event );
	);
}

void hooks::c_event_handler::fire_game_event ( event_t* event ) {
	if ( !event || !g::local )
		return;

	//if ( !strcmp( event->get_name( ), _( "weapon_fire" ) ) )
	//	features::lagcomp::cache_shot( event );

	if ( !strcmp ( event->get_name ( ), _ ( "player_say" ) ) )
		/* translator::translate( ); */;

	if ( !strcmp ( event->get_name ( ), _ ( "player_hurt" ) ) )
		process_hurt_ex ( event );

	if ( !strcmp ( event->get_name ( ), _ ( "bullet_impact" ) ) )
		process_impact_ex ( event );

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

std::unique_ptr< hooks::c_event_handler > hooks::event_handler;