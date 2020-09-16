#include "frame_stage_notify.hpp"
#include "../globals.hpp"
#include "../menu/menu.hpp"

#include "../features/antiaim.hpp"
#include "../features/chams.hpp"
#include "../features/glow.hpp"
#include "../features/nade_prediction.hpp"
#include "../animations/animation_system.hpp"
#include "../animations/resolver.hpp"
#include "../javascript/js_api.hpp"
#include "../menu/options.hpp"
#include "../features/prediction.hpp"
#include "../menu/d3d9_render.hpp"

void* find_hud_element( const char* name ) {
	static auto hud = pattern::search( _( "client.dll" ), _( "B9 ? ? ? ? E8 ? ? ? ? 8B 5D 08" ) ).add( 1 ).deref( ).get< void* >( );
	static auto find_hud_element_func = pattern::search( _( "client.dll" ), _( "55 8B EC 53 8B 5D 08 56 57 8B F9 33 F6 39 77 28" ) ).get< void* ( __thiscall* )( void*, const char* ) >( );
	return ( void* )find_hud_element_func( hud, name );
}

void run_preserve_death_notices( ) {
	static auto& removals = options::vars [ _( "visuals.other.removals" ) ].val.l;

	if ( !g::local || !g::local->alive( ) )
		return;

	static auto clear_death_notices = pattern::search( _( "client.dll" ), _( "55 8B EC 83 EC 0C 53 56 8B 71 58" ) ).get< void( __thiscall* )( void* ) >( );

	const auto death_notice = find_hud_element( _( "CCSGO_HudDeathNotice" ) );

	if ( death_notice ) {
		if ( g::round == round_t::in_progress ) {
			auto* local_death_notice = reinterpret_cast< float* > ( uintptr_t( death_notice ) + 80 );

			if ( local_death_notice )
				*local_death_notice = removals [ 7 ] ? FLT_MAX : 1.5f;
		}

		if ( g::round == round_t::starting && uintptr_t( death_notice ) - 20 > 0 )
			if ( clear_death_notices )
				clear_death_notices( reinterpret_cast< void* > ( uintptr_t( death_notice ) - 20 ) );
	}
}

void set_aspect_ratio( ) {
	static auto& aspect_ratio = options::vars [ _( "visuals.other.aspect_ratio" ) ].val.f;
	static auto& prop_color = options::vars [ _( "visuals.other.prop_color" ) ].val.c;

	g::cvars::r_drawstaticprops->set_value (3 );
	g::cvars::r_drawstaticprops->no_callback();

	auto as_float = aspect_ratio * 1.7777777f;

	g::cvars::r_aspectratio->set_value ( aspect_ratio * 1.7777777f );
	g::cvars::r_aspectratio->no_callback ( );
}

decltype( &hooks::frame_stage_notify ) hooks::old::frame_stage_notify = nullptr;

