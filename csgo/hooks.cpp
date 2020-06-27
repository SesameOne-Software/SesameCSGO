#include <intrin.h>
#include <oxui.hpp>
#include "hooks.hpp"
#include "globals.hpp"
#include "minhook/minhook.h"
#include "renderer/d3d9.hpp"
#include "menu/menu.hpp"

/* features */
#include "animations/animations.hpp"
#include "animations/resolver.hpp"
#include "features/chams.hpp"
#include "features/esp.hpp"
#include "features/movement.hpp"
#include "features/ragebot.hpp"
#include "features/antiaim.hpp"
#include "features/prediction.hpp"
#include "features/clantag.hpp"
#include "features/glow.hpp"
#include "features/nade_prediction.hpp"
#include "features/autowall.hpp"
#include "features/other_visuals.hpp"

/* security */
#include "security/security_handler.hpp"

bool hooks::in_autowall = false;
int hooks::scroll_delta = 0;
bool hooks::in_setupbones = false;
WNDPROC hooks::o_wndproc = nullptr;
bool hooks::no_update = false;

decltype( &hooks::in_prediction_hk ) hooks::in_prediction = nullptr;
decltype( &hooks::get_viewmodel_fov_hk ) hooks::get_viewmodel_fov = nullptr;
decltype( &hooks::list_leaves_in_box_hk ) hooks::list_leaves_in_box = nullptr;
decltype( &hooks::write_usercmd_hk ) hooks::write_usercmd = nullptr;
decltype( &hooks::fire_event_hk ) hooks::fire_event = nullptr;
decltype( &hooks::override_view_hk ) hooks::override_view = nullptr;
decltype( &hooks::get_bool_hk ) hooks::get_bool = nullptr;
decltype( &hooks::createmove_hk ) hooks::createmove = nullptr;
decltype( &hooks::framestagenotify_hk ) hooks::framestagenotify = nullptr;
decltype( &hooks::endscene_hk ) hooks::endscene = nullptr;
decltype( &hooks::reset_hk ) hooks::reset = nullptr;
decltype( &hooks::lockcursor_hk ) hooks::lockcursor = nullptr;
decltype( &hooks::sceneend_hk ) hooks::sceneend = nullptr;
decltype( &hooks::drawmodelexecute_hk ) hooks::drawmodelexecute = nullptr;
decltype( &hooks::doextraboneprocessing_hk ) hooks::doextraboneprocessing = nullptr;
decltype( &hooks::setupbones_hk ) hooks::setupbones = nullptr;
decltype( &hooks::get_eye_angles_hk ) hooks::get_eye_angles = nullptr;
decltype( &hooks::setupvelocity_hk ) hooks::setupvelocity = nullptr;
decltype( &hooks::draw_bullet_impacts_hk ) hooks::drawbulletimpacts = nullptr;
decltype( &hooks::trace_ray_hk ) hooks::traceray = nullptr;
decltype( &hooks::cl_sendmove_hk ) hooks::cl_sendmove = nullptr;
decltype( &hooks::update_clientside_animations_hk ) hooks::update_clientside_animations = nullptr;
decltype( &hooks::send_datagram_hk ) hooks::send_datagram = nullptr;
decltype( &hooks::should_skip_animation_frame_hk ) hooks::should_skip_animation_frame = nullptr;
decltype( &hooks::is_hltv_hk ) hooks::is_hltv = nullptr;
decltype( &hooks::paint_traverse_hk ) hooks::paint_traverse = nullptr;
decltype( &hooks::send_net_msg_hk ) hooks::send_net_msg = nullptr;
decltype( &hooks::emit_sound_hk ) hooks::emit_sound = nullptr;
decltype( &hooks::cs_blood_spray_callback_hk ) hooks::cs_blood_spray_callback = nullptr;
decltype( &hooks::modify_eye_pos_hk ) hooks::modify_eye_pos = nullptr;

/* event listener */
class c_event_handler : c_event_listener {
public:
	c_event_handler( ) {
		csgo::i::events->add_listener( this, _( "weapon_fire" ), false );
		csgo::i::events->add_listener( this, _( "player_say" ), false );
		csgo::i::events->add_listener( this, _( "player_hurt" ), false );
		csgo::i::events->add_listener ( this, _ ( "bullet_impact" ), true );
		csgo::i::events->add_listener ( this, _ ( "round_freeze_end" ), false );
		csgo::i::events->add_listener ( this, _ ( "round_start" ), false );
		csgo::i::events->add_listener ( this, _ ( "round_end" ), false );
	}

	virtual ~c_event_handler( ) {
		csgo::i::events->remove_listener( this );
	}

	virtual void fire_game_event( event_t* event ) {
		if ( !event || !g::local )
			return;

		//if ( !strcmp( event->get_name( ), _( "weapon_fire" ) ) )
		//	features::lagcomp::cache_shot( event );

		if ( !strcmp( event->get_name( ), _( "player_say" ) ) )
			/* translator::translate( ); */;

		if ( !strcmp( event->get_name( ), _( "player_hurt" ) ) )
			animations::resolver::process_hurt( event );

		if ( !strcmp( event->get_name( ), _( "bullet_impact" ) ) )
			animations::resolver::process_impact( event );

		if ( !strcmp ( event->get_name ( ), _ ( "round_freeze_end" ) ) )
			g::round = round_t::in_progress;

		if ( !strcmp ( event->get_name ( ), _ ( "round_start" ) ) )
			g::round = round_t::starting;

		if ( !strcmp ( event->get_name ( ), _ ( "round_end" ) ) )
			g::round = round_t::ending;
	}

	int get_event_debug_id( ) override {
		return 42;
	}
};

std::unique_ptr< c_event_handler > event_handler;

float __fastcall hooks::get_viewmodel_fov_hk ( REG ) {
	OPTION ( double, viewmodel_fov, "Sesame->C->Other->Removals->Viewmodel FOV", oxui::object_slider );
	return static_cast < float > ( viewmodel_fov );
}

/* fix event delays */
bool __fastcall hooks::fire_event_hk( REG ) {
	auto local = csgo::i::ent_list->get< player_t* >( csgo::i::engine->get_local_player( ) );

	if ( !csgo::i::client_state )
		return fire_event( REG_OUT );

	struct event_t {
	public:
		PAD( 4 );
		float delay;
		PAD( 48 );
		event_t* next;
	};

	for ( auto event = *( event_t** ) ( uintptr_t ( csgo::i::client_state ) + 0x4E64 ); event; event = event->next )
		event->delay = 0.0f;

	return fire_event( REG_OUT );
}

void __stdcall hooks::update_clientside_animations_hk( ) {
	if ( !g::local || !g::local->alive ( ) )
		update_clientside_animations ( );
}

void __stdcall hooks::cl_sendmove_hk( ) {
	cl_sendmove( );
}

void __fastcall hooks::override_view_hk( REG, void* setup ) {
	OPTION( bool, thirdperson, "Sesame->E->Effects->Main->Third Person", oxui::object_checkbox );
	OPTION ( double, thirdperson_range, "Sesame->E->Effects->Main->Third Person Range", oxui::object_slider );
	KEYBIND ( thirdperson_key, "Sesame->E->Effects->Main->Third Person Key" );
	KEYBIND ( fd_key, "Sesame->B->Other->Other->Fakeduck Key" );
	OPTION ( bool, no_zoom, "Sesame->C->Other->Removals->No Zoom", oxui::object_checkbox );
	OPTION ( double, custom_fov, "Sesame->C->Other->Removals->Custom FOV", oxui::object_slider );

	if ( g::local && ( no_zoom ? true : !g::local->scoped( ) ) )
		*reinterpret_cast< float* > ( uintptr_t ( setup ) + 176 ) = static_cast < float > ( custom_fov );

	auto get_ideal_dist = [ & ] ( float ideal_distance ) {
		vec3_t inverse;
		csgo::i::engine->get_viewangles( inverse );

		inverse.x *= -1.0f, inverse.y += 180.0f;

		vec3_t direction = csgo::angle_vec( inverse );

		ray_t ray;
		trace_t trace;
		trace_filter_t filter( g::local );

		csgo::util_traceline( g::local->eyes( ), g::local->eyes( ) + ( direction * ideal_distance ), 0x600400B, g::local, &trace );

		return ( ideal_distance * trace.m_fraction ) - 10.0f;
	};

	if ( thirdperson && thirdperson_key && g::local ) {
		if ( g::local->alive( ) ) {
			vec3_t ang;
			csgo::i::engine->get_viewangles ( ang );
			csgo::i::input->m_camera_in_thirdperson = true;
			csgo::i::input->m_camera_offset = vec3_t ( ang.x, ang.y, get_ideal_dist ( thirdperson_range ) );
		}
		else {
			csgo::i::input->m_camera_in_thirdperson = false;
			g::local->observer_mode ( ) = 5;
		}
	}
	else {
		csgo::i::input->m_camera_in_thirdperson = false;
	}

	if ( fd_key && g::local && g::local->alive( ) )
		*reinterpret_cast< float* >( uintptr_t ( setup ) + 0xc0 ) = g::local->abs_origin ( ).z + 64.0f;

	override_view( REG_OUT, setup );
}

