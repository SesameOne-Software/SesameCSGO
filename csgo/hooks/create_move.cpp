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
	static auto& fd_key = options::vars [ _( "antiaim.fakeduck_key" ) ].val.i;
	static auto& fd_key_mode = options::vars [ _( "antiaim.fakeduck_key_mode" ) ].val.i;

	/* choke packets if requested */
	if ( g::local && g::local->weapon ( ) && g::local->weapon ( )->data ( )
		&& ( !!( ucmd->m_buttons & ( buttons_t::attack | ( g::local->weapon ( )->data ( )->m_type == weapon_type_t::knife ? buttons_t::attack2 : static_cast< buttons_t >( 0 ) ) ) ) && exploits::can_shoot ( ) )
		&& !( fd_enabled && utils::keybind_active( fd_key, fd_key_mode ) ) )
		g::send_packet = true;

	/* reset pitch as fast as possible after shot so our on-shot doesn't get completely raped */
	//if ( !features::ragebot::active_config.choke_on_shot && last_attack && !( ucmd->m_buttons & buttons_t::attack ) && !( fd_enabled && utils::keybind_active( fd_key, fd_key_mode ) ) )
	//	g::send_packet = true;

	last_attack = !!( ucmd->m_buttons & buttons_t::attack );
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

void fd_crouch ( ucmd_t* ucmd ) {
	static auto& fd_enabled = options::vars [ _ ( "antiaim.fakeduck" ) ].val.b;
	static auto& fd_key = options::vars [ _ ( "antiaim.fakeduck_key" ) ].val.i;
	static auto& fd_key_mode = options::vars [ _ ( "antiaim.fakeduck_key_mode" ) ].val.i;

	if ( fd_enabled && utils::keybind_active ( fd_key, fd_key_mode ) )
		ucmd->m_buttons |= buttons_t::duck;
}

int last_send_cmd = 0;

