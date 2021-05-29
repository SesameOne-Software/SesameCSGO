﻿#include "frame_stage_notify.hpp"
#include "../globals.hpp"
#include "../menu/menu.hpp"

#include "../features/antiaim.hpp"
#include "../features/chams.hpp"
#include "../features/glow.hpp"
#include "../features/nade_prediction.hpp"
#include "../animations/anims.hpp"
#include "../animations/resolver.hpp"
#include "../menu/options.hpp"
#include "../features/prediction.hpp"

#include "../features/skinchanger.hpp"

#include <span>

#undef min
#undef max

void* find_hud_element( const char* name ) {
	static auto hud = pattern::search( _( "client.dll" ), _( "B9 ? ? ? ? 68 ? ? ? ? E8 ? ? ? ? 89 46 24" ) ).add( 1 ).deref( ).get< void* >( );
	static auto find_hud_element_func = pattern::search( _( "client.dll" ), _( "55 8B EC 53 8B 5D 08 56 57 8B F9 33 F6 39 77 28" ) ).get< void* ( __thiscall* )( void*, const char* ) >( );
	return ( void* )find_hud_element_func( hud, name );
}

void run_preserve_death_notices( ) {
	static auto& removals = options::vars [ _( "visuals.other.removals" ) ].val.l;
	static auto clear_death_notices = pattern::search ( _ ( "client.dll" ), _ ( "55 8B EC 83 EC 0C 53 56 8B 71 58" ) ).get< void ( __thiscall* )( void* ) > ( );

	const auto death_notice = find_hud_element ( _ ( "CCSGO_HudDeathNotice" ) );

	if ( !g::local || !g::local->alive ( ) ) {
		if ( clear_death_notices && death_notice )
			clear_death_notices ( reinterpret_cast< void* > ( uintptr_t ( death_notice ) - 20 ) );
	}

	if ( death_notice ) {
		if ( g::round == round_t::in_progress ) {
			auto* local_death_notice = reinterpret_cast< float* > ( uintptr_t( death_notice ) + 80 );

			if ( local_death_notice )
				*local_death_notice = removals [ 7 ] ? std::numeric_limits<float>::max ( ) : 1.5f;
		}

		if ( g::round == round_t::starting && uintptr_t( death_notice ) - 20 > 0 )
			if ( clear_death_notices )
				clear_death_notices( reinterpret_cast< void* > ( uintptr_t( death_notice ) - 20 ) );
	}
}

