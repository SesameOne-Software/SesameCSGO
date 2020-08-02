#include "frame_stage_notify.hpp"
#include "../globals.hpp"
#include "../menu/menu.hpp"

#include "../features/antiaim.hpp"
#include "../features/chams.hpp"
#include "../features/glow.hpp"
#include "../features/nade_prediction.hpp"
#include "../animations/animations.hpp"
#include "../animations/resolver.hpp"
#include "../javascript/js_api.hpp"

void* find_hud_element ( const char* name ) {
	static auto hud = pattern::search ( _ ( "client.dll" ), _ ( "B9 ? ? ? ? E8 ? ? ? ? 8B 5D 08" ) ).add ( 1 ).deref ( ).get< void* > ( );
	static auto find_hud_element_func = pattern::search ( _ ( "client.dll" ), _ ( "55 8B EC 53 8B 5D 08 56 57 8B F9 33 F6 39 77 28" ) ).get< void* ( __thiscall* )( void*, const char* ) > ( );
	return ( void* ) find_hud_element_func ( hud, name );
}

void run_preserve_death_notices ( ) {
	OPTION ( bool, preserve_killfeed, "Sesame->C->Other->Removals->Preserve Killfeed", oxui::object_checkbox );

	if ( !g::local || !g::local->alive ( ) )
		return;

	static auto clear_death_notices = pattern::search ( _ ( "client.dll" ), _ ( "55 8B EC 83 EC 0C 53 56 8B 71 58" ) ).get< void ( __thiscall* )( void* ) > ( );

	const auto death_notice = find_hud_element ( _ ( "CCSGO_HudDeathNotice" ) );

	if ( death_notice ) {
		if ( g::round == round_t::in_progress ) {
			auto* local_death_notice = reinterpret_cast< float* > ( uintptr_t ( death_notice ) + 80 );

			if ( local_death_notice )
				*local_death_notice = preserve_killfeed ? FLT_MAX : 1.5f;
		}

		if ( g::round == round_t::starting && uintptr_t ( death_notice ) - 20 > 0 )
			if ( clear_death_notices )
				clear_death_notices ( reinterpret_cast< void* > ( uintptr_t ( death_notice ) - 20 ) );
	}
}

void set_aspect_ratio ( ) {
	OPTION ( oxui::color, prop_clr, "Sesame->C->Other->World->Prop Color", oxui::object_colorpicker );
	static auto r_drawstaticprops = pattern::search ( _ ( "engine.dll" ), _ ( "B9 ? ? ? ? FF 50 34 85 C0 0F 84 ? ? ? ? 8B 0D ? ? ? ? 81 F9 ? ? ? ? 75 0C A1 ? ? ? ? 35 ? ? ? ? EB 05 8B 01 FF 50 34 83 F8 02" ) ).add ( 1 ).deref ( ).get< void* > ( );
	auto fval = ( prop_clr.r == 255 && prop_clr.g == 255 && prop_clr.b == 255 && prop_clr.a == 255 ) ? 1.0f : 3.0f;
	*reinterpret_cast< uintptr_t* > ( uintptr_t ( r_drawstaticprops ) + 0x2c ) = *reinterpret_cast < uintptr_t* > ( uintptr_t ( &fval ) ) ^ uintptr_t ( r_drawstaticprops );
	*reinterpret_cast< uintptr_t* > ( uintptr_t ( r_drawstaticprops ) + 0x2c + 0x4 ) = static_cast < int > ( fval ) ^ uintptr_t ( r_drawstaticprops );

	OPTION ( double, aspect_ratio, "Sesame->C->Other->Removals->Custom Aspect Ratio", oxui::object_slider );
	static auto r_aspect_ratio = pattern::search ( _ ( "engine.dll" ), _ ( "81 F9 ? ? ? ? 75 16 F3 0F 10 0D ? ? ? ? F3 0F 11 4D ? 81 75 ? ? ? ? ? EB 18 8B 01 8B 40 30 FF D0 F3 0F 10 0D ? ? ? ? 8B 0D ? ? ? ? D9 5D FC F3 0F 10 45 ? 0F 2F 05 ? ? ? ? 76 34" ) ).add ( 2 ).deref ( ).get< void* > ( );
	auto as_float = static_cast< float > ( aspect_ratio ) * 1.7777777f;
	*reinterpret_cast< uintptr_t* > ( uintptr_t ( r_aspect_ratio ) + 0x2c ) = *reinterpret_cast< uintptr_t* > ( &as_float ) ^ uintptr_t ( r_aspect_ratio );
}

decltype( &hooks::frame_stage_notify ) hooks::old::frame_stage_notify = nullptr;