bool __fastcall hooks::create_move( REG, float sampletime, ucmd_t* ucmd ) {
	const auto ret = old::create_move ( REG_OUT, sampletime, ucmd );

	if ( !ucmd || !ucmd->m_cmdnum )
		return ret;

	if ( !exploits::in_exploit && ret ) {
		cs::i::pred->set_local_viewangles ( ucmd->m_angs );
		cs::i::engine->set_viewangles ( ucmd->m_angs );
	}

	utils::update_key_toggles( );

	/* recharge if we need, and return */
	if ( !exploits::in_exploit && exploits::recharge( ucmd ) ) 
		return false;

	in_cm = true;

	fd_crouch ( ucmd );

	ducking = !!( ucmd->m_buttons & buttons_t::duck );

	//RUN_SAFE (
	//	"features::ragebot::get_weapon_config",
	features::ragebot::get_weapon_config( features::ragebot::active_config );
	//);

	if ( !g::local || !g::local->alive( ) )
		g::cock_time = 0.0f;

	security_handler::update( );

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

	features::prediction::run ( [ & ] ( ) {
		const auto weapon = g::local->weapon ( );

		if ( weapon ) {
			if ( g::local && g::local->weapon ( ) )
				features::spread_circle::total_spread = weapon->inaccuracy ( ) + weapon->spread ( );
			else
				features::spread_circle::total_spread = 0.0f;
		}

		features::antiaim::simulate_lby ( );

		if ( !exploits::in_exploit ) {
			features::legitbot::run ( ucmd );

			if ( !!( ucmd->m_buttons & buttons_t::attack ) )
				exploits::extend_recharge_delay ( cs::time2ticks ( static_cast< float >( features::ragebot::active_config.dt_recharge_delay ) / 1000.0f ) );

			features::ragebot::run ( ucmd, old_smove, old_fmove, old_angs );

			if ( !!( ucmd->m_buttons & buttons_t::attack ) )
				exploits::extend_recharge_delay ( cs::time2ticks ( static_cast< float >( features::ragebot::active_config.dt_recharge_delay ) / 1000.0f ) );

			features::ragebot::tickbase_controller ( ucmd );
		}

		features::antiaim::run ( ucmd, old_smove, old_fmove, last_attack );
		features::autopeek::run ( ucmd, old_smove, old_fmove, old_angs );

		if ( !exploits::in_exploit ) {
			if ( !!( ucmd->m_buttons & buttons_t::attack ) )
				exploits::has_shifted = false;

			exploits::will_shift = false;

			/* recreate what holdaim var does */
			/* part of anims */ {
				if ( g::cvars::sv_maxusrcmdprocessticks_holdaim->get_bool ( ) ) {
					if ( !!( ucmd->m_buttons & buttons_t::attack ) ) {
						g::angles = ucmd->m_angs;
						g::hold_aim = true;
					}
				}
				else {
					g::hold_aim = false;
				}

				if ( !g::hold_aim )
					g::angles = ucmd->m_angs;

				if ( g::send_packet )
					g::hold_aim = false;
			}
		}
	} );

	if ( !exploits::in_exploit )
		fix_event_delay ( ucmd );

	/* auto-revolver */
	if ( g::local && g::local->weapon ( ) ) {
		const auto weapon = g::local->weapon ( );
		const auto server_time = cs::ticks2time ( g::local->tick_base ( ) );

		if ( g::local
			&& weapon->data ( )
			&& features::ragebot::active_config.auto_revolver
			&& weapon->item_definition_index ( ) == weapons_t::revolver
			&& !( ucmd->m_buttons & buttons_t::attack ) ) {
			ucmd->m_buttons &= ~buttons_t::attack2;

			const auto can_shoot = server_time >= g::local->next_attack ( )
				&& server_time >= weapon->next_primary_attack ( )
				/*&& weapon->postpone_fire_time ( ) < server_time*/;

			g::can_fire_revolver = true;

			if ( g::can_fire_revolver && can_shoot ) {
				if ( g::cock_time <= server_time ) {
					if ( weapon->next_secondary_attack ( ) <= server_time )
						g::cock_time = server_time + 0.234375f;
					else
						ucmd->m_buttons |= buttons_t::attack2;
				}
				else
					ucmd->m_buttons |= buttons_t::attack;

				g::can_fire_revolver = server_time > g::cock_time;
			}
			else {
				g::can_fire_revolver = false;
				g::cock_time = server_time + 0.234375f;
				ucmd->m_buttons &= ~buttons_t::attack;
			}
		}
		else {
			g::can_fire_revolver = false;
		}
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
	fix_slide ( ucmd );

	if ( g::send_packet )
		last_send_cmd = ucmd->m_cmdnum;

	if ( g::send_packet ) {
		g::sent_cmd = *ucmd;
		//animations::fake::simulate( );
	}

	ucmd->m_fmove = std::clamp< float >( ucmd->m_fmove, -g::cvars::cl_forwardspeed->get_float ( ), g::cvars::cl_forwardspeed->get_float ( ) );
	ucmd->m_smove = std::clamp< float >( ucmd->m_smove, -g::cvars::cl_sidespeed->get_float(), g::cvars::cl_sidespeed->get_float ( ) );

	/* airstuck (only on community servers) */
	airstuck ( ucmd );
	
	/* event delay fix */
	if ( cs::i::client_state ) {
		auto nc = cs::i::client_state->net_channel ( );
	
		if ( nc ) {
			auto& current_data = g::network_data [ ucmd->m_cmdnum % g::network_data.size ( ) ];

			current_data.out_sequence = ucmd->m_cmdnum;
			current_data.last_out_cmd = cs::i::client_state->last_outgoing_cmd ( );
	
			//nc->send_datagram ( nullptr );
			//
			//nc->choked_packets--;
			//nc->out_seq_nr--;
		}
	}

	//if ( g::send_packet && !cs::is_valve_server ( ) )
	//	g::outgoing_cmd_nums.push_front ( ucmd->m_cmdnum );
	//
	//if ( cs::i::client_state && !g::send_packet && !cs::is_valve_server ( ) ) {
	//	auto nc = cs::i::client_state->net_channel ( );
	//
	//	if ( nc ) {
	//		const auto backup_choked = nc->choked_packets;
	//
	//		nc->choked_packets = 0;
	//		nc->send_datagram ( nullptr );
	//		nc->out_seq_nr--;
	//		nc->choked_packets = backup_choked;
	//	}
	//}

	if ( !exploits::in_exploit )
		*reinterpret_cast< bool* >( *reinterpret_cast< uintptr_t* >( reinterpret_cast< uintptr_t >( _AddressOfReturnAddress ( ) ) - 4 ) - 28 ) = g::send_packet;

	if ( !exploits::in_exploit )
		exploits::run ( ucmd );

	in_cm = false;

	return false;
}