void run_radar_esp ( ) {
	const auto radar_hud = find_hud_element ( _ ( "CCSGO_HudRadar" ) );

	if ( !radar_hud )
		return;

	struct radar_player_t {
		vec3_t pos; //0x0000
		vec3_t angle; //0x000C
		vec3_t spotted_map_angle_related; //0x0018
		uint32_t tab_related; //0x0024
		PAD ( 12 );
		float spotted_time; //0x0034
		float spotted_fraction; //0x0038
		float time; //0x003C
		PAD ( 4 );
		int player_index; //0x0044
		int entity_index; //0x0048
		char pad_0x004C [ 0x4 ]; //0x004C
		int health; //0x0050
		char name [ 32 ]; //0x785888
		char pad_0x0074 [ 0x75 ]; //0x0074
		bool spotted; //0x00E9
		char pad_0x00EA [ 0x8A ]; //0x00EA
	};

	auto radar_players = ( radar_player_t* ) ( reinterpret_cast<uintptr_t>( radar_hud ) - 20 );
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
	//dbg_print ( fmt::format("stage: {}", stage).c_str());

	static auto m_vecBulletVerifyListClient = pattern::search ( _ ( "client.dll" ), _ ( "F3 0F 10 86 ? ? ? ? F3 0F 58 44 24 0C" ) ).add ( 4 ).deref ( ).add ( 0x274 ).get< uint32_t > ( );
	static auto& bullet_impacts_client = options::vars [ _ ( "visuals.other.bullet_impacts_client" ) ].val.b;
	static auto& bullet_impacts_client_color = options::vars [ _ ( "visuals.other.bullet_impacts_client_color" ) ].val.c;

	static auto& removals = options::vars [ _( "visuals.other.removals" ) ].val.l;
	static auto& world_color = options::vars [ _( "visuals.other.world_color" ) ].val.c;
	static auto& prop_color = options::vars [ _( "visuals.other.prop_color" ) ].val.c;
	static auto& ragdoll_force = options::vars [ _( "misc.effects.ragdoll_force_scale" ) ].val.f;

	g::local = ( !cs::i::engine->is_connected( ) || !cs::i::engine->is_in_game( ) ) ? nullptr : cs::i::ent_list->get< player_t* >( cs::i::engine->get_local_player( ) );

	/* fix rate problems */ {
		const auto rate = static_cast< int >( 1.0f / cs::i::globals->m_ipt + 0.5f );

		g::cvars::cl_updaterate->no_callback ( );
		g::cvars::cl_cmdrate->no_callback ( );

		g::cvars::cl_updaterate->set_value ( rate );
		g::cvars::cl_cmdrate->set_value ( rate );
	}

	set_aspect_ratio ( );

	security_handler::update( );

	vec3_t old_aimpunch;
	vec3_t old_viewpunch;
	
	/* reset resolver data when not in game */
	if ( !cs::i::engine->is_in_game ( ) || !cs::i::engine->is_connected() ) {
		anims::resolver::rdata::new_resolve.fill ( false );
		anims::resolver::rdata::resolved_side.fill ( anims::desync_side_t::desync_middle );
	}

	if ( g::local && cs::i::client_state ) {
		static auto last_cmd_time = cs::i::client_state->next_cmd_time ( );
		static auto last_ack_cmd = cs::i::client_state->last_command_ack ( );

		if ( last_cmd_time != cs::i::client_state->next_cmd_time ( )
			|| last_ack_cmd != cs::i::client_state->last_command_ack ( ) ) {
			if ( features::prediction::vel_modifier != 1.0f ) {
				features::prediction::vel_modifier = g::local->velocity_modifier ( );

				*reinterpret_cast< bool* > ( reinterpret_cast< uintptr_t >( cs::i::pred ) + 0x24 ) = true;
				*reinterpret_cast< int* > ( reinterpret_cast< uintptr_t >( cs::i::pred ) + 0x1C ) = 0;
			}

			last_cmd_time = cs::i::client_state->next_cmd_time ( );
			last_ack_cmd = cs::i::client_state->last_command_ack ( );
		}
	}

	if ( cs::i::engine->is_in_game( ) && cs::i::engine->is_connected( ) ) {
		if ( stage == 5 && g::local ) {
			/* bullet impacts */
			if ( g::local && bullet_impacts_client ) {
				static int last_num_impacts = 0;

				struct client_hit_verify_t {
					vec3_t pos;
					float time;
					float duration;
				};

				const auto num_impacts = *reinterpret_cast< int* >( reinterpret_cast< uintptr_t >( g::local ) + m_vecBulletVerifyListClient );
				const auto impacts = *reinterpret_cast< client_hit_verify_t** >( reinterpret_cast< uintptr_t >( g::local ) + m_vecBulletVerifyListClient + 4 );

				if ( num_impacts > last_num_impacts ) {
					for ( auto i = 0; i <= num_impacts - last_num_impacts - 1; i++ ) {
						auto& impact = impacts [ num_impacts - ( num_impacts - last_num_impacts ) + i ];
						cs::add_box_overlay ( impact.pos, vec3_t ( -2.0f, -2.0f, -2.0f ), vec3_t ( 2.0f, 2.0f, 2.0f ), cs::calc_angle ( g::local->eyes ( ), impact.pos ), bullet_impacts_client_color.r * 255.0f, bullet_impacts_client_color.g * 255.0f, bullet_impacts_client_color.b * 255.0f, bullet_impacts_client_color.a * 255.0f, 7.0f );
						anims::resolver::rdata::clientside_shot = true;
					}
				}

				last_num_impacts = num_impacts;
			}

			//static auto sv_showimpacts = cs::i::cvar->find (_("sv_showimpacts") );
			//sv_showimpacts->no_callback ( );
			//sv_showimpacts->set_value ( 1 );

			RUN_SAFE(
				"animations::resolver::create_beams",
				anims::resolver::create_beams( );
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
				const auto mat = cs::i::mat_sys->findmat( mat_s, _( "Other textures" ) );

				if ( mat )
					mat->set_material_var_flag( 4, removals [ 0 ] );
			}

			static auto smoke_count = pattern::search( _( "client.dll" ), _( "55 8B EC 83 EC 08 8B 15 ? ? ? ? 0F 57 C0" ) ).add( 8 ).deref( ).get< int* >( );

			if ( removals [ 0 ] )
				*smoke_count = 0;

			static options::option::colorf last_prop_clr = prop_color;
			static options::option::colorf last_clr = world_color;
			static bool last_alive = false;

			if ( g::local && ( g::local->alive( ) != last_alive || ( last_clr.r != world_color.r || last_clr.g != world_color.g || last_clr.b != world_color.b || last_clr.a != world_color.a ) || ( last_prop_clr.r != prop_color.r || last_prop_clr.g != prop_color.g || last_prop_clr.b != prop_color.b || last_prop_clr.a != prop_color.a ) ) ) {
				static auto load_named_sky = pattern::search( _( "engine.dll" ), _( "55 8B EC 81 EC ? ? ? ? 56 57 8B F9 C7 45" ) ).get< void( __fastcall* )( const char* ) >( );

				load_named_sky( _( "sky_csgo_night02" ) );

				for ( auto i = cs::i::mat_sys->first_material( ); i != cs::i::mat_sys->invalid_material( ); i = cs::i::mat_sys->next_material( i ) ) {
					auto mat = cs::i::mat_sys->get_material( i );

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

			for ( int i = 1; i <= cs::i::ent_list->get_highest_index( ); i++ ) {
				const auto pl = cs::i::ent_list->get < player_t* >( i );

				if ( !pl || pl->dormant( ) || !pl->client_class( ) || pl->client_class( )->m_class_id != 42 )
					continue;

				pl->force( ) *= ragdoll_force;
				pl->ragdoll_vel( ) *= ragdoll_force;
			}
		}
	}

	if ( stage == 2 ) {
		RUN_SAFE (
			"features::skinchanger::run",
			features::skinchanger::run ( );
		);

		if ( g::local )
			features::prediction::fix_netvars ( g::local->tick_base ( ) );

		features::prediction::fix_viewmodel ( );
	}

	anims::pre_fsn ( stage );

	/* draw server hitboxes for testing */
	//if ( g::local && g::local->alive() ){
	//	static const auto UTIL_PlayerByIndex = pattern::search(_( "server.dll") ,_( "85 C9 7E 2A A1") ).get<player_t* ( __fastcall* )( int )>( );
	//	static const auto draw_server_hitboxes = pattern::search( _( "server.dll" ) , _( "55 8B EC 81 EC ? ? ? ? 53 56 8B 35 ? ? ? ? 8B D9 57 8B CE" ) ).get<void*>( );
	//
	//	const auto duration = -1.0f;
	//	const auto player = UTIL_PlayerByIndex( g::local->idx( ) );
	//
	//	if ( player ) {
	//		__asm {
	//			pushad
	//			movss xmm1, duration
	//			push 1
	//			mov ecx, player
	//			call draw_server_hitboxes
	//			popad
	//		}
	//	}
	//}

	old::frame_stage_notify( REG_OUT, stage );

	anims::fsn ( stage );

	//features::prediction::update ( stage );

	if ( stage == 5 && g::local && cs::i::engine->is_in_game( ) && cs::i::engine->is_connected( ) ) {
		if ( g::local->alive( ) ) {
			if ( removals[ 3 ] )
				g::local->aim_punch( ) = old_aimpunch;

			if ( removals[ 4 ] )
				g::local->view_punch( ) = old_viewpunch;
		}
	}
}