extern std::deque< animations::resolver::hit_matrix_rec_t > hit_matrix_rec;
extern animations::resolver::hit_matrix_rec_t cur_hit_matrix_rec;

void __fastcall hooks::sceneend_hk( REG ) {
	sceneend( REG_OUT );

	if ( !g::local || !g::local->alive ( ) )
		hit_matrix_rec.clear ( );

	if ( g::local && g::local->alive ( ) && g::local->simtime( ) != g::local->old_simtime( ) )
		features::chams::old_origin = g::local->origin ( );

	if ( g::local ) {
		for ( auto i = 1; i <= 200; i++ ) {
			const auto ent = csgo::i::ent_list->get < entity_t* > ( i );

			if ( !ent || !ent->client_class ( ) || ( ent->client_class ( )->m_class_id != 40 && ent->client_class ( )->m_class_id != 42 ) )
				continue;

			static auto off_player_handle = netvars::get_offset ( _ ( "DT_CSRagdoll->m_hPlayer" ) );

			auto pl_idx = -1;

			if ( ent->client_class ( )->m_class_id == 42 )
				pl_idx = *reinterpret_cast< uint32_t* > ( uintptr_t ( ent ) + off_player_handle ) & 0xfff;
			else
				pl_idx = ent->idx ( );

			if ( pl_idx != -1 && pl_idx ) {
				std::vector< animations::resolver::hit_matrix_rec_t > hit_matricies { };

				if ( !hit_matrix_rec.empty ( ) )
					for ( auto& hit : hit_matrix_rec )
						if ( hit.m_pl == pl_idx )
							hit_matricies.push_back ( hit );

				if ( !hit_matricies.empty ( ) ) {
					for ( auto& hit : hit_matricies ) {
						cur_hit_matrix_rec = hit;

						features::chams::in_model = true;
						ent->draw ( );
						features::chams::in_model = false;
					}
				}
			}
		}
	}

	features::glow::cache_entities( );
}

void __fastcall hooks::drawmodelexecute_hk( REG, void* ctx, void* state, const mdlrender_info_t& info, matrix3x4_t* bone_to_world ) {
	features::chams::drawmodelexecute( ctx, state, info, bone_to_world );
}

auto disable_sounds = false;

bool __fastcall hooks::in_prediction_hk ( REG ) {
	static const auto return_to_maintain_sequence_transitions = pattern::search ( "client.dll", "84 C0 74 17 8B 87" ).get < void* >( );
	static const auto return_to_play_step_sound = pattern::search ( "client.dll", "84 C0 ? ? A1 ? ? ? ? B9 ? ? ? ? 8B 40 3C FF D0 84 C0 ? ? ? ? ? ? 8B 45 0C 85 C0 ? ? ? ? ? ? 8B 93 F8 2F" ).get < void* > ( );

	g::local = csgo::i::ent_list->get< player_t* > ( csgo::i::engine->get_local_player ( ) );

	if ( _ReturnAddress ( ) == return_to_maintain_sequence_transitions && g::local->valid ( ) )
		return false;

	if ( _ReturnAddress ( ) == return_to_play_step_sound && disable_sounds )
		return true;

	return in_prediction ( REG_OUT );
}

auto restore_ticks = 0;
auto cock_ticks = 0;
auto last_attack = false;
auto last_tickbase_shot = false;

void fix_event_delay ( ucmd_t* ucmd ) {
	OPTION ( int, _fd_mode, "Sesame->B->Other->Other->Fakeduck Mode", oxui::object_dropdown );
	KEYBIND ( _fd_key, "Sesame->B->Other->Other->Fakeduck Key" );

	/* choke packets if requested */
	if ( ucmd->m_buttons & 1 && g::local && !_fd_key )
		g::send_packet = true;

	/* reset pitch as fast as possible after shot so our on-shot doesn't get completely raped */
	if ( !features::ragebot::active_config.choke_on_shot && last_attack && !( ucmd->m_buttons & 1 ) && !( _fd_key && _fd_mode ) )
		g::send_packet = true;
	
	last_attack = ucmd->m_buttons & 1;
}

bool in_cm = false;
bool ducking = false;

