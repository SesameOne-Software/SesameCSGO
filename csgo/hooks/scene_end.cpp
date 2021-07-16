#include <deque>

#include "scene_end.hpp"
#include "../globals.hpp"

#include "../features/antiaim.hpp"
#include "../features/chams.hpp"
#include "../features/glow.hpp"
#include "../animations/resolver.hpp"

#include "../menu/options.hpp"

extern std::deque< anims::resolver::hit_matrix_rec_t > hit_matrix_rec;
extern anims::resolver::hit_matrix_rec_t cur_hit_matrix_rec;

decltype( &hooks::scene_end ) hooks::old::scene_end = nullptr;

void __fastcall hooks::scene_end ( REG ) {
	MUTATE_START
	static auto& fog = options::vars [ _ ( "visuals.other.fog" ) ].val.b;
	static auto& bloom = options::vars [ _ ( "visuals.other.bloom" ) ].val.b;

	static auto& fog_color = options::vars [ _ ( "visuals.other.fog_color" ) ].val.c;
	static auto& fog_distance = options::vars [ _ ( "visuals.other.fog_distance" ) ].val.f;
	static auto& fog_density = options::vars [ _ ( "visuals.other.fog_density" ) ].val.f;
	static auto& bloom_scale = options::vars [ _ ( "visuals.other.bloom_scale" ) ].val.f;
	static auto& bloom_exponent = options::vars [ _ ( "visuals.other.bloom_exponent" ) ].val.f;
	static auto& bloom_saturation = options::vars [ _ ( "visuals.other.bloom_saturation" ) ].val.f;

	old::scene_end ( REG_OUT );

	if ( cs::i::client_state && g::local && g::local->alive ( ) && !cs::i::client_state->choked ( ) )
		features::chams::old_origin = g::local->origin ( );

	for ( auto i = 1; i <= cs::i::ent_list->get_highest_index ( ); i++ ) {
		const auto ent = cs::i::ent_list->get < entity_t* > ( i );

		if ( !ent || !ent->client_class ( ) )
			continue;

		switch ( ent->client_class ( )->m_class_id ) {
		case 69: {
			/*
			const auto v1 = ent->use_auto_exposure_min ( ); // true
			const auto v2 = ent->use_auto_exposure_max ( ); // true
			const auto v3 = ent->use_bloom_scale ( ); // true
			const auto v4 = ent->auto_exposure_min ( ); // 0.8
			const auto v5 = ent->auto_exposure_max ( ); // 1.3
			const auto v6 = ent->bloom_scale ( ); // 0.2
			const auto v7 = ent->bloom_scale_min ( ); // 0.2
			const auto v8 = ent->bloom_exponent ( ); // 2.5
			const auto v9 = ent->bloom_saturation ( ); // 1.0
			*/
			ent->use_bloom_scale ( ) = bloom;
			ent->bloom_scale ( ) = bloom ? bloom_scale : 0.2f;
			ent->bloom_exponent ( ) = bloom ? bloom_exponent : 2.5f;
			ent->bloom_saturation ( ) = bloom ? bloom_saturation : 1.0f;
		} break;
		case 78: {
			/*
			const auto v1 = ent->fog_enable ( ); // true
			const auto v2 = ent->fog_blend ( ); // false
			const auto v3 = ent->fog_color_primary ( ); // 0xffb1c7c2
			const auto v4 = ent->fog_color_secondary ( ); // 0xffffffff
			const auto v5 = ent->fog_start ( ); // 512.0
			const auto v6 = ent->fog_end ( ); // 6000.0
			const auto v7 = ent->fog_far_z ( ); // -1.0
			const auto v8 = ent->fog_max_density ( ); // 0.4
			*/
			union color_data {
				color_data ( int r, int g, int b, int a ) {
					view.r = r;
					view.g = g;
					view.b = b;
					view.a = a;
				}

				struct {
					uint8_t r, g, b, a;
				} view;

				uint32_t raw;
			};

			ent->fog_enable ( ) = fog;
			ent->fog_color_primary ( ) = fog ? color_data ( fog_color.r * 255.0f, fog_color.g * 255.0f, fog_color.b * 255.0f, fog_color.a * 255.0f ).raw : 0xffb1c7c2;
			ent->fog_color_secondary ( ) = fog ? color_data ( fog_color.r * 255.0f, fog_color.g * 255.0f, fog_color.b * 255.0f, fog_color.a * 255.0f ).raw : 0xffffffff;
			ent->fog_start ( ) = fog ? 100.0f : 512.0f;
			ent->fog_end ( ) = fog ? fog_distance : 600.0f;
			ent->fog_max_density ( ) = fog ? fog_density : 0.4f;
		} break;
		}
	}

	features::chams::render_shots ( );
	/*
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
	}*/

	RUN_SAFE(
		"features::glow::cache_entities",
		features::glow::cache_entities( );
	);

	MUTATE_END
}