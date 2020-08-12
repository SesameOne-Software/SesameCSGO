#include "create_move.hpp"
#include "in_prediction.hpp"
#include "../globals.hpp"

#include "../features/ragebot.hpp"
#include "../features/movement.hpp"
#include "../features/esp.hpp"
#include "../features/clantag.hpp"
#include "../features/other_visuals.hpp"
#include "../features/nade_prediction.hpp"
#include "../features/antiaim.hpp"
#include "../animations/resolver.hpp"
#include "../javascript/js_api.hpp"

#include "../features/autowall.hpp"

#include "../menu/options.hpp"

vec3_t old_origin;
bool in_cm = false;
bool ducking = false;
int restore_ticks = 0;
bool last_attack = false;
bool last_tickbase_shot = false;
int g_refresh_counter = 0;

bool hooks::vars::in_refresh = false;

void fix_event_delay ( ucmd_t* ucmd ) {
	static auto& fd_enabled = options::vars [ _ ( "antiaim.fakeduck" ) ].val.b;
	static auto& fd_mode = options::vars [ _ ( "antiaim.fakeduck_mode" ) ].val.i;
	static auto& fd_key = options::vars [ _ ( "antiaim.fakeduck_key" ) ].val.i;
	static auto& fd_key_mode = options::vars [ _ ( "antiaim.fd_key_mode" ) ].val.i;

	/* choke packets if requested */
	if ( ucmd->m_buttons & 1 && g::local && !(fd_enabled && utils::keybind_active(fd_key, fd_key_mode )) )
		g::send_packet = true;

	/* reset pitch as fast as possible after shot so our on-shot doesn't get completely raped */
	if ( !features::ragebot::active_config.choke_on_shot && last_attack && !( ucmd->m_buttons & 1 ) && !( fd_enabled && utils::keybind_active ( fd_key, fd_key_mode ) ) && !csgo::is_valve_server ( ) )
		g::send_packet = true;

	last_attack = ucmd->m_buttons & 1;
}

decltype( &hooks::create_move ) hooks::old::create_move = nullptr;

