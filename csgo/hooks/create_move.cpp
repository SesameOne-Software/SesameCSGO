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

vec3_t old_origin;
bool in_cm = false;
bool ducking = false;
int restore_ticks = 0;
int cock_ticks = 0;
bool last_attack = false;
bool last_tickbase_shot = false;

void fix_event_delay ( ucmd_t* ucmd ) {
	OPTION ( int, _fd_mode, "Sesame->B->Other->Other->Fakeduck Mode", oxui::object_dropdown );
	KEYBIND ( _fd_key, "Sesame->B->Other->Other->Fakeduck Key" );

	/* choke packets if requested */
	if ( ucmd->m_buttons & 1 && g::local && !_fd_key )
		g::send_packet = true;

	/* reset pitch as fast as possible after shot so our on-shot doesn't get completely raped */
	if ( !features::ragebot::active_config.choke_on_shot && last_attack && !( ucmd->m_buttons & 1 ) && !( _fd_key && _fd_mode ) && !csgo::is_valve_server ( ) )
		g::send_packet = true;

	last_attack = ucmd->m_buttons & 1;
}

decltype( &hooks::create_move ) hooks::old::create_move = nullptr;

bool __fastcall hooks::create_move ( REG, float sampletime, ucmd_t* ucmd ) {
	ducking = ucmd->m_buttons & 4;
	in_cm = true;

	//RUN_SAFE (
	//	"features::ragebot::get_weapon_config",
		features::ragebot::get_weapon_config ( features::ragebot::active_config );
	//);

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
		prediction::disable_sounds = true;

		csgo::i::pred->update (
			csgo::i::client_state->delta_tick ( ),
			csgo::i::client_state->delta_tick ( ) > 0,
			csgo::i::client_state->last_command_ack ( ),
			csgo::i::client_state->last_outgoing_cmd ( ) + csgo::i::client_state->choked ( ) );

		prediction::disable_sounds = false;
	}

	const auto refresh_tickbase = g::shifted_tickbase - 1 > ucmd->m_cmdnum&& g::local&& g::local->alive ( );

	if ( refresh_tickbase && !csgo::is_valve_server ( ) ) {
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

	//RUN_SAFE (
	//	"features::esp::handle_dynamic_updates",
		features::esp::handle_dynamic_updates ( );
	//);

	auto ret = old::create_move ( REG_OUT, sampletime, ucmd );

	g::ucmd = ucmd;

	//RUN_SAFE (
	//	"features::prediction::update_curtime",
		features::prediction::update_curtime ( );
	//);

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
		g::shifted_tickbase = ucmd->m_cmdnum + std::clamp ( static_cast < int > ( features::ragebot::active_config.max_dt_ticks ), 0, csgo::is_valve_server ( ) ? 8 : 16 );
		g::next_tickbase_shot = false;
		last_tickbase_shot = false;
	}

	last_tickbase_shot = g::next_tickbase_shot;

	if ( !refresh_tickbase && g::local && g::local->weapon ( ) && g::local->weapon ( )->data ( ) && features::ragebot::active_config.auto_revolver && g::local->weapon ( )->item_definition_index ( ) == 64 && !( ucmd->m_buttons & 1 ) ) {
		if ( csgo::time2ticks ( features::prediction::predicted_curtime ) > cock_ticks ) {
			ucmd->m_buttons &= ~1;
			cock_ticks = csgo::time2ticks ( features::prediction::predicted_curtime + 0.25f ) - 1;
		}
		else {
			ucmd->m_buttons |= 1;
		}
	}

	csgo::clamp ( ucmd->m_angs );

	csgo::rotate_movement ( ucmd, old_smove, old_fmove, old_angs );

	*( bool* ) ( *( uintptr_t* ) ( uintptr_t ( _AddressOfReturnAddress ( ) ) - 4 ) - 28 ) = refresh_tickbase ? true : g::send_packet;

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