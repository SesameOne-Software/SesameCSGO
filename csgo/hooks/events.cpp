#include "events.hpp"
#include "../globals.hpp"

#include "../animations/resolver.hpp"

#include "../features/skinchanger.hpp"

hooks::c_event_handler::c_event_handler ( ) {
	cs::i::events->add_listener ( this, _ ( "player_death" ), false );
	cs::i::events->add_listener ( this, _ ( "weapon_fire" ), false );
	cs::i::events->add_listener ( this, _ ( "player_say" ), false );
	cs::i::events->add_listener ( this, _ ( "player_hurt" ), false );
	cs::i::events->add_listener ( this, _ ( "bullet_impact" ), true );
	cs::i::events->add_listener ( this, _ ( "round_freeze_end" ), false );
	cs::i::events->add_listener ( this, _ ( "round_start" ), false );
	cs::i::events->add_listener ( this, _ ( "round_end" ), false );
}

hooks::c_event_handler::~c_event_handler ( ) {
	cs::i::events->remove_listener ( this );
}

void process_hurt_ex ( event_t* event ) {
	RUN_SAFE (
		"animations::resolver::process_hurt",
		anims::resolver::process_hurt ( event );
	);
}

void process_impact_ex ( event_t* event ) {
	RUN_SAFE (
		"animations::resolver::process_impact",
		anims::resolver::process_impact ( event );
	);
}

void hooks::c_event_handler::fire_game_event ( event_t* event ) {
	if ( !event || !g::local )
		return;
	MUTATE_START

	//if ( !strcmp( event->get_name( ), _( "weapon_fire" ) ) )
	//	features::lagcomp::cache_shot( event );

		if ( !strcmp ( event->get_name ( ), _ ( "player_death" ) ) )
			/* translator::translate( ); */;

	if ( !strcmp ( event->get_name ( ), _ ( "player_say" ) ) )
		features::skinchanger::process_death(event);

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
	MUTATE_END
}

int hooks::c_event_handler::get_event_debug_id ( ) {
	return 42;
}

std::unique_ptr< hooks::c_event_handler > hooks::event_handler;