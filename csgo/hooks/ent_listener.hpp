#pragma once
#include "../sdk/sdk.hpp"

#include "../animations/anims.hpp"

#include "../features/lagcomp.hpp"
#include "../features/ragebot.hpp"

/* CREDITS CHAMBERS */

class c_entity_listener {
public:
	virtual void on_entity_created ( void* ent ) = 0;
	virtual void on_entity_deleted ( void* ent ) = 0;

	void add ( );
};

void c_entity_listener::add ( ) {
	static auto add_listener_ent = pattern::search ( _("client.dll"), _ ( "55 8B EC 8B 0D ? ? ? ? 33 C0 56 85 C9 7E 32 8B 55 08 8B 35" ) ).get<void ( __stdcall* )( c_entity_listener* )> ( );
	add_listener_ent ( this );
}

class c_entity_listener_mgr : public c_entity_listener {
	virtual void on_entity_created ( void* ent );
	virtual void on_entity_deleted ( void* ent );
};

static void clear_shit ( int idx ) {
	if ( !idx || idx > cs::i::globals->m_max_clients )
		return;

	/* anims */
	anims::reset_data ( idx );

	features::ragebot::get_hits ( idx ) = 0;
	features::ragebot::get_misses ( idx ) = { 0, 0, 0 };

	/* lagcomp */
	if ( !features::lagcomp::data::records [ idx ].empty ( ) )
		features::lagcomp::data::records [ idx ].clear ( );

	if ( !features::lagcomp::data::visual_records [ idx ].empty ( ) )
		features::lagcomp::data::visual_records [ idx ].clear ( );

	features::lagcomp::data::old_origins [ idx ] = vec3_t ( 0.0f, 0.0f, 0.0f );
	features::lagcomp::data::shot_count [ idx ] = 0;

	//dbg_print ( _("clear callback called!\n") );
}

void c_entity_listener_mgr::on_entity_created ( void* ent ) {
	if ( ent ) {
		auto pl = reinterpret_cast< player_t* >( ent );

		if ( pl->is_player ( ) )
			clear_shit ( pl->idx() );
	}
}

void c_entity_listener_mgr::on_entity_deleted ( void* ent ) {
	if ( ent ) {
		auto pl = reinterpret_cast<player_t*>(ent);

		if ( pl->is_player ( ) )
			clear_shit ( pl->idx ( ) );
	}
}

extern std::unique_ptr< c_entity_listener_mgr > ent_listener;