bool __fastcall hooks::createmove_hk( REG, float sampletime, ucmd_t* ucmd ) {
	ducking = ucmd->m_buttons & 4;
	in_cm = true;

	features::ragebot::get_weapon_config(features::ragebot::active_config);

	if ( !g::local || !g::local->alive ( ) ) {
		g::shifted_tickbase = ucmd->m_cmdnum;
		cock_ticks = 0;
	}

	if ( !ucmd || !ucmd->m_cmdnum ) {
		in_cm = false;
		return false;
	}

	security_handler::update ( );

	/* thanks chambers */
	if ( csgo::i::client_state->choked ( ) ) {
		disable_sounds = true;

		csgo::i::pred->update (
			csgo::i::client_state->delta_tick ( ),
			csgo::i::client_state->delta_tick ( ) > 0,
			csgo::i::client_state->last_command_ack ( ),
			csgo::i::client_state->last_outgoing_cmd ( ) + csgo::i::client_state->choked ( ) );

		disable_sounds = false;
	}

	const auto refresh_tickbase = g::shifted_tickbase - 1 > ucmd->m_cmdnum && g::local && g::local->alive ( );

	if ( refresh_tickbase && !csgo::is_valve_server( ) ) {
		*( bool* ) ( *( uintptr_t* ) ( uintptr_t ( _AddressOfReturnAddress ( ) ) - 4 ) - 28 ) = true;
		ucmd->m_tickcount = INT_MAX;
		in_cm = false;
		return false;
	}

	if ( g::local && g::local->weapon ( ) ) {
		const auto weapon = g::local->weapon ( );
		weapon->update_accuracy ( );
		features::spread_circle::total_spread = weapon->inaccuracy ( ) + weapon->spread ( );
	}
	else {
		features::spread_circle::total_spread = 0.0f;
	}

	features::esp::handle_dynamic_updates ( );

	auto ret = createmove( REG_OUT, sampletime, ucmd );

	g::ucmd = ucmd;
	
	features::prediction::update_curtime( );

	auto old_angs = ucmd->m_angs;

	features::clantag::run( ucmd );
	features::movement::run( ucmd, old_angs );

	auto old_smove = ucmd->m_smove;
	auto old_fmove = ucmd->m_fmove;

	csgo::for_each_player ( [ ] ( player_t* pl ) {
		static auto reloading_offset = pattern::search ( _ ( "client.dll" ), _ ( "C6 87 ? ? ? ? ? 8B 06 8B CE FF 90" ) ).add ( 2 ).deref ( ).get < uint32_t > ( );

		features::esp::esp_data [ pl->idx ( ) ].m_fakeducking = pl->crouch_speed ( ) == 8.0f && pl->crouch_amount ( ) > 0.1f && pl->crouch_amount ( ) < 0.9f;
 		features::esp::esp_data [ pl->idx ( ) ].m_reloading = pl->weapon ( ) ? *reinterpret_cast< bool* >( uintptr_t ( pl->weapon ( ) ) + reloading_offset ) : false;

		if ( g::local && g::local->weapon ( ) && g::local->weapon ( )->data ( ) ) {
			auto dmg_out = static_cast< float >( g::local->weapon ( )->data ( )->m_dmg );
			autowall::scale_dmg ( pl, g::local->weapon ( )->data ( ), 3, dmg_out );
			features::esp::esp_data [ pl->idx ( ) ].m_fatal = static_cast< int > ( dmg_out ) >= pl->health ( );
		}
		else {
			features::esp::esp_data [ pl->idx ( ) ].m_fatal = false;
		}

		features::lagcomp::pop ( pl );
	} );

	features::nade_prediction::trace ( ucmd );

	features::prediction::run( [ & ] ( ) {
		csgo::for_each_player ( [ ] ( player_t* pl ) {
			features::lagcomp::pop ( pl );
		} );

		features::antiaim::simulate_lby( );
		ducking = ucmd->m_buttons & 4;
		features::ragebot::run( ucmd, old_smove, old_fmove, old_angs );
		features::antiaim::run( ucmd, old_smove, old_fmove );
		animations::resolver::update( ucmd );
	} );

	fix_event_delay ( ucmd );

	if ( ucmd->m_buttons & 1 )
		g::next_tickbase_shot = false;

	if ( !g::next_tickbase_shot && last_tickbase_shot ) {
		g::shifted_tickbase = ucmd->m_cmdnum + std::clamp ( static_cast < int > ( features::ragebot::active_config.max_dt_ticks ), 0, csgo::is_valve_server ( ) ? 8 : 16 );
		g::next_tickbase_shot = false;
		last_tickbase_shot = false;
	}

	last_tickbase_shot = g::next_tickbase_shot;

	if ( !refresh_tickbase && g::local && g::local->weapon ( ) && g::local->weapon ( )->data( ) && features::ragebot::active_config.auto_revolver && g::local->weapon ( )->item_definition_index ( ) == 64 && !( ucmd->m_buttons & 1 ) ) {
		if ( csgo::time2ticks( features::prediction::predicted_curtime ) > cock_ticks ) {
			ucmd->m_buttons &= ~1;
			cock_ticks = csgo::time2ticks ( features::prediction::predicted_curtime + 0.25f ) - 1;
		}
		else {
			ucmd->m_buttons |= 1;
		}
	}

	csgo::clamp( ucmd->m_angs );

	csgo::rotate_movement( ucmd, old_smove, old_fmove, old_angs );

	*( bool* ) ( *( uintptr_t* ) ( uintptr_t( _AddressOfReturnAddress( ) ) - 4 ) - 28 ) = refresh_tickbase ? true : g::send_packet;

	/* fix anti-aim slide */ {
		if ( ucmd->m_fmove ) {
			ucmd->m_buttons &= ~( ucmd->m_fmove < 0.0f ? 8 : 16 );
			ucmd->m_buttons |= ( ucmd->m_fmove > 0.0f ? 8 : 16 );
		}

		if ( ucmd->m_smove ) {
			ucmd->m_buttons &= ~( ucmd->m_smove < 0.0f ? 1024 : 512 );
			ucmd->m_buttons |= ( ucmd->m_smove > 0.0f ? 1024 : 512 );
		}
	}

	features::ragebot::tickbase_controller( ucmd );

	if ( g::send_packet ) {
		g::sent_cmd = *ucmd;
		//animations::fake::simulate( );
	}

	ucmd->m_fmove = std::clamp< float > ( ucmd->m_fmove, -450.0f, 450.0f );
	ucmd->m_smove = std::clamp< float > ( ucmd->m_smove, -450.0f, 450.0f );

	in_cm = false;
	return false;
}

void* find_hud_element ( const char* name ) {
	static auto hud = pattern::search ( _("client.dll"), _("B9 ? ? ? ? E8 ? ? ? ? 8B 5D 08") ).add ( 1 ).deref ( ).get< void* > ( );
	static auto find_hud_element_func = pattern::search ( _("client.dll"), _("55 8B EC 53 8B 5D 08 56 57 8B F9 33 F6 39 77 28") ).get< void* ( __thiscall* )( void*, const char* ) > ( );
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
	*reinterpret_cast< uintptr_t* > ( uintptr_t ( r_drawstaticprops ) + 0x2c ) = *reinterpret_cast < uintptr_t* > ( uintptr_t( &fval ) ) ^ uintptr_t ( r_drawstaticprops );
	*reinterpret_cast< uintptr_t* > ( uintptr_t ( r_drawstaticprops ) + 0x2c + 0x4 ) = static_cast < int > ( fval ) ^ uintptr_t ( r_drawstaticprops );

	OPTION ( double, aspect_ratio, "Sesame->C->Other->Removals->Custom Aspect Ratio", oxui::object_slider );
	static auto r_aspect_ratio = pattern::search ( _ ( "engine.dll" ), _ ( "81 F9 ? ? ? ? 75 16 F3 0F 10 0D ? ? ? ? F3 0F 11 4D ? 81 75 ? ? ? ? ? EB 18 8B 01 8B 40 30 FF D0 F3 0F 10 0D ? ? ? ? 8B 0D ? ? ? ? D9 5D FC F3 0F 10 45 ? 0F 2F 05 ? ? ? ? 76 34" ) ).add ( 2 ).deref ( ).get< void* > ( );
	auto as_float = static_cast< float > ( aspect_ratio ) * 1.7777777f;
	*reinterpret_cast< uintptr_t* > ( uintptr_t ( r_aspect_ratio ) + 0x2c ) = *reinterpret_cast< uintptr_t* > ( &as_float ) ^ uintptr_t ( r_aspect_ratio );
}

