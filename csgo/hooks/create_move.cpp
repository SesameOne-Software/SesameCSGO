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

#include "../animations/resolver.hpp"

#include "../menu/options.hpp"

#undef min
#undef max

vec3_t old_origin;
bool in_cm = false;
bool ducking = false;
int restore_ticks = 0;
bool last_attack = false;
bool delay_tick = false;
vec3_t old_angles;

void fix_event_delay( ucmd_t* ucmd ) {
	static auto& fd_enabled = options::vars [ _( "antiaim.fakeduck" ) ].val.b;
	static auto& fd_mode = options::vars [ _( "antiaim.fakeduck_mode" ) ].val.i;
	static auto& fd_key = options::vars [ _( "antiaim.fakeduck_key" ) ].val.i;
	static auto& fd_key_mode = options::vars [ _( "antiaim.fd_key_mode" ) ].val.i;

	/* choke packets if requested */
	if ( !!(ucmd->m_buttons & buttons_t::attack) && g::local && !( fd_enabled && utils::keybind_active( fd_key, fd_key_mode ) ) && !cs::is_valve_server( ) )
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

	if ( g::local && g::local->alive() && airstuck && utils::keybind_active ( airstuck_key, airstuck_mode ) && !cs::is_valve_server() ) {
		ucmd->m_tickcount = std::numeric_limits<int>::min ( );
		ucmd->m_cmdnum = std::numeric_limits<int>::min ( );
	}
}