void __fastcall hooks::frame_stage_notify( REG, int stage ) {
	static auto& removals = options::vars [ _( "visuals.other.removals" ) ].val.l;
	static auto& world_color = options::vars [ _( "visuals.other.world_color" ) ].val.c;
	static auto& prop_color = options::vars [ _( "visuals.other.prop_color" ) ].val.c;
	static auto& ragdoll_force = options::vars [ _( "misc.effects.ragdoll_force_scale" ) ].val.f;

	g::local = ( !csgo::i::engine->is_connected( ) || !csgo::i::engine->is_in_game( ) ) ? nullptr : csgo::i::ent_list->get< player_t* >( csgo::i::engine->get_local_player( ) );

	if ( stage == 5 )
		sesui::binds::frame_time = csgo::i::globals->m_frametime;

	/* reset tickbase shift data if not in game. */
	if ( !csgo::i::engine->is_connected( ) || !csgo::i::engine->is_in_game( ) ) {
		g::dt_ticks_to_shift = 0;
		g::dt_recharge_time = 0;
	}

	security_handler::update( );

	/* fix rate problems */ {
		const auto rate = csgo::time2ticks ( 1.0f);

		g::cvars::cl_updaterate->no_callback ( );
		g::cvars::cl_cmdrate->no_callback ( );

		g::cvars::cl_updaterate->set_value ( rate );
		g::cvars::cl_cmdrate->set_value ( rate );
	}

	vec3_t old_aimpunch;
	vec3_t old_viewpunch;
	float old_flashalpha;
	float old_flashtime;

	if ( csgo::i::engine->is_in_game( ) && csgo::i::engine->is_connected( ) ) {
		anims::run( stage );

		features::prediction::update( stage );

		set_aspect_ratio ( );

		if ( stage == 5 && g::local ) {
			RUN_SAFE(
				"animations::resolver::create_beams",
				animations::resolver::create_beams( );
			);

			//RUN_SAFE(
			//	"features::nade_prediction::draw_beam",
			//	features::nade_prediction::draw_beam( );
			//);

			RUN_SAFE (
				"run_preserve_death_notices",
				run_preserve_death_notices ( );
			);

			if ( g::local->alive( ) ) {
				old_aimpunch = g::local->aim_punch( );
				old_viewpunch = g::local->view_punch( );

				if ( removals [ 3 ] )
					g::local->aim_punch( ) = vec3_t( 0.0f, 0.0f, 0.0f );

				if ( removals [ 4 ] )
					g::local->view_punch( ) = vec3_t( 0.0f, 0.0f, 0.0f );
			}

			static const std::vector< const char* > smoke_mats = {
				"particle/vistasmokev1/vistasmokev1_fire",
				"particle/vistasmokev1/vistasmokev1_smokegrenade",
				"particle/vistasmokev1/vistasmokev1_emods",
				"particle/vistasmokev1/vistasmokev1_emods_impactdust",
			};

			for ( auto mat_s : smoke_mats ) {
				const auto mat = csgo::i::mat_sys->findmat( mat_s, _( "Other textures" ) );

				if ( mat )
					mat->set_material_var_flag( 4, removals [ 0 ] );
			}

			static auto smoke_count = pattern::search( _( "client.dll" ), _( "55 8B EC 83 EC 08 8B 15 ? ? ? ? 0F 57 C0" ) ).add( 8 ).deref( ).get< int* >( );

			if ( removals [ 0 ] )
				*smoke_count = 0;

			static sesui::color last_prop_clr = prop_color;
			static sesui::color last_clr = world_color;
			static bool last_alive = false;

			if ( g::local && ( g::local->alive( ) != last_alive || ( last_clr.r != world_color.r || last_clr.g != world_color.g || last_clr.b != world_color.b || last_clr.a != world_color.a ) || ( last_prop_clr.r != prop_color.r || last_prop_clr.g != prop_color.g || last_prop_clr.b != prop_color.b || last_prop_clr.a != prop_color.a ) ) ) {
				static auto load_named_sky = pattern::search( _( "engine.dll" ), _( "55 8B EC 81 EC ? ? ? ? 56 57 8B F9 C7 45" ) ).get< void( __fastcall* )( const char* ) >( );

				load_named_sky( _( "sky_csgo_night02" ) );

				for ( auto i = csgo::i::mat_sys->first_material( ); i != csgo::i::mat_sys->invalid_material( ); i = csgo::i::mat_sys->next_material( i ) ) {
					auto mat = csgo::i::mat_sys->get_material( i );

					if ( !mat || mat->is_error_material( ) )
						continue;

					const auto is_prop = strstr( mat->get_texture_group_name( ), _( "StaticProp" ) );

					if ( is_prop ) {
						mat->color_modulate( prop_color.r * 255.0f, prop_color.g * 255.0f, prop_color.b * 255.0f );
						mat->alpha_modulate( prop_color.a * 255.0f );
					}
					else if ( strstr( mat->get_texture_group_name( ), _( "World" ) ) ) {
						mat->color_modulate( world_color.r * 255.0f, world_color.g * 255.0f, world_color.b * 255.0f );
						mat->alpha_modulate( world_color.a * 255.0f );
					}
				}

				last_prop_clr = prop_color;
				last_clr = world_color;
			}

			last_alive = g::local && g::local->alive( );

			for ( int i = 1; i <= csgo::i::ent_list->get_highest_index( ); i++ ) {
				const auto pl = csgo::i::ent_list->get < player_t* >( i );

				if ( !pl || pl->dormant( ) || !pl->client_class( ) || pl->client_class( )->m_class_id != 42 )
					continue;

				pl->force( ) *= ragdoll_force;
				pl->ragdoll_vel( ) *= ragdoll_force;
			}
		}

		if ( stage == 2 && g::local ) {
			if ( removals [ 1 ] ) {
				g::local->flash_alpha( ) = 0.0f;
				g::local->flash_duration( ) = 0.0f;
			}

			RUN_SAFE(
				"js::process_net_update_callbacks",
				js::process_net_update_callbacks( );
			);
		}

		if ( stage == 2 && g::local ) {
			RUN_SAFE(
				"js::process_net_update_end_callbacks",
				js::process_net_update_end_callbacks( );
			);
		}
	}

	old::frame_stage_notify( REG_OUT, stage );

	if ( stage == 5 && g::local && csgo::i::engine->is_in_game( ) && csgo::i::engine->is_connected( ) ) {
		if ( g::local->alive( ) ) {
			g::local->aim_punch( ) = old_aimpunch;
			g::local->view_punch( ) = old_viewpunch;
		}
	}
}