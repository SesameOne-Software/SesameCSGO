﻿#include "create_move.hpp"
#include "in_prediction.hpp"
#include "../globals.hpp"

#include "../features/ragebot.hpp"
#include "../features/movement.hpp"
#include "../features/esp.hpp"
#include "../features/clantag.hpp"
#include "../features/other_visuals.hpp"
#include "../features/nade_prediction.hpp"
#include "../features/antiaim.hpp"
#include "../features/legitbot.hpp"
#include "../features/blockbot.hpp"
#include "../features/autowall.hpp"
#include "../features/autopeek.hpp"
#include "../features/exploits.hpp"
#include "../features/prediction.hpp"

#include "../animations/resolver.hpp"

#include "../menu/options.hpp"

#undef min
#undef max

namespace lby {
	extern bool in_update;
}

vec3_t old_origin;
bool in_cm = false;
bool ducking = false;
int restore_ticks = 0;
bool last_attack = false;
bool delay_tick = false;
vec3_t old_angles;

bool flip_slide = false;
void fix_slide( ucmd_t* ucmd ) {
	if ( !g::local
		|| g::local->movetype( ) == movetypes_t::noclip
		|| g::local->movetype( ) == movetypes_t::ladder )
		return;

	static auto& should_slide = options::vars[ _( "antiaim.slide" ) ].val.b;
	static auto& jittermove = options::vars[ _( "antiaim.jittermove" ) ].val.b;

	auto fix_legs = [ & ] ( bool slide ) {
		if ( slide ) {
			if ( ucmd->m_fmove ) {
				ucmd->m_buttons &= ~( ucmd->m_fmove < 0.0f ? buttons_t::back : buttons_t::forward );
				ucmd->m_buttons |= ( ucmd->m_fmove > 0.0f ? buttons_t::back : buttons_t::forward );
			}

			if ( ucmd->m_smove ) {
				ucmd->m_buttons &= ~( ucmd->m_smove < 0.0f ? buttons_t::left : buttons_t::right );
				ucmd->m_buttons |= ( ucmd->m_smove > 0.0f ? buttons_t::left : buttons_t::right );
			}
			return;
		}

		ucmd->m_buttons &= ~( buttons_t::right | buttons_t::left | buttons_t::back | buttons_t::forward );
	};

	fix_legs( jittermove ? flip_slide : should_slide );

	if ( g::send_packet )
		flip_slide = !flip_slide;
}

void fix_event_delay( ucmd_t* ucmd ) {
	static auto& fd_enabled = options::vars [ _( "antiaim.fakeduck" ) ].val.b;
	static auto& fd_mode = options::vars [ _( "antiaim.fakeduck_mode" ) ].val.i;
	static auto& fd_key = options::vars [ _( "antiaim.fakeduck_key" ) ].val.i;
	static auto& fd_key_mode = options::vars [ _( "antiaim.fd_key_mode" ) ].val.i;

	/* choke packets if requested */
	if ( !!(ucmd->m_buttons & buttons_t::attack) && g::local && !( fd_enabled && utils::keybind_active( fd_key, fd_key_mode ) ) )
		g::send_packet = true;

	/* reset pitch as fast as possible after shot so our on-shot doesn't get completely raped */
	//if ( !features::ragebot::active_config.choke_on_shot && last_attack && !( ucmd->m_buttons & buttons_t::attack ) && !( fd_enabled && utils::keybind_active( fd_key, fd_key_mode ) ) && !cs::is_valve_server( ) )
	//	g::send_packet = true;

	last_attack = !!(ucmd->m_buttons & buttons_t::attack);
}

decltype( &hooks::create_move ) hooks::old::create_move = nullptr;

void airstuck ( ucmd_t* ucmd ) {
	static auto& airstuck = options::vars [ _ ( "misc.movement.airstuck" ) ].val.b;
	static auto& airstuck_key = options::vars [ _ ( "misc.movement.airstuck_key" ) ].val.i;
	static auto& airstuck_mode = options::vars [ _ ( "misc.movement.airstuck_key_mode" ) ].val.i;

	if ( g::local && g::local->alive() && airstuck && utils::keybind_active ( airstuck_key, airstuck_mode ) && !cs::is_valve_server() && g::round != round_t::starting ) {
		ucmd->m_tickcount = std::numeric_limits<int>::max ( );
		ucmd->m_cmdnum = std::numeric_limits<int>::max ( );
	}
}