bool __fastcall hooks::create_move( REG, float sampletime, ucmd_t* ucmd ) {
	auto ret = old::create_move( REG_OUT, sampletime, ucmd );

	if ( !ucmd || !ucmd->m_cmdnum )
		return ret;

	anims::new_tick = true;

	if ( ret )
		cs::i::pred->set_local_viewangles ( ucmd->m_angs );

	if ( cs::i::client_state->choked ( ) ) {
		cs::i::pred->update (
			cs::i::client_state->delta_tick ( ),
			cs::i::client_state->delta_tick ( ) > 0,
			cs::i::client_state->last_command_ack ( ),
			cs::i::client_state->last_outgoing_cmd ( ) + cs::i::client_state->choked ( )
		);
	}

	ducking = !!(ucmd->m_buttons & buttons_t::duck);
	in_cm = true;

	utils::update_key_toggles( );

	/* recharge if we need, and return */
	if ( exploits::recharge ( ucmd ) )
		return false;

	//RUN_SAFE (
	//	"features::ragebot::get_weapon_config",
	features::ragebot::get_weapon_config( features::ragebot::active_config );
	//);

	if ( !g::local || !g::local->alive( ) ) {
		g::cock_ticks = 0;
	}

	security_handler::update( );

	if ( g::local && g::local->weapon( ) ) {
		const auto weapon = g::local->weapon( );
		weapon->update_accuracy( );
		features::spread_circle::total_spread = weapon->inaccuracy( ) + weapon->spread( );
	}
	else {
		features::spread_circle::total_spread = 0.0f;
	}

	//RUN_SAFE (
	//	"features::esp::handle_dynamic_updates",
	features::esp::handle_dynamic_updates( );
	//);

	g::ucmd = ucmd;

	auto old_angs = ucmd->m_angs;

	//RUN_SAFE (
	//	"features::clantag::run",
	features::clantag::run( ucmd );
	//);

	//RUN_SAFE (
	//	"features::movement::run",
	features::movement::run( ucmd, old_angs );

	features::blockbot::run( ucmd, old_angs );
	//);

	auto old_smove = ucmd->m_smove;
	auto old_fmove = ucmd->m_fmove;

	//RUN_SAFE (
	//	"csgo::for_each_player",
	cs::for_each_player( [ ] ( player_t* pl ) {
		static auto reloading_offset = pattern::search( _( "client.dll" ), _( "C6 87 ? ? ? ? ? 8B 06 8B CE FF 90" ) ).add( 2 ).deref( ).get < uint32_t >( );

		features::esp::esp_data [ pl->idx( ) ].m_fakeducking = pl->crouch_speed( ) == 8.0f && pl->crouch_amount( ) > 0.1f && pl->crouch_amount( ) < 0.9f;
		features::esp::esp_data [ pl->idx( ) ].m_reloading = pl->weapon( ) ? *reinterpret_cast< bool* >( uintptr_t( pl->weapon( ) ) + reloading_offset ) : false;

		if ( g::local && g::local->weapon( ) && g::local->weapon( )->data( ) ) {
			auto dmg_out = static_cast< float >( g::local->weapon( )->data( )->m_dmg );
			autowall::scale_dmg( pl, g::local->weapon( )->data( ), 3, dmg_out );
			features::esp::esp_data [ pl->idx( ) ].m_fatal = static_cast< int > ( dmg_out ) >= pl->health( );
		}
		else {
			features::esp::esp_data [ pl->idx( ) ].m_fatal = false;
		}
		} );
	//);

	//RUN_SAFE (
	//	"features::nade_prediction::trace",
	features::nade_prediction::trace( ucmd );
	//);

	//RUN_SAFE (
		//	"animations::resolver::update",
	//animations::resolver::update( ucmd );
	//);

	//RUN_SAFE (
	//	"features::prediction::run",
	features::prediction::run( [ & ] ( ) {
		cs::for_each_player( [ ] ( player_t* pl ) {
			//RUN_SAFE (
			//	"features::lagcomp::pop",
			features::lagcomp::pop( pl );
			//);
			} );

		features::antiaim::simulate_lby( );
		ducking = !!(ucmd->m_buttons & buttons_t::duck);

		features::legitbot::run( ucmd );

		//RUN_SAFE (
		//	"features::ragebot::run",
			features::ragebot::run( ucmd, old_smove, old_fmove, old_angs );
		//);

		//RUN_SAFE (
		//	"features::antiaim::run",
		features::antiaim::run( ucmd, old_smove, old_fmove );

		features::autopeek::run ( ucmd, old_smove, old_fmove, old_angs );
		} );
	//);

	if( !!(ucmd->m_buttons & buttons_t::attack) )
		exploits::has_shifted = false;

	fix_event_delay( ucmd );

	if ( g::local && g::local->weapon( ) && g::local->weapon( )->data( ) && features::ragebot::active_config.auto_revolver && g::local->weapon( )->item_definition_index( ) == weapons_t::revolver && !( ucmd->m_buttons & buttons_t::attack ) ) {
		if ( cs::time2ticks( features::prediction::curtime() ) > g::cock_ticks ) {
			ucmd->m_buttons &= ~buttons_t::attack;
			g::cock_ticks = cs::time2ticks( features::prediction::curtime ( ) + 0.25f ) - 1;
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

	*( bool* )( *( uintptr_t* )( uintptr_t( _AddressOfReturnAddress( ) ) - 4 ) - 28 ) = g::send_packet;

	/* fix anti-aim slide */ {
		if ( ucmd->m_fmove ) {
			ucmd->m_buttons &= ~( ucmd->m_fmove < 0.0f ? buttons_t::forward : buttons_t::back );
			ucmd->m_buttons |= ( ucmd->m_fmove > 0.0f ? buttons_t::forward : buttons_t::back );
		}

		if ( ucmd->m_smove ) {
			ucmd->m_buttons &= ~( ucmd->m_smove < 0.0f ? buttons_t::right : buttons_t::left );
			ucmd->m_buttons |= ( ucmd->m_smove > 0.0f ? buttons_t::right : buttons_t::left );
		}

		/* slide to opposite side (anti-toeaim) */
		/*
		if ( ucmd->m_fmove ) {
			ucmd->m_buttons &= ~( ucmd->m_fmove < 0.0f ? 16 : 8 );
			ucmd->m_buttons |= ( ucmd->m_fmove > 0.0f ? 16 : 8 );
		}

		if ( ucmd->m_smove ) {
			ucmd->m_buttons &= ~( ucmd->m_smove < 0.0f ? 512 : 1024 );
			ucmd->m_buttons |= ( ucmd->m_smove > 0.0f ? 512 : 1024 );
		}
		*/
	}

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

	exploits::run ( ucmd );

	return false;
}