void __fastcall hooks::framestagenotify_hk( REG, int stage ) {
	OPTION ( bool, no_smoke, "Sesame->C->Other->Removals->No Smoke", oxui::object_checkbox );
	OPTION ( bool, no_flash, "Sesame->C->Other->Removals->No Flash", oxui::object_checkbox );
	OPTION ( bool, no_aimpunch, "Sesame->C->Other->Removals->No Aimpunch", oxui::object_checkbox );
	OPTION ( bool, no_viewpunch, "Sesame->C->Other->Removals->No Viewpunch", oxui::object_checkbox );
	OPTION ( oxui::color, world_clr, "Sesame->C->Other->World->World Color", oxui::object_colorpicker );
	OPTION ( oxui::color, prop_clr, "Sesame->C->Other->World->Prop Color", oxui::object_colorpicker );
	OPTION ( double, ragdoll_force, "Sesame->E->Effects->Main->Ragdoll Force", oxui::object_slider );

	g::local = ( !csgo::i::engine->is_connected( ) || !csgo::i::engine->is_in_game( ) ) ? nullptr : csgo::i::ent_list->get< player_t* >( csgo::i::engine->get_local_player( ) );

	/* reset tickbase shift data if not in game. */
	if ( !csgo::i::engine->is_connected( ) || !csgo::i::engine->is_in_game( ) ) {
		g::dt_ticks_to_shift = 0;
		g::dt_recharge_time = 0;
	}

	security_handler::update ( );

	vec3_t old_aimpunch;
	vec3_t old_viewpunch;
	float old_flashalpha;

	animations::run( stage );

	set_aspect_ratio ( );

	if ( stage == 5 && g::local ) {
		run_preserve_death_notices ( );

		if ( g::local->alive ( ) ) {
			old_aimpunch = g::local->aim_punch ( );
			old_viewpunch = g::local->view_punch ( );

			if ( no_aimpunch )
				g::local->aim_punch ( ) = vec3_t( 0.0f, 0.0f, 0.0f );

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

		static auto smoke_count = pattern::search ( _ ( "client.dll" ), _ ( "55 8B EC 83 EC 08 8B 15 ? ? ? ? 0F 57 C0" ) ).add ( 8 ).deref( ).get< int* > ( );
		
		if ( no_smoke )
			*smoke_count = 0;

		static oxui::color last_prop_clr = prop_clr;
		static oxui::color last_clr = world_clr;
		static float spawn_time = 0.0f;

		if ( g::local && ( g::local->spawn_time ( ) != spawn_time || ( last_clr.r != world_clr.r || last_clr.g != world_clr.g || last_clr.b != world_clr.b || last_clr.a != world_clr.a ) || ( last_prop_clr.r != prop_clr.r || last_prop_clr.g != prop_clr.g || last_prop_clr.b != prop_clr.b || last_prop_clr.a != prop_clr.a ) ) ) {
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
			spawn_time = g::local->spawn_time ( );
		}

		for ( int i = 1; i <= 200; i++ ) {
			const auto pl = csgo::i::ent_list->get < player_t* > ( i );

			if ( !pl || pl->dormant ( ) || !pl->client_class ( ) || pl->client_class ( )->m_class_id != 42 )
				continue;

			pl->force ( ) *= ragdoll_force;
			pl->ragdoll_vel ( ) *= ragdoll_force;
		}
	}

	if ( stage == 2 && g::local && no_flash )
		g::local->flash_alpha ( ) = 0.0f;

	framestagenotify( REG_OUT, stage );

	if ( stage == 5 && g::local ) {
		if ( g::local->alive ( ) ) {
			g::local->aim_punch ( ) = old_aimpunch;
			g::local->view_punch ( ) = old_viewpunch;
		}
	}
}

long __fastcall hooks::endscene_hk( REG, IDirect3DDevice9* device ) {
	OPTION ( bool, no_scope, "Sesame->C->Other->Removals->No Scope", oxui::object_checkbox );

	static auto ret = _ReturnAddress( );

	if ( ret != _ReturnAddress( ) )
		return endscene( REG_OUT, device );

	IDirect3DStateBlock9* pixel_state = nullptr;
	IDirect3DVertexDeclaration9* vertex_decleration = nullptr;
	IDirect3DVertexShader9* vertex_shader = nullptr;

	DWORD rs_anti_alias = 0;

	device->GetRenderState( D3DRS_MULTISAMPLEANTIALIAS, &rs_anti_alias );
	device->CreateStateBlock( D3DSBT_PIXELSTATE, &pixel_state );
	device->GetVertexDeclaration( &vertex_decleration );
	device->GetVertexShader( &vertex_shader );
	device->SetRenderState( D3DRS_MULTISAMPLEANTIALIAS, false );
	device->SetVertexShader( nullptr );
	device->SetPixelShader( nullptr );

	device->SetVertexShader ( nullptr );
	device->SetPixelShader ( nullptr );
	device->SetFVF ( D3DFVF_XYZRHW | D3DFVF_DIFFUSE );
	device->SetRenderState ( D3DRS_LIGHTING, false );
	device->SetRenderState ( D3DRS_FOGENABLE, false );
	device->SetRenderState ( D3DRS_CULLMODE, D3DCULL_NONE );
	device->SetRenderState ( D3DRS_FILLMODE, D3DFILL_SOLID );
	device->SetRenderState ( D3DRS_ZENABLE, D3DZB_FALSE );
	device->SetRenderState ( D3DRS_SCISSORTESTENABLE, true );
	device->SetRenderState ( D3DRS_ZWRITEENABLE, false );
	device->SetRenderState ( D3DRS_STENCILENABLE, false );
	device->SetRenderState ( D3DRS_MULTISAMPLEANTIALIAS, true );
	device->SetRenderState ( D3DRS_ANTIALIASEDLINEENABLE, true );
	device->SetRenderState ( D3DRS_ALPHABLENDENABLE, true );
	device->SetRenderState ( D3DRS_ALPHATESTENABLE, false );
	device->SetRenderState ( D3DRS_SEPARATEALPHABLENDENABLE, true );
	device->SetRenderState ( D3DRS_SRCBLEND, D3DBLEND_SRCALPHA );
	device->SetRenderState ( D3DRS_SRCBLENDALPHA, D3DBLEND_INVDESTALPHA );
	device->SetRenderState ( D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA );
	device->SetRenderState ( D3DRS_DESTBLENDALPHA, D3DBLEND_ONE );
	device->SetRenderState ( D3DRS_SRGBWRITEENABLE, false );
	device->SetRenderState ( D3DRS_COLORWRITEENABLE, D3DCOLORWRITEENABLE_RED | D3DCOLORWRITEENABLE_GREEN | D3DCOLORWRITEENABLE_BLUE | D3DCOLORWRITEENABLE_ALPHA );

	security_handler::update ( );
	
	features::nade_prediction::draw ( );
	features::esp::render( );
	animations::resolver::render_impacts ( );
	
	if ( no_scope && g::local && g::local->scoped( ) ) {
		int w, h;
		render::screen_size ( w, h );
		render::line ( w / 2, 0, w / 2, h, D3DCOLOR_RGBA ( 0, 0, 0, 255 ) );
		render::line ( 0, h / 2, w, h / 2, D3DCOLOR_RGBA( 0, 0, 0, 255 ) );
	}
	
	features::spread_circle::draw ( );
	
	menu::draw_watermark ( );
	menu::draw( );

	pixel_state->Apply( );
	pixel_state->Release( );

	device->SetVertexDeclaration( vertex_decleration );
	device->SetVertexShader( vertex_shader );
	device->SetRenderState( D3DRS_MULTISAMPLEANTIALIAS, rs_anti_alias );

	return endscene( REG_OUT, device );
}

long __fastcall hooks::reset_hk( REG, IDirect3DDevice9* device, D3DPRESENT_PARAMETERS* presentation_params ) {
	features::esp::esp_font->Release( );
	features::esp::indicator_font->Release( );
	features::esp::dbg_font->Release ( );
	features::esp::watermark_font->Release ( );

	menu::destroy( );

	auto hr = reset( REG_OUT, device, presentation_params );

	if ( SUCCEEDED( hr ) ) {
		render::create_font ( ( void** ) &features::esp::dbg_font, _ ( L"Segoe UI" ), N ( 12 ), false );
		render::create_font ( ( void** ) &features::esp::esp_font, _ ( L"Segoe UI" ), N ( 18 ), false );
		render::create_font ( ( void** ) &features::esp::indicator_font, _ ( L"Segoe UI" ), N ( 14 ), true );
		render::create_font ( ( void** ) &features::esp::watermark_font, _ ( L"Segoe UI" ), N ( 18 ), false );
		menu::reset( );
	}

	return hr;
}

void __fastcall hooks::lockcursor_hk( REG ) {
	if ( menu::open( ) ) {
		csgo::i::surface->unlock_cursor( );
		return;
	}

	lockcursor( REG_OUT );
}

void __fastcall hooks::doextraboneprocessing_hk( REG, int a2, int a3, int a4, int a5, int a6, int a7 ) {
	return;
}

bool __fastcall hooks::setupbones_hk( REG, matrix3x4_t* out, int max_bones, int mask, float curtime ) {
	static auto ccsplayer_setup_bones_ret_addr = pattern::search ( _ ( "client.dll" ), _ ( "E8 ? ? ? ? 5E 5D C2 10 00 32 C0" ) ).add ( 5 ).get< int* > ( );

	const auto pl = reinterpret_cast< player_t* >( uintptr_t ( ecx ) - 4 );

	if ( _ReturnAddress ( ) == ccsplayer_setup_bones_ret_addr && g::local && pl && pl->team( ) != g::local->team( ) ) {
		//if ( pl->bone_accessor ( ).get_bone_arr_for_write ( ) )
		//	std::memcpy ( pl->bone_accessor ( ).get_bone_arr_for_write ( ), &animations::data::fixed_bones [ pl->idx ( ) ], sizeof ( matrix3x4_t ) * pl->bone_count ( ) );

		return false;
	}

	//if ( g::local && pl && pl->client_class ( ) && pl->client_class ( )->m_class_id == 40 ) {
	//	const auto backup_effects = *reinterpret_cast< int* >( uintptr_t ( pl ) + N ( 0xf0 ) );
	//
	//	const auto backup_curtime = csgo::i::globals->m_curtime;
	//	const auto backup_realtime = csgo::i::globals->m_realtime;
	//	const auto backup_frametime = csgo::i::globals->m_frametime;
	//	const auto backup_abs_frametime = csgo::i::globals->m_abs_frametime;
	//	const auto backup_abs_framestarttimestddev = csgo::i::globals->m_abs_framestarttimestddev;
	//	const auto backup_interp = csgo::i::globals->m_interp;
	//	const auto backup_framecount = csgo::i::globals->m_framecount;
	//	const auto backup_tickcount = csgo::i::globals->m_tickcount;
	//
	//	csgo::i::globals->m_curtime = pl->simtime ( );
	//	csgo::i::globals->m_realtime = pl->simtime ( );
	//	csgo::i::globals->m_frametime = csgo::i::globals->m_ipt;
	//	csgo::i::globals->m_abs_frametime = csgo::i::globals->m_ipt;
	//	csgo::i::globals->m_abs_framestarttimestddev = pl->simtime ( ) - csgo::i::globals->m_ipt;
	//	csgo::i::globals->m_interp = 0.0f;
	//	csgo::i::globals->m_framecount = INT_MAX;
	//	csgo::i::globals->m_tickcount = csgo::time2ticks ( pl->simtime ( ) );
	//
	//	*reinterpret_cast< int* >( uintptr_t ( pl ) + N ( 0xf0 ) ) |= N ( 8 );
	//
	//	const auto ret = setupbones ( REG_OUT, out, max_bones, mask, curtime );
	//
	//	csgo::i::globals->m_curtime = backup_curtime;
	//	csgo::i::globals->m_realtime = backup_realtime;
	//	csgo::i::globals->m_frametime = backup_frametime;
	//	csgo::i::globals->m_abs_frametime = backup_abs_frametime;
	//	csgo::i::globals->m_abs_framestarttimestddev = backup_abs_framestarttimestddev;
	//	csgo::i::globals->m_interp = backup_interp;
	//	csgo::i::globals->m_framecount = backup_framecount;
	//	csgo::i::globals->m_tickcount = backup_tickcount;
	//
	//	*reinterpret_cast< int* >( uintptr_t ( pl ) + N ( 0xf0 ) ) = backup_effects;
	//
	//	if ( out )
	//		std::memcpy ( out, &animations::data::fixed_bones [ pl->idx ( ) ], sizeof ( matrix3x4_t ) * pl->bone_count ( ) );
	//	
	//	//if ( pl->bone_accessor ( ).get_bone_arr_for_write ( ) )
	//	//	std::memcpy ( pl->bone_accessor ( ).get_bone_arr_for_write ( ), &animations::data::fixed_bones [ pl->idx ( ) ], sizeof ( matrix3x4_t ) * pl->bone_count ( ) );
	//
	//	return ret;
	//}

	return setupbones( REG_OUT, out, max_bones, mask, curtime );
}

vec3_t* __fastcall hooks::get_eye_angles_hk( REG ) {
	auto local = csgo::i::ent_list->get< player_t* >( csgo::i::engine->get_local_player( ) );

	if ( ecx != local )
		return get_eye_angles( REG_OUT );

	static auto ret_to_thirdperson_pitch = pattern::search( _( "client.dll" ), _( "8B CE F3 0F 10 00 8B 06 F3 0F 11 45 ? FF 90 ? ? ? ? F3 0F 10 55" ) ).get< std::uintptr_t >( );
	static auto ret_to_thirdperson_yaw = pattern::search( _( "client.dll" ), _( "F3 0F 10 55 ? 51 8B 8E" ) ).get< std::uintptr_t >( );

	if ( ( std::uintptr_t( _ReturnAddress( ) ) == ret_to_thirdperson_pitch
		|| std::uintptr_t( _ReturnAddress( ) ) == ret_to_thirdperson_yaw )
		&& !no_update )
		return g::ucmd ? &g::angles : &local->angles( );

	return get_eye_angles( REG_OUT );
}

bool __fastcall hooks::get_bool_hk( REG ) {
	//static auto cl_interpolate = pattern::search( _( "client.dll" ), _( "85 C0 BF ? ? ? ? 0F 95 C3" ) ).get< void* >( );
	static auto cam_think = pattern::search( _( "client.dll" ), _( "85 C0 75 30 38 86" ) ).get< void* >( );
	//static auto hermite_fix = pattern::search( _( "client.dll" ), _( "0F B6 15 ? ? ? ? 85 C0" ) ).get< void* >( );
	//static auto cl_extrapolate = pattern::search ( _ ( "client.dll" ), _ ( "85 C0 74 22 8B 0D ? ? ? ? 8B 01 8B" ) ).get< void* > ( );
	//static auto color_static_props = pattern::search ( _ ( "engine.dll" ), _ ( "85 C0 0F 84 ? ? ? ? 8D 4B 32" ) ).get< void* > ( );
	//static auto color_static_props1 = pattern::search ( _ ( "engine.dll" ), _ ( "85 C0 75 74 8B 0D" ) ).get< void* > ( );

	if ( !ecx )
		return false;

	if ( _ReturnAddress ( ) == cam_think /*|| _ReturnAddress ( ) == hermite_fix || _ReturnAddress ( ) == hermite_fix || _ReturnAddress ( ) == cl_extrapolate*/ )
		return true;

	return get_bool( REG_OUT );
}

void __declspec( naked ) __stdcall hooks::naked_setupvelocity_hk( ) {
	__asm {
		pushad
		push ecx
		call /* anim_system::setup_velocity */ eax
		popad
		ret
	}
}

void __fastcall hooks::setupvelocity_hk( animstate_t* state, void* edx ) {
	__asm pushad
	// anim_system::setup_velocity( state );
	__asm popad
}

void __fastcall hooks::draw_bullet_impacts_hk( REG, int a1, int a2 ) {
	if ( in_autowall )
		return;

	drawbulletimpacts( REG_OUT, a1, a2 );
}

void __fastcall hooks::trace_ray_hk( REG, const ray_t& ray, unsigned int mask, trace_filter_t* filter, trace_t* trace ) {
	traceray( REG_OUT, ray, mask, filter, trace );

	if ( in_autowall )
		trace->m_surface.m_flags |= 4;
}

class c_nc {
public:
	PAD ( 23 );
	bool should_delete;
	int out_seq_nr;
	int in_seq_nr;
	int out_seq_nr_ack;
	int out_reliable_state;
	int in_reliable_state;
	int choked_packets;
};

int __fastcall hooks::send_datagram_hk( REG, void* datagram ) {
	const auto nc = *reinterpret_cast< c_nc** >( uintptr_t ( csgo::i::client_state ) + 0x9c );

	if ( nc && nc->choked_packets ) {
		static auto nc_send_datagram = pattern::search ( _ ( "engine.dll" ), _ ( "55 8B EC 83 E4 F0 B8 ? ? ? ? E8 ? ? ? ? 56 57 8B F9 89 7C 24 18" ) ).get< int ( __thiscall* )( void*, void* )> ( );
		const auto current_choke = nc->choked_packets;

		nc->choked_packets = 0;

		const auto ret = send_datagram ( REG_OUT, datagram );
		//nc_send_datagram ( nc, nullptr );

		nc->out_seq_nr--;
		nc->choked_packets = current_choke;

		return ret;
	}

	return send_datagram ( REG_OUT, datagram );
}

bool __fastcall hooks::should_skip_animation_frame_hk( REG ) {
	return false;
}

bool __fastcall hooks::is_hltv_hk( REG ) {
	static const auto accumulate_layers_call = pattern::search( _( "client.dll" ), _( "84 C0 75 0D F6 87" ) ).get< void* >( );
	static const auto setupvelocity_call = pattern::search( _( "client.dll" ), _( "84 C0 75 38 8B 0D ? ? ? ? 8B 01 8B 80 ? ? ? ? FF D0" ) ).get< void* >( );

	if ( !csgo::i::engine->is_in_game( ) || !csgo::i::engine->is_connected( ) )
		return is_hltv( REG_OUT );

	if ( _ReturnAddress( ) == accumulate_layers_call || _ReturnAddress( ) == setupvelocity_call )
		return true;

	return is_hltv( REG_OUT );
}

bool __fastcall hooks::write_usercmd_hk( REG, int slot, void* buf, int from, int to, bool new_cmd ) {
	static auto write_ucmd = pattern::search( _("client.dll"), _("55 8B EC 83 E4 F8 51 53 56 8B D9 8B 0D" )).get< void( __fastcall* )( void*, ucmd_t*, ucmd_t* ) >( );
	static auto cl_sendmove_ret = pattern::search( _("engine.dll"), _("84 C0 74 04 B0 01 EB 02 32 C0 8B FE 46 3B F3 7E C9 84 C0 0F 84") ).get< void* >( );

	if ( _ReturnAddress( ) != cl_sendmove_ret || g::dt_ticks_to_shift <= 0 ) {
		return write_usercmd( REG_OUT, slot, buf, from, to, new_cmd );
	}

	if ( from != -1 )
		return true;

	const auto new_commands = *reinterpret_cast< int* >( uintptr_t( buf ) - 0x2C );
	const auto num_cmd = csgo::i::client_state->choked ( ) + 1;
	const auto next_cmd_num = csgo::i::client_state->last_outgoing_cmd( ) + num_cmd;
	const auto total_new_cmds = std::clamp( g::dt_ticks_to_shift, 0, csgo::is_valve_server( ) ? 8 : 16 );

	from = -1;
	*reinterpret_cast< int* >( uintptr_t( buf ) - 0x2C ) = total_new_cmds;
	*reinterpret_cast< int* >( uintptr_t( buf ) - 0x30 ) = 0;

	for ( to = next_cmd_num - new_commands + 1; to <= next_cmd_num; to++ ) {
		if ( !write_usercmd( REG_OUT, slot, buf, from, to, true ) )
			return false;

		from = to;
	}
	
	const auto last_real_cmd = vfunc< ucmd_t * ( __thiscall* )( void*, int, int ) >( csgo::i::input, 8 )( csgo::i::input, slot, from );
	ucmd_t cmd_from;

	if ( last_real_cmd )
		cmd_from = *last_real_cmd;

	ucmd_t cmd_to = cmd_from;
	cmd_to.m_cmdnum++;

	if ( features::ragebot::active_config.dt_teleport && !csgo::is_valve_server ( ) /* don't wanna set tickcount to int_max or else we will get banned on valve servers */ )
		cmd_to.m_tickcount++;
	else
		cmd_to.m_tickcount += 666;

	for ( auto i = new_commands; i <= total_new_cmds; i++ ) {
		write_ucmd ( buf, &cmd_to, &cmd_from );
		cmd_from = cmd_to;
		cmd_to.m_cmdnum++;
		cmd_to.m_tickcount++;
	}

	const auto weapon_data = ( g::local && g::local->weapon ( ) && g::local->weapon ( )->data ( ) ) ? g::local->weapon ( )->data ( ) : nullptr;
	const auto fire_rate = weapon_data ? weapon_data->m_fire_rate : total_new_cmds;

	//if ( features::ragebot::active_config.dt_teleport )
	//	g::shifted_tickbase = cmd_to.m_cmdnum + 1;

	g::dt_ticks_to_shift = 0;
	g::next_tickbase_shot = true;

	return true;
}

int __fastcall hooks::list_leaves_in_box_hk ( REG, vec3_t& mins, vec3_t& maxs, uint16_t* list, int list_max ) {
	OPTION ( bool, skip_occlusion, "Sesame->C->Other->Removals->Skip Occlusion", oxui::object_checkbox );

	if ( !skip_occlusion || !g::local )
		return list_leaves_in_box ( REG_OUT, mins, maxs, list, list_max );

	struct render_info_t {
		void* m_renderable;
		void* m_alpha_property;
		int m_enum_count;
		int m_render_frame;
		unsigned short m_first_shadow;
		unsigned short m_leaf_list;
		short m_area;
		uint16_t m_flags;
		uint16_t m_flags2;
		vec3_t m_bloated_abs_mins;
		vec3_t m_bloated_abs_maxs;
		vec3_t m_abs_mins;
		vec3_t m_abs_maxs;
		PAD ( 4 );
	};

	static auto ret_addr = pattern::search ( _ ( "client.dll" ), _ ( "56 52 FF 50 18" ) ).add( 5 ).get< void* > ( );

	if ( _ReturnAddress ( ) != ret_addr )
		return list_leaves_in_box ( REG_OUT, mins, maxs, list, list_max );

	auto info = *( render_info_t** ) ( ( uintptr_t ) _AddressOfReturnAddress ( ) + 0x14 );

	if ( !info || !info->m_renderable )
		return list_leaves_in_box ( REG_OUT, mins, maxs, list, list_max );

	auto get_client_unknown = [ ] ( void* renderable ) {
		typedef void* ( __thiscall* o_fn )( void* );
		return ( *( o_fn** ) renderable ) [ 0 ] ( renderable );
	};

	auto get_base_entity = [ ] ( void* c_unk ) {
		typedef player_t* ( __thiscall* o_fn )( void* );
		return ( *( o_fn** ) c_unk ) [ 7 ] ( c_unk );
	};

	auto base_entity = get_base_entity ( get_client_unknown ( info->m_renderable ) );

	if ( !base_entity || base_entity->idx( ) > 64 || !base_entity->idx( ) )
		return list_leaves_in_box ( REG_OUT, mins, maxs, list, list_max );

	info->m_flags &= ~0x100;
	info->m_flags2 |= 0x40;

	static vec3_t map_min = vec3_t ( -16384.0f, -16384.0f, -16384.0f );
	static vec3_t map_max = vec3_t ( 16384.0f, 16384.0f, 16384.0f );

	return list_leaves_in_box ( REG_OUT, map_min, map_max, list, list_max );
}

void __fastcall hooks::paint_traverse_hk ( REG, int ipanel, bool force_repaint, bool allow_force ) {
	OPTION ( bool, no_scope, "Sesame->C->Other->Removals->No Scope", oxui::object_checkbox );

	static auto override_post_processing_disable = pattern::search ( _ ( "client.dll" ), _ ( "80 3D ? ? ? ? ? 53 56 57 0F 85" ) ).add ( 2 ).deref ( ).get< bool* > ( );

	if ( no_scope && !strcmp ( _ ( "HudZoom" ), csgo::i::panel->get_name ( ipanel ) ) )
		return;

	paint_traverse ( REG_OUT, ipanel, force_repaint, allow_force );

	*override_post_processing_disable = g::local && g::local->scoped( ) && no_scope;
}

bool __fastcall hooks::send_net_msg_hk ( REG, void* msg, bool force_reliable, bool voice ) {	if ( vfunc < int ( __thiscall* )( void* ) > ( msg, 8 )( msg ) == 9 )
		voice = true;

	return send_net_msg ( REG_OUT, msg, force_reliable, voice );
}

int __fastcall hooks::emit_sound_hk ( REG, void* filter, int ent_idx, int chan, const char* sound_entry, unsigned int sound_entry_hash, const char* sample, float volume, float attenuation, int seed, int flags, int pitch, const vec3_t* origin, const vec3_t* dir, vec3_t* vec_origins, bool update_positions, float sound_time, int speaker_ent, void* sound_params ) {
	OPTION ( double, revolver_cock_volume, "Sesame->E->Effects->Main->Revolver Cock Volume", oxui::object_slider );
	OPTION ( double, weapon_volume, "Sesame->E->Effects->Main->Weapon Volume", oxui::object_slider );
	
	if ( !strcmp ( sound_entry, _ ( "Weapon_Revolver.Prepare" ) ) && revolver_cock_volume != 1.0 )
		volume *= revolver_cock_volume;
	else if ( strstr ( sound_entry, _ ( "Weapon_" ) ) )
		volume *= weapon_volume;
	
	return emit_sound ( REG_OUT, filter, ent_idx, chan, sound_entry, sound_entry_hash, sample, volume, attenuation, seed, flags, pitch, origin, dir, vec_origins, update_positions, sound_time, speaker_ent, sound_params );
}

void __cdecl hooks::cs_blood_spray_callback_hk( const effect_data_t& effect_data ) {
	animations::resolver::process_blood ( effect_data );
	cs_blood_spray_callback ( effect_data );
}

void __fastcall hooks::modify_eye_pos_hk ( REG, vec3_t& pos ) {
	const auto state = reinterpret_cast < animstate_t* > ( ecx );
	const auto pl = state->m_entity;

	using bone_lookup_fn = int ( __thiscall* )( void*, const char* );
	static auto lookup_bone = pattern::search ( _ ( "client.dll" ), _ ( "55 8B EC 53 56 8B F1 57 83 BE ? ? ? ? ? 75" ) ).get<bone_lookup_fn> ( );

	if ( !g::local->valid ( ) || !pl->valid ( ) || pl != g::local )
		return modify_eye_pos ( REG_OUT, pos );

	if ( !in_cm && !ducking )
		return modify_eye_pos ( REG_OUT, pos );

	if ( !state->m_hit_ground && state->m_duck_amount == 0.0f && csgo::i::ent_list->get_by_handle< entity_t* > ( pl->ground_entity_handle ( ) ) )
		return;

	auto bone_idx = lookup_bone ( pl, _ ( "head_0" ) );
	vec3_t bone_pos = animations::data::fixed_bones [ g::local->idx ( ) ][ bone_idx ].origin ( );
	bone_pos.z += 1.7f;

	if ( bone_pos.z < pos.z ) {
		auto lerp = std::clamp< float > ( ( std::fabsf ( pos.z - bone_pos.z ) - 4.0f ) / 6.0f, 0.0f, 1.0f );
		pos.z += ( ( bone_pos.z - pos.z ) * ( ( ( lerp * lerp ) * 3.0f ) - ( ( ( lerp * lerp ) * 2.0f ) * lerp ) ) );
	}
}

long __stdcall hooks::wndproc( HWND hwnd, std::uint32_t msg, std::uintptr_t wparam, std::uint32_t lparam ) {
	auto skip_mouse_input_processing = false;

	switch ( msg ) {
	case WM_LBUTTONDOWN: case WM_LBUTTONDBLCLK:
	case WM_RBUTTONDOWN: case WM_RBUTTONDBLCLK:
	case WM_MBUTTONDOWN: case WM_MBUTTONDBLCLK:
	case WM_XBUTTONDOWN: case WM_XBUTTONDBLCLK: {
		int button = 0;
		if ( msg == WM_LBUTTONDOWN || msg == WM_LBUTTONDBLCLK ) { button = 0; }
		if ( msg == WM_RBUTTONDOWN || msg == WM_RBUTTONDBLCLK ) { button = 1; }
		if ( msg == WM_MBUTTONDOWN || msg == WM_MBUTTONDBLCLK ) { button = 2; }
		if ( msg == WM_XBUTTONDOWN || msg == WM_XBUTTONDBLCLK ) { button = ( GET_XBUTTON_WPARAM ( wparam ) == XBUTTON1 ) ? 3 : 4; }
		mouse_down [ button ] = true;
		skip_mouse_input_processing = true;
		break;
	}
	case WM_LBUTTONUP:
	case WM_RBUTTONUP:
	case WM_MBUTTONUP:
	case WM_XBUTTONUP: {
		int button = 0;
		if ( msg == WM_LBUTTONUP ) { button = 0; }
		if ( msg == WM_RBUTTONUP ) { button = 1; }
		if ( msg == WM_MBUTTONUP ) { button = 2; }
		if ( msg == WM_XBUTTONUP ) { button = ( GET_XBUTTON_WPARAM ( wparam ) == XBUTTON1 ) ? 3 : 4; }
		mouse_down [ button ] = false;
		skip_mouse_input_processing = true;
		break;
	}
	case WM_KEYDOWN:
	case WM_SYSKEYDOWN:
		if ( wparam < 256 )
			key_down [ wparam ] = true;
		break;
	case WM_KEYUP:
	case WM_SYSKEYUP:
		if ( wparam < 256 )
			key_down [ wparam ] = false;
		break;
	}

	menu::wndproc ( hwnd, msg, wparam, lparam );

	if ( menu::open( ) && ( ( skip_mouse_input_processing || wparam <= VK_XBUTTON2 ) || ( msg == WM_MOUSEWHEEL ) ) )
		return true;

	return CallWindowProcA( o_wndproc, hwnd, msg, wparam, lparam );
}

bool hooks::init( ) {
	unsigned long font_count = 0;
	LI_FN( AddFontMemResourceEx ) ( sesame_font_data, sizeof sesame_font_data, nullptr, &font_count );

	menu::init( );
	erase::erase_func ( menu::init );

	/* create fonts */
	render::create_font( ( void** ) &features::esp::dbg_font, _( L"Segoe UI" ), N( 12 ), false );
	render::create_font( ( void** ) &features::esp::esp_font, _( L"Segoe UI" ), N( 18 ), false );
	render::create_font ( ( void** ) &features::esp::indicator_font, _ ( L"Segoe UI" ), N ( 14 ), true );
	render::create_font( ( void** ) &features::esp::watermark_font, _( L"Segoe UI" ), N( 18 ), false );

	/* load default config */
	//menu::load_default( );

	const auto clsm_numUsrCmdProcessTicksMax_clamp = pattern::search( _( "engine.dll" ), _( "0F 4F F0 89 5D FC" ) ).get< void* >( );

	if ( clsm_numUsrCmdProcessTicksMax_clamp ) {
		unsigned long old_prot = 0;
		LI_FN( VirtualProtect )( clsm_numUsrCmdProcessTicksMax_clamp, 3, PAGE_EXECUTE_READWRITE, &old_prot );
		memset( clsm_numUsrCmdProcessTicksMax_clamp, 0x90, 3 );
		LI_FN ( VirtualProtect )( clsm_numUsrCmdProcessTicksMax_clamp, 3, old_prot, &old_prot );
	}

	const auto clsendmove = pattern::search( _( "engine.dll" ), _( "E8 ? ? ? ? 84 DB 0F 84 ? ? ? ? 8B 8F" ) ).resolve_rip( ).get< void* >( );
	const auto clientmodeshared_createmove = pattern::search( _( "client.dll" ), _( "55 8B EC 8B 0D ? ? ? ? 85 C9 75 06 B0" ) ).get< decltype( &createmove_hk ) >( );
	const auto chlclient_framestagenotify = pattern::search( _( "client.dll" ), _( "55 8B EC 8B 0D ? ? ? ? 8B 01 8B 80 74 01 00 00 FF D0 A2" ) ).get< decltype( &framestagenotify_hk ) >( );
	const auto idirect3ddevice9_endscene = vfunc< decltype( &endscene_hk ) >( csgo::i::dev, N( 42 ) );
	const auto idirect3ddevice9_reset = vfunc< decltype( &reset_hk ) >( csgo::i::dev, N( 16 ) );
	const auto vguimatsurface_lockcursor = pattern::search( _( "vguimatsurface.dll" ), _( "80 3D ? ? ? ? 00 8B 91 A4 02 00 00 8B 0D ? ? ? ? C6 05 ? ? ? ? 01" ) ).get< decltype( &lockcursor_hk ) >( );
	const auto ivrenderview_sceneend = vfunc< decltype( &sceneend_hk ) >( csgo::i::render_view, N( 9 ) );
	const auto modelrender_drawmodelexecute = vfunc< decltype( &drawmodelexecute_hk ) >( csgo::i::mdl_render, N( 21 ) );
	const auto c_csplayer_doextraboneprocessing = pattern::search( _( "client.dll" ), _( "55 8B EC 83 E4 F8 81 EC FC 00 00 00 53 56 8B F1 57" ) ).get< void* >( );
	const auto c_baseanimating_setupbones = pattern::search( _( "client.dll" ), _( "55 8B EC 83 E4 F0 B8 ? ? ? ? E8 ? ? ? ? 56 57 8B F9" ) ).get< void* >( );
	const auto c_csplayer_get_eye_angles = pattern::search( _( "client.dll" ), _( "56 8B F1 85 F6 74 32" ) ).get< void* >( );
	const auto c_csgoplayeranimstate_setupvelocity = pattern::search( _( "client.dll" ), _( "55 8B EC 83 E4 F8 83 EC 30 56 57 8B 3D" ) ).get< void* >( );
	const auto c_csgoplayeranimstate_setupvelocity_call = pattern::search( _( "client.dll" ), _( "E8 ? ? ? ? E9 ? ? ? ? 83 BE" ) ).resolve_rip( ).add( 0x366 ).get< void* >( );
	const auto draw_bullet_impacts = pattern::search( _( "client.dll" ), _( "56 8B 71 4C 57" ) ).get< void* >( );
	const auto convar_getbool = pattern::search( _( "client.dll" ), _( "8B 51 1C 3B D1 75 06" ) ).get< void* >( );
	const auto traceray_fn = vfunc< void* >( csgo::i::trace, N( 5 ) );
	const auto overrideview = pattern::search( _( "client.dll" ), _( "55 8B EC 83 E4 F8 83 EC 58 56 57 8B 3D ? ? ? ? 85 FF" ) ).get< void* >( );
	const auto updateclientsideanimations = pattern::search( _( "client.dll" ), _( "8B 0D ? ? ? ? 53 56 57 8B 99 0C 10 00 00 85 DB 74 ? 6A 04 6A 00" ) ).get< void* >( );
	const auto nc_send_datagram = pattern::search( _( "engine.dll" ), _( "55 8B EC 83 E4 F0 B8 ? ? ? ? E8 ? ? ? ? 56 57 8B F9 89 7C 24 18" ) ).get<void*>( );
	const auto engine_fire_event = vfunc< void* >( csgo::i::engine, N( 59 ) );
	const auto c_baseanimating_should_skip_animation_frame = pattern::search( _( "client.dll" ), _( "E8 ? ? ? ? 88 44 24 0B" ) ).add( N( 1 ) ).deref( ).get< void* >( );
	const auto engine_is_hltv = vfunc< void* >( csgo::i::engine, N( 93 ) );
	const auto chlclient_write_usercmd = vfunc< void* > ( csgo::i::client, N ( 24 ) );
	const auto bsp_query_list_leaves_in_box = vfunc< void* >( csgo::i::engine->get_bsp_tree_query( ), N( 6 ) );
	const auto panel_painttraverse = vfunc< void* > ( csgo::i::panel, N ( 41 ) );
	const auto clientmode_get_viewmodel_fov = vfunc< void* > ( **( void*** ) ( ( *( uintptr_t** ) csgo::i::client ) [ 10 ] + 5 ), N ( 35 ) );
	const auto pred_in_prediction = vfunc< void* > ( csgo::i::pred, N( 14 ) );
	const auto nc_sendnetmsg = pattern::search ( _ ( "engine.dll" ), _ ( "55 8B EC 83 EC 08 56 8B F1 8B 86 ? ? ? ? 85 C0" ) ).get<void*> ( );
	const auto ienginesound_emitsound = pattern::search ( _ ( "engine.dll" ), _ ( "E8 ? ? ? ? 8B E5 5D C2 3C 00 55" ) ).resolve_rip ( ).get<void*> ( );
	const auto global_cs_blood_spray_callback = pattern::search ( _ ( "client.dll" ), _ ( "55 8B EC 8B 4D 08 F3 0F 10 51 ? 8D 51 18" ) ).get<void*> ( );
	const auto modify_eye_position = pattern::search ( _ ( "client.dll" ), _ ( "57 E8 ? ? ? ? 8B 06 8B CE FF 90" ) ).add(1).resolve_rip().get<void*> ( );

	MH_Initialize( );

#define to_string( func ) #func
#define dbg_hook( a, b, c ) print_and_hook ( a, b, c, _( to_string ( b ) ) )
	auto print_and_hook = [ ] ( void* from, void* to, void** original, const char* func_name ) {
		MH_CreateHook ( from, to, original );
		MH_EnableHook ( from );
		// dbg_print ( _ ( "Hooked: %s\n" ), func_name );
		//std::this_thread::sleep_for ( std::chrono::seconds ( N ( 1 ) ) );
	};

	dbg_hook ( panel_painttraverse, paint_traverse_hk, ( void** ) &paint_traverse );
	dbg_hook( engine_fire_event, fire_event_hk, ( void** ) &fire_event );
	dbg_hook( clientmodeshared_createmove, createmove_hk, ( void** ) &createmove );
	dbg_hook( chlclient_framestagenotify, framestagenotify_hk, ( void** ) &framestagenotify );
	dbg_hook( idirect3ddevice9_endscene, endscene_hk, ( void** ) &endscene );
	dbg_hook( idirect3ddevice9_reset, reset_hk, ( void** ) &reset );
	dbg_hook( vguimatsurface_lockcursor, lockcursor_hk, ( void** ) &lockcursor );
	dbg_hook( ivrenderview_sceneend, sceneend_hk, ( void** ) &sceneend );
	dbg_hook( modelrender_drawmodelexecute, drawmodelexecute_hk, ( void** ) &drawmodelexecute );
	dbg_hook( c_csplayer_doextraboneprocessing, doextraboneprocessing_hk, ( void** ) &doextraboneprocessing );
	dbg_hook( c_csplayer_get_eye_angles, get_eye_angles_hk, ( void** ) &get_eye_angles );
	// dbg_hook( traceray_fn, trace_ray_hk, ( void** ) &traceray );
	dbg_hook( c_baseanimating_setupbones, setupbones_hk, ( void** ) &setupbones );
	dbg_hook( convar_getbool, get_bool_hk, ( void** ) &get_bool );
	dbg_hook( overrideview, override_view_hk, ( void** ) &override_view );
	dbg_hook( engine_is_hltv, is_hltv_hk, ( void** ) &is_hltv );
	dbg_hook ( chlclient_write_usercmd, write_usercmd_hk, ( void** ) &write_usercmd );
	dbg_hook ( bsp_query_list_leaves_in_box, list_leaves_in_box_hk, ( void** ) &list_leaves_in_box );
	dbg_hook ( clientmode_get_viewmodel_fov, get_viewmodel_fov_hk, ( void** ) &get_viewmodel_fov );
	dbg_hook ( pred_in_prediction, in_prediction_hk, ( void** ) &in_prediction );
	//dbg_hook( nc_send_datagram, send_datagram_hk, ( void** ) &send_datagram );
	dbg_hook ( c_baseanimating_should_skip_animation_frame, should_skip_animation_frame_hk, ( void** ) &should_skip_animation_frame );
	dbg_hook ( nc_sendnetmsg, send_net_msg_hk, ( void** ) &send_net_msg );
	dbg_hook ( ienginesound_emitsound, emit_sound_hk, ( void** ) &emit_sound );
	dbg_hook ( global_cs_blood_spray_callback, cs_blood_spray_callback_hk, ( void** ) &cs_blood_spray_callback );
	dbg_hook( modify_eye_position, modify_eye_pos_hk, ( void** ) &modify_eye_pos );
	//dbg_hook( updateclientsideanimations, update_clientside_animations_hk, ( void** ) &update_clientside_animations );

	event_handler = std::make_unique< c_event_handler >( );

	o_wndproc = ( WNDPROC ) LI_FN( SetWindowLongA )( LI_FN( FindWindowA )( _( "Valve001" ), nullptr ), GWLP_WNDPROC, long( wndproc ) );

	END_FUNC

	return true;
}