void break_bt ( ucmd_t* ucmd ) {
	static auto& break_bt = options::vars [ _ ( "antiaim.break_backtrack" ) ].val.b;
	static auto& break_bt_key = options::vars [ _ ( "antiaim.break_backtrack_key" ) ].val.i;
	static auto& break_bt_mode = options::vars [ _ ( "antiaim.break_backtrack_key_mode" ) ].val.i;

	if ( g::local && g::local->alive ( ) && break_bt && utils::keybind_active ( break_bt_key, break_bt_mode ) && !cs::is_valve_server ( ) && g::send_packet && g::round != round_t::starting ) {
		ucmd->m_tickcount = std::numeric_limits<int>::max ( );
		ucmd->m_cmdnum = std::numeric_limits<int>::max ( );
	}
}

int last_send_cmd = 0;

void reject_bad_shots ( ucmd_t* ucmd ) {
	if ( g::local && g::local->alive ( ) && cs::i::client_state ) {
		if ( g::local->velocity_modifier ( ) - features::prediction::vel_modifier < -0.1f ) {
			//for ( auto i = 0; i < cs::i::client_state->choked ( ) + 1; i++ ) {
			//	auto next_verified_cmd = cs::i::input->get_verified_cmd ( last_send_cmd + i );
			//	auto next_cmd = cs::i::input->get_cmd ( last_send_cmd + i );
			//
			//	if ( next_cmd && ( !!( next_cmd->m_buttons & buttons_t::attack ) || !!( next_cmd->m_buttons & buttons_t::attack2 ) ) ) {
			//		next_cmd->m_buttons &= ~buttons_t::attack;
			//		next_cmd->m_buttons &= ~buttons_t::attack2;
			//
			//		next_verified_cmd->m_cmd = *next_cmd;
			//		next_verified_cmd->m_crc = next_cmd->get_checksum ( );
			//	}
			//}

			*reinterpret_cast< bool* > ( reinterpret_cast< uintptr_t >( cs::i::pred ) + 0x24 ) = true;
		}

		features::prediction::vel_modifier = g::local->velocity_modifier ( );
	}
}

