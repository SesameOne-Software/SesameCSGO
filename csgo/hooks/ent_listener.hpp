#pragma once
#include "../sdk/sdk.hpp"

#include "../animations/animation_system.hpp"

#include "../features/lagcomp.hpp"

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
	anims::player_data [ idx ].spawn_times = 0.0f;
	anims::player_data [ idx ].last_update = 0.0f;
	anims::player_data [ idx ].last_origin = {};
	anims::player_data [ idx ].old_origin = {};
	anims::player_data [ idx ].last_velocity = {};
	anims::player_data [ idx ].old_velocity = {};

	anims::choked_commands [ idx ] = 0;
	anims::desync_sign [ idx ] = 0.0f;
	anims::client_feet_playback_rate [ idx ] = 0.0f;
	anims::feet_playback_rate [ idx ] = 0.0f;

	if(!anims::old_animlayers [ idx ] .empty())
		anims::old_animlayers [ idx ].clear ( );

	if ( !anims::frames [ idx ].empty ( ) )
	anims::frames [ idx ].clear ( );

	/* lagcomp */
	if ( !features::lagcomp::data::records [ idx ].empty ( ) )
		features::lagcomp::data::records [ idx ].clear ( );

	features::lagcomp::data::cham_records [ idx ].m_pl = nullptr;

	if ( !features::lagcomp::data::all_records [ idx ].empty ( ) )
		features::lagcomp::data::all_records [ idx ].clear ( );

	features::lagcomp::data::shot_records [ idx ].m_pl = nullptr;

	if ( !features::lagcomp::data::extrapolated_records [ idx ].empty ( ) )
		features::lagcomp::data::extrapolated_records [ idx ].clear ( );

	//dbg_print ( _("clear callback called!\n") );
}

void c_entity_listener_mgr::on_entity_created ( void* ent ) {
	if ( ent ) {
		auto pl = ( player_t* ) ent;

		if ( pl->is_player ( ) ) {
			clear_shit ( pl->idx() );
		}
	}
}

void c_entity_listener_mgr::on_entity_deleted ( void* ent ) {
	if ( ent ) {
		auto pl = ( player_t* ) ent;

		if ( pl->is_player ( ) ) {
			clear_shit ( pl->idx ( ) );
		}
	}
}

extern std::unique_ptr< c_entity_listener_mgr > ent_listener;