void __fastcall hooks::frame_stage_notify ( REG, int stage ) {
	OPTION ( bool, no_smoke, "Sesame->C->Other->Removals->No Smoke", oxui::object_checkbox );
	OPTION ( bool, no_flash, "Sesame->C->Other->Removals->No Flash", oxui::object_checkbox );
	OPTION ( bool, no_aimpunch, "Sesame->C->Other->Removals->No Aimpunch", oxui::object_checkbox );
	OPTION ( bool, no_viewpunch, "Sesame->C->Other->Removals->No Viewpunch", oxui::object_checkbox );
	OPTION ( oxui::color, world_clr, "Sesame->C->Other->World->World Color", oxui::object_colorpicker );
	OPTION ( oxui::color, prop_clr, "Sesame->C->Other->World->Prop Color", oxui::object_colorpicker );
	OPTION ( double, ragdoll_force, "Sesame->E->Effects->Main->Ragdoll Force", oxui::object_slider );

	g::local = ( !csgo::i::engine->is_connected ( ) || !csgo::i::engine->is_in_game ( ) ) ? nullptr : csgo::i::ent_list->get< player_t* > ( csgo::i::engine->get_local_player ( ) );

	/* reset tickbase shift data if not in game. */
	if ( !csgo::i::engine->is_connected ( ) || !csgo::i::engine->is_in_game ( ) ) {
		g::dt_ticks_to_shift = 0;
		g::dt_recharge_time = 0;
	}

	security_handler::update ( );

	vec3_t old_aimpunch;
	vec3_t old_viewpunch;
	float old_flashalpha;

	animations::run ( stage );

	RUN_SAFE (
		"set_aspect_ratio",
		set_aspect_ratio ( );
	);

	if ( stage == 5 && g::local ) {
		RUN_SAFE (
			"animations::resolver::create_beams",
			animations::resolver::create_beams ( );
		);

		RUN_SAFE (
			"features::nade_prediction::draw_beam",
			features::nade_prediction::draw_beam ( );
		);

		RUN_SAFE (
			"run_preserve_death_notices",
			run_preserve_death_notices ( );
		);

		if ( g::local->alive ( ) ) {
			old_aimpunch = g::local->aim_punch ( );
			old_viewpunch = g::local->view_punch ( );

			if ( no_aimpunch )
				g::local->aim_punch ( ) = vec3_t ( 0.0f, 0.0f, 0.0f );

			if ( no_viewpunch )
				g::local->view_punch ( ) = vec3_t ( 0.0f, 0.0f, 0.0f );
		}

		static const std::vector< const char* > smoke_mats = {
			"particle/vistasmokev1/vistasmokev1_fire",
			"particle/vistasmokev1/vistasmokev1_smokegrenade",
			"particle/vistasmokev1/vistasmokev1_emods",
			"particle/vistasmokev1/vistasmokev1_emods_impactdust",
		};

		for ( auto mat_s : smoke_mats ) {
			const auto mat = csgo::i::mat_sys->findmat ( mat_s, _ ( "Other textures" ) );
			mat->set_material_var_flag ( 4, no_smoke );
		}

		static auto smoke_count = pattern::search ( _ ( "client.dll" ), _ ( "55 8B EC 83 EC 08 8B 15 ? ? ? ? 0F 57 C0" ) ).add ( 8 ).deref ( ).get< int* > ( );

		if ( no_smoke )
			*smoke_count = 0;

		static oxui::color last_prop_clr = prop_clr;
		static oxui::color last_clr = world_clr;
		static bool last_alive = false;

		RUN_SAFE (
			"run_world_color_modulation",
			if ( g::local && ( g::local->alive ( ) != last_alive || ( last_clr.r != world_clr.r || last_clr.g != world_clr.g || last_clr.b != world_clr.b || last_clr.a != world_clr.a ) || ( last_prop_clr.r != prop_clr.r || last_prop_clr.g != prop_clr.g || last_prop_clr.b != prop_clr.b || last_prop_clr.a != prop_clr.a ) ) ) {
				static auto load_named_sky = pattern::search ( _ ( "engine.dll" ), _ ( "55 8B EC 81 EC ? ? ? ? 56 57 8B F9 C7 45" ) ).get< void ( __fastcall* )( const char* ) > ( );

				load_named_sky ( _ ( "sky_csgo_night02" ) );

				for ( auto i = csgo::i::mat_sys->first_material ( ); i != csgo::i::mat_sys->invalid_material ( ); i = csgo::i::mat_sys->next_material ( i ) ) {
					auto mat = csgo::i::mat_sys->get_material ( i );

					if ( !mat || mat->is_error_material ( ) )
						continue;

					const auto is_prop = strstr ( mat->get_texture_group_name ( ), _ ( "StaticProp" ) );

					if ( is_prop ) {
						mat->color_modulate ( prop_clr.r, prop_clr.g, prop_clr.b );
						mat->alpha_modulate ( prop_clr.a );
					}
					else if ( strstr ( mat->get_texture_group_name ( ), _ ( "World" ) ) ) {
						mat->color_modulate ( world_clr.r, world_clr.g, world_clr.b );
						mat->alpha_modulate ( world_clr.a );
					}
				}

				last_prop_clr = prop_clr;
				last_clr = world_clr;
			}

		last_alive = g::local && g::local->alive ( );
		);

		RUN_SAFE(
			"run_scale_ragdoll_force",
		for ( int i = 1; i <= 200; i++ ) {
			const auto pl = csgo::i::ent_list->get < player_t* > ( i );

			if ( !pl || pl->dormant ( ) || !pl->client_class ( ) || pl->client_class ( )->m_class_id != 42 )
				continue;

			pl->force ( ) *= ragdoll_force;
			pl->ragdoll_vel ( ) *= ragdoll_force;
		}
		);
	}

	if ( stage == 2 && g::local ) {
		if ( no_flash )
			g::local->flash_alpha ( ) = 0.0f;

		RUN_SAFE (
			"js::process_net_update_callbacks",
			js::process_net_update_callbacks ( );
		);
	}

	if ( stage == 2 && g::local ) {
		RUN_SAFE (
			"js::process_net_update_end_callbacks",
			js::process_net_update_end_callbacks ( );
		);
	}

	old::frame_stage_notify ( REG_OUT, stage );

	if ( stage == 5 && g::local ) {
		if ( g::local->alive ( ) ) {
			g::local->aim_punch ( ) = old_aimpunch;
			g::local->view_punch ( ) = old_viewpunch;
		}
	}
}