bool __fastcall hooks::create_move( REG, float sampletime, ucmd_t* ucmd ) {
	if ( !exploits::in_exploit ) {
		const auto ret = old::create_move ( REG_OUT, sampletime, ucmd );

		if ( ucmd ) {
			cs::i::engine->set_viewangles ( ucmd->m_angs );
			cs::i::pred->set_local_viewangles ( ucmd->m_angs );
		}

		if ( !ucmd || !ucmd->m_cmdnum )
			return ret;
	}

	ducking = !!(ucmd->m_buttons & buttons_t::duck);
	in_cm = true;

	utils::update_key_toggles( );

	/* recharge if we need, and return */
	if ( !exploits::in_exploit && exploits::recharge( ucmd ) ) {
		in_cm = false;
		return false;
	}

	if ( cs::i::client_state->choked ( ) && (features::ragebot::active_config.dt_teleport ? true : !exploits::in_exploit )) {
		cs::i::pred->update (
			cs::i::client_state->delta_tick ( ),
			cs::i::client_state->delta_tick ( ) > 0,
			cs::i::client_state->last_command_ack ( ),
			cs::i::client_state->last_outgoing_cmd ( ) + cs::i::client_state->choked ( )
		);
	}

	//RUN_SAFE (
	//	"features::ragebot::get_weapon_config",
	features::ragebot::get_weapon_config( features::ragebot::active_config );
	//);

	if ( !g::local || !g::local->alive( ) )
		g::cock_ticks = 0;

	security_handler::update( );

	if ( g::local && g::local->weapon( ) ) {
		const auto weapon = g::local->weapon( );
		weapon->update_accuracy( );
		features::spread_circle::total_spread = weapon->inaccuracy( ) + weapon->spread( );
	}
	else {
		features::spread_circle::total_spread = 0.0f;
	}

	if ( !exploits::in_exploit )
		features::esp::handle_dynamic_updates( );

	g::ucmd = ucmd;

	auto old_angs = ucmd->m_angs;

	if ( !exploits::in_exploit )
		features::clantag::run( ucmd );

	features::movement::run( ucmd, old_angs );

	features::blockbot::run( ucmd, old_angs );

	auto old_smove = ucmd->m_smove;
	auto old_fmove = ucmd->m_fmove;

	if ( !exploits::in_exploit ) {
		cs::for_each_player ( [ ] ( player_t* pl ) {
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
	}

	if ( !exploits::in_exploit )
		features::nade_prediction::trace( ucmd );

	features::prediction::run( [ & ] ( ) {
		features::antiaim::simulate_lby( );
		ducking = !!(ucmd->m_buttons & buttons_t::duck);

		features::legitbot::run( ucmd );

		//if ( !exploits::in_exploit )
			features::ragebot::run( ucmd, old_smove, old_fmove, old_angs );

		features::antiaim::run( ucmd, old_smove, old_fmove );

		features::autopeek::run ( ucmd, old_smove, old_fmove, old_angs );
		} );

	if( !!(ucmd->m_buttons & buttons_t::attack) )
		exploits::has_shifted = false;

	reject_bad_shots ( ucmd );

	fix_event_delay( ucmd );

	if ( g::local && g::local->weapon( ) && g::local->weapon( )->data( ) && features::ragebot::active_config.auto_revolver && g::local->weapon( )->item_definition_index( ) == weapons_t::revolver && !( ucmd->m_buttons & buttons_t::attack ) ) {
		if ( g::local->tick_base ( ) > g::cock_ticks ) {
			ucmd->m_buttons &= ~buttons_t::attack;
			g::cock_ticks = g::local->tick_base ( ) + cs::time2ticks(0.25f) - 1;
			g::can_fire_revolver = true;
		}
		else {
			ucmd->m_buttons |= buttons_t::attack;
			g::can_fire_revolver = false;
		}
	}
	else {
		g::can_fire_revolver = false;
	}

	cs::clamp( ucmd->m_angs );

	if ( !( ucmd->m_buttons & buttons_t::attack ) )
		old_angles = ucmd->m_angs;

	vec3_t engine_angs;
	cs::i::engine->get_viewangles( engine_angs );
	cs::clamp( engine_angs );
	cs::i::engine->set_viewangles( engine_angs );

	cs::rotate_movement( ucmd, old_smove, old_fmove, old_angs );

	//break_bt ( ucmd );

	*( bool* )( *( uintptr_t* )( uintptr_t( _AddressOfReturnAddress( ) ) - 4 ) - 28 ) = g::send_packet;

	if ( g::send_packet )
		last_send_cmd = ucmd->m_cmdnum;

	fix_slide( ucmd );

	features::ragebot::tickbase_controller( ucmd );

	if ( g::send_packet ) {
		g::sent_cmd = *ucmd;
		//animations::fake::simulate( );
	}

	ucmd->m_fmove = std::clamp< float >( ucmd->m_fmove, -g::cvars::cl_forwardspeed->get_float ( ), g::cvars::cl_forwardspeed->get_float ( ) );
	ucmd->m_smove = std::clamp< float >( ucmd->m_smove, -g::cvars::cl_sidespeed->get_float(), g::cvars::cl_sidespeed->get_float ( ) );

	in_cm = false;

	/* airstuck (only on community servers) */
	airstuck ( ucmd );

	if ( !exploits::in_exploit )
		exploits::run ( ucmd );

	/* recreate what holdaim var does */
	/* part of anims */ {
		if ( g::cvars::sv_maxusrcmdprocessticks_holdaim->get_bool( ) ) {
			if ( !!( ucmd->m_buttons & buttons_t::attack ) ) {
				g::angles = ucmd->m_angs;
				g::hold_aim = true;
			}
		}
		else {
			g::hold_aim = false;
		}

		if ( !g::hold_aim ) {
			g::angles = ucmd->m_angs;
		}

		if ( g::send_packet )
			g::hold_aim = false;
	}

	return false;
}