bool __fastcall hooks::create_move ( REG, float sampletime, ucmd_t* ucmd ) {
	ducking = ucmd->m_buttons & 4;
	in_cm = true;

	utils::update_key_toggles ( );

	//RUN_SAFE (
	//	"features::ragebot::get_weapon_config",
		features::ragebot::get_weapon_config ( features::ragebot::active_config );
	//);

	if ( !g::local || !g::local->alive ( ) ) {
		g::cock_ticks = 0;
	}

	if ( !ucmd || !ucmd->m_cmdnum ) {
		in_cm = false;
		return false;
	}

	security_handler::update ( );

	if ( g_refresh_counter < g::shifted_amount ) {
		//*( bool* ) ( *( uintptr_t* ) ( uintptr_t ( _AddressOfReturnAddress ( ) ) - 4 ) - 28 ) = true;
		ucmd->m_tickcount += 666;
		in_cm = false;
		g_refresh_counter++;
		vars::in_refresh = true;
	}
	else {
		vars::in_refresh = false;
	}

	features::prediction::update_curtime ( );

	/* thanks chambers */
	if ( csgo::i::client_state->choked ( ) ) {
		prediction::disable_sounds = true;

		csgo::i::pred->update (
			csgo::i::client_state->delta_tick ( ),
			csgo::i::client_state->delta_tick ( ) > 0,
			csgo::i::client_state->last_command_ack ( ),
			csgo::i::client_state->last_outgoing_cmd ( ) + csgo::i::client_state->choked ( ) );

		prediction::disable_sounds = false;
	}

	if ( g::local && g::local->weapon ( ) ) {
		const auto weapon = g::local->weapon ( );
		weapon->update_accuracy ( );
		features::spread_circle::total_spread = weapon->inaccuracy ( ) + weapon->spread ( );
	}
	else {
		features::spread_circle::total_spread = 0.0f;
	}

	//RUN_SAFE (
	//	"features::esp::handle_dynamic_updates",
		features::esp::handle_dynamic_updates ( );
	//);

	auto ret = old::create_move ( REG_OUT, sampletime, ucmd );

	g::ucmd = ucmd;

	auto old_angs = ucmd->m_angs;

	//RUN_SAFE (
	//	"features::clantag::run",
		features::clantag::run ( ucmd );
	//);

	//RUN_SAFE (
	//	"features::movement::run",
		features::movement::run ( ucmd, old_angs );
	//);

	auto old_smove = ucmd->m_smove;
	auto old_fmove = ucmd->m_fmove;

	//RUN_SAFE (
	//	"csgo::for_each_player",
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
	} );
	//);

	//RUN_SAFE (
	//	"features::nade_prediction::trace",
		features::nade_prediction::trace ( ucmd );
	//);

	//RUN_SAFE (
	//	"features::prediction::run",
		features::prediction::run ( [ & ] ( ) {
		csgo::for_each_player ( [ ] ( player_t* pl ) {
			//RUN_SAFE (
			//	"features::lagcomp::pop",
				features::lagcomp::pop ( pl );
			//);
		} );

		features::antiaim::simulate_lby ( );
		ducking = ucmd->m_buttons & 4;

		//RUN_SAFE (
		//	"features::ragebot::run",
		if (!vars::in_refresh )
			features::ragebot::run ( ucmd, old_smove, old_fmove, old_angs );
		//);

		//RUN_SAFE (
		//	"js::process_create_move_callbacks",
			js::process_create_move_callbacks ( );
		//);

		//RUN_SAFE (
		//	"features::antiaim::run",
			features::antiaim::run ( ucmd, old_smove, old_fmove );
		//);

		//RUN_SAFE (
		//	"animations::resolver::update",
			animations::resolver::update ( ucmd );
		//);
	} );
	//);

	fix_event_delay ( ucmd );

	if ( ucmd->m_buttons & 1 )
		g::next_tickbase_shot = false;

	if ( !g::next_tickbase_shot && last_tickbase_shot ) {
		g_refresh_counter = 0;
		g::next_tickbase_shot = false;
		last_tickbase_shot = false;
	}

	last_tickbase_shot = g::next_tickbase_shot;

	if ( !vars::in_refresh && g::local && g::local->weapon ( ) && g::local->weapon ( )->data ( ) && features::ragebot::active_config.auto_revolver && g::local->weapon ( )->item_definition_index ( ) == 64 && !( ucmd->m_buttons & 1 ) ) {
		if ( csgo::time2ticks ( features::prediction::predicted_curtime ) > g::cock_ticks ) {
			ucmd->m_buttons &= ~1;
			g::cock_ticks = csgo::time2ticks ( features::prediction::predicted_curtime + 0.25f ) - 2;
			g::can_fire_revolver = true;
		}
		else {
			ucmd->m_buttons |= 1;
			g::can_fire_revolver = false;
		}
	}
	else {
		g::can_fire_revolver = false;
	}

	csgo::clamp ( ucmd->m_angs );

	vec3_t engine_angs;
	csgo::i::engine->get_viewangles ( engine_angs );
	csgo::clamp ( engine_angs );
	csgo::i::engine->set_viewangles ( engine_angs );

	csgo::rotate_movement ( ucmd, old_smove, old_fmove, old_angs );

	*( bool* ) ( *( uintptr_t* ) ( uintptr_t ( _AddressOfReturnAddress ( ) ) - 4 ) - 28 ) = g::send_packet;

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

	features::ragebot::tickbase_controller ( ucmd );

	if ( g::send_packet ) {
		g::sent_cmd = *ucmd;
		//animations::fake::simulate( );
	}

	ucmd->m_fmove = std::clamp< float > ( ucmd->m_fmove, -450.0f, 450.0f );
	ucmd->m_smove = std::clamp< float > ( ucmd->m_smove, -450.0f, 450.0f );

	in_cm = false;
	return false;
}