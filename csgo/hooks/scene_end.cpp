#include <deque>

#include "scene_end.hpp"
#include "../globals.hpp"

#include "../features/antiaim.hpp"
#include "../features/chams.hpp"
#include "../features/glow.hpp"
#include "../animations/resolver.hpp"

extern std::deque< animations::resolver::hit_matrix_rec_t > hit_matrix_rec;
extern animations::resolver::hit_matrix_rec_t cur_hit_matrix_rec;

decltype( &hooks::scene_end ) hooks::old::scene_end = nullptr;

void __fastcall hooks::scene_end( REG ) {
	old::scene_end( REG_OUT );

	if ( !g::local || !g::local->alive( ) )
		hit_matrix_rec.clear( );

	if ( g::local && g::local->alive( ) && g::send_packet )
		features::chams::old_origin = g::local->origin( );

	if ( g::local ) {
		for ( auto i = 1; i <= csgo::i::ent_list->get_highest_index( ); i++ ) {
			const auto ent = csgo::i::ent_list->get < entity_t* >( i );

			if ( !ent || !ent->client_class( ) || ( ent->client_class( )->m_class_id != 40 && ent->client_class( )->m_class_id != 42 ) )
				continue;

			static auto off_player_handle = netvars::get_offset( _( "DT_CSRagdoll->m_hPlayer" ) );

			auto pl_idx = -1;

			if ( ent->client_class( )->m_class_id == 42 )
				pl_idx = *reinterpret_cast< uint32_t* > ( uintptr_t( ent ) + off_player_handle ) & 0xfff;
			else
				pl_idx = ent->idx( );

			if ( pl_idx != -1 && pl_idx ) {
				std::vector< animations::resolver::hit_matrix_rec_t > hit_matricies { };

				if ( !hit_matrix_rec.empty( ) )
					for ( auto& hit : hit_matrix_rec )
						if ( hit.m_pl == pl_idx - 1 )
							hit_matricies.push_back( hit );

				if ( !hit_matricies.empty( ) ) {
					for ( auto& hit : hit_matricies ) {
						cur_hit_matrix_rec = hit;

						features::chams::in_model = true;
						ent->draw( );
						features::chams::in_model = false;
					}
				}
			}
		}
	}

	RUN_SAFE(
		"features::glow::cache_entities",
		features::glow::cache_entities( );
	);
}