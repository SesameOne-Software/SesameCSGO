﻿#include <ShlObj.h>
#include <memory>

#include "hooks.hpp"
#include "../security/security_handler.hpp"
#include "../menu/menu.hpp"
#include "../minhook/minhook.h"

#include "../renderer/render.hpp"

/* all hooks */
#include "create_move.hpp"
#include "cs_blood_spray_callback.hpp"
#include "do_extra_bone_processing.hpp"
#include "draw_model_execute.hpp"
#include "emit_sound.hpp"
#include "end_scene.hpp"
#include "frame_stage_notify.hpp"
#include "get_bool.hpp"
#include "get_eye_angles.hpp"
#include "get_viewmodel_fov.hpp"
#include "in_prediction.hpp"
#include "is_hltv.hpp"
#include "list_leaves_in_box.hpp"
#include "lock_cursor.hpp"
#include "modify_eye_pos.hpp"
#include "override_view.hpp"
#include "paint_traverse.hpp"
#include "reset.hpp"
#include "scene_end.hpp"
#include "send_datagram.hpp"
#include "should_skip_anim_frame.hpp"
#include "write_usercmd_delta_to_buffer.hpp"
#include "setup_bones.hpp"
#include "run_simulation.hpp"
#include "build_transformations.hpp"
#include "base_interpolate_part1.hpp"
#include "temp_entities.hpp"
#include "update_clientside_animations.hpp"
#include "netmsg_tick.hpp"
#include "process_interp_list.hpp"
#include "run_command.hpp"
#include "accumulate_layers.hpp"
#include "notify_on_layer_change_cycle.hpp"
#include "notify_on_layer_change_weight.hpp"
#include "is_connected.hpp"
#include "perform_flashbang_effect.hpp"
#include "prediction_error_handler.hpp"
#include "draw_cube_overlay.hpp"
#include "adjust_interp_amount.hpp"
#include "calc_view.hpp"
#include "post_network_data_received.hpp"
#include "packet_start.hpp"
#include "get_client_interp_amount.hpp"
#include "svc_msg_voice_data.hpp"
#include "get_client_model_renderable.hpp"

#include "events.hpp"
#include "wnd_proc.hpp"

/* features */
#include "../features/esp.hpp"
#include "../features/skinchanger.hpp"

#include "../menu/menu.hpp"

#include "../menu/options.hpp"

#include "ent_listener.hpp"

/* resources */

std::unique_ptr< c_entity_listener_mgr > ent_listener;

#pragma optimize( "2", off )

void hooks::init( ) {
	//VMP_BEGINULTRA ( );
	g::resources::init ( );
	/* initialize cheat config */
	gui::init( );

	//const auto cl_extrapolate = cs::i::cvar->find (_("cl_extrapolate") );
	//cl_extrapolate->no_callback ( );
	//cl_extrapolate->set_value ( 0 );

	const auto r_jiggle_bones = cs::i::cvar->find ( _ ( "r_jiggle_bones" ) );
	r_jiggle_bones->no_callback ( );
	r_jiggle_bones->set_value ( 0 );

	cs::i::engine->client_cmd_unrestricted ( _ ( "developer 1" ) );
	cs::i::engine->client_cmd_unrestricted ( _ ( "con_filter_enable 2" ) );

	features::skinchanger::init ( );

	/* load default config */
	//menu::load_default( );

	old::wnd_proc = ( WNDPROC ) LI_FN ( SetWindowLongA )( LI_FN ( FindWindowA )( _ ( "Valve001" ), nullptr ), GWLP_WNDPROC, LONG_PTR ( wnd_proc ) );

	/* remove max processing ticks clamp */
	const auto clsm_numUsrCmdProcessTicksMax_clamp = pattern::search( _( "engine.dll" ), _( "0F 4F F0 89 5D FC" ) ).get< void* >( );
	//
	unsigned long old_prot = N( 0 );
	LI_FN ( VirtualProtect )( clsm_numUsrCmdProcessTicksMax_clamp, N ( 3 ), N( PAGE_EXECUTE_READWRITE ), &old_prot );
	memset ( clsm_numUsrCmdProcessTicksMax_clamp, N( 0x90 ), N ( 3 ) );
	LI_FN ( VirtualProtect )( clsm_numUsrCmdProcessTicksMax_clamp, N ( 3 ), old_prot, &old_prot );

	/* remove breakpoints */
	const auto client_bp = pattern::search ( _ ( "client.dll" ), _ ( "CC F3 0F 10 4D 18" ) ).get< void* > ( );

	old_prot = N ( 0 );
	LI_FN ( VirtualProtect )( client_bp, N ( 1 ), N ( PAGE_EXECUTE_READWRITE ), &old_prot );
	memset ( client_bp, N ( 0x90 ), N ( 1 ) );
	LI_FN ( VirtualProtect )( client_bp, N ( 1 ), old_prot, &old_prot );

	const auto server_bp = pattern::search ( _ ( "server.dll" ), _ ( "CC F3 0F 10 4D 18" ) ).get< void* > ( );

	old_prot = N ( 0 );
	LI_FN ( VirtualProtect )( server_bp, N ( 1 ), N ( PAGE_EXECUTE_READWRITE ), &old_prot );
	memset ( server_bp, N ( 0x90 ), N ( 1 ) );
	LI_FN ( VirtualProtect )( server_bp, N ( 1 ), old_prot, &old_prot );

	const auto engine_bp = pattern::search ( _ ( "engine.dll" ), _ ( "CC FF 15 ? ? ? ? 8B D0 BB" ) ).get< void* > ( );

	old_prot = N ( 0 );
	LI_FN ( VirtualProtect )( engine_bp, N ( 1 ), N ( PAGE_EXECUTE_READWRITE ), &old_prot );
	memset ( engine_bp, N ( 0x90 ), N ( 1 ) );
	LI_FN ( VirtualProtect )( engine_bp, N ( 1 ), old_prot, &old_prot );

	/* hook functions */
	const auto _create_move = pattern::search( _( "client.dll" ), _( "55 8B EC 8B 0D ? ? ? ? 85 C9 75 06" ) ).get< void* >( );
	const auto _frame_stage_notify = pattern::search( _( "client.dll" ), _( "55 8B EC 8B 0D ? ? ? ? 8B 01 8B 80 74 01 00 00 FF D0 A2" ) ).get< void* >( );
	const auto _end_scene = vfunc< void* >( cs::i::dev, N( 42 ) );
	const auto _reset = vfunc< void* >( cs::i::dev, N( 16 ) );
	const auto _lock_cursor = pattern::search( _( "vguimatsurface.dll" ), _( "80 3D ? ? ? ? 00 8B 91 A4 02 00 00 8B 0D ? ? ? ? C6 05 ? ? ? ? 01" ) ).get< void* >( );
	const auto _scene_end = vfunc< void* >( cs::i::render_view, N( 9 ) );
	const auto _draw_model_execute = vfunc< void* >( cs::i::mdl_render, N( 21 ) );
	const auto _do_extra_bone_processing = pattern::search( _( "client.dll" ), _( "55 8B EC 83 E4 F8 81 EC FC 00 00 00 53 56 8B F1 57" ) ).get< void* >( );
	const auto _get_eye_angles = pattern::search( _( "client.dll" ), _( "56 8B F1 85 F6 74 32" ) ).get< void* >( );
	const auto _get_int = pattern::search( _( "client.dll" ), _( "8B 51 1C 3B D1 75 06" ) ).get< void* >( );
	const auto _override_view = pattern::search( _( "client.dll" ), _( "55 8B EC 83 E4 F8 83 EC 58 56 57 8B 3D" ) ).get< void* >( );
	const auto _send_datagram = pattern::search( _( "engine.dll" ), _( "55 8B EC 83 E4 F0 B8 ? ? ? ? E8 ? ? ? ? 56 57 8B F9 89 7C 24 18" ) ).get<void*>( );
	const auto _should_skip_anim_frame = pattern::search( _( "client.dll" ), _( "57 8B F9 8B 07 8B 80 ? ? ? ? FF D0 84 C0 75 02" ) ).get< void* >( );
	const auto _is_hltv = vfunc< void* >( cs::i::engine, N( 93 ) );
	const auto _write_usercmd_delta_to_buffer = vfunc< void* >( cs::i::client, N( 24 ) );
	const auto _list_leaves_in_box = vfunc< void* >( cs::i::engine->get_bsp_tree_query( ), N( 6 ) );
	const auto _paint_traverse = vfunc< void* >( cs::i::panel, N( 41 ) );
	const auto _get_viewmodel_fov = vfunc< void* >( **( void*** )( ( *( uintptr_t** )cs::i::client ) [ 10 ] + 5 ), N( 35 ) );
	const auto _in_prediction = vfunc< void* >( cs::i::pred, N( 14 ) );	
	const auto _emit_sound = pattern::search( _( "engine.dll" ), _( "E8 ? ? ? ? 5B 8B E5 5D C2 44 00" ) ).resolve_rip( ).get<void*>( );
	const auto _cs_blood_spray_callback = pattern::search( _( "client.dll" ), _( "55 8B EC 8B 4D 08 F3 0F 10 51 ? 8D 51 18" ) ).get<void*>( );
	const auto _modify_eye_pos = pattern::search( _( "client.dll" ), _( "E8 ? ? ? ? 8B 06 8B CE FF 90 ? ? ? ? 85 C0 74 4E" ) ).resolve_rip().get<void*>( );
	const auto _setup_bones = pattern::search( _( "client.dll" ), _( "55 8B EC 83 E4 F0 B8 ? ? ? ? E8 ? ? ? ? 56 57 8B F9" ) ).get< void* >( );
	const auto _run_command = vfunc<void*>( cs::i::pred, 19 );
	const auto _run_simulation = pattern::search( _( "client.dll" ), _( "E8 ? ? ? ? A1 ? ? ? ? F3 0F 10 45 ? F3 0F 11 40" ) ).resolve_rip().get< void* >( );
	const auto _build_transformations = pattern::search ( _ ( "client.dll" ), _ ( "55 8B EC 83 E4 F0 81 EC ? ? ? ? 56 57 8B F9 8B 0D ? ? ? ? 89 7C 24 1C" ) ).get< void* > ( );
	const auto _base_interpolate_part1 = pattern::search ( _ ( "client.dll" ), _ ( "55 8B EC 51 8B 45 14 56" ) ).get< void* > ( );
	const auto _temp_entities = pattern::search ( _ ( "engine.dll" ), _ ( "55 8B EC 83 E4 F8 83 EC 4C A1 ? ? ? ? 80 B8" ) ).get< void* > ( );
	const auto _update_clientside_animations = pattern::search ( _ ( "client.dll" ), _ ( "E8 ? ? ? ? 8B 0D ? ? ? ? 8B 01 FF 50 10" ) ).resolve_rip ( ).get< void* > ( );
	const auto _netmsg_tick = pattern::search ( _ ( "engine.dll" ), _ ( "55 8B EC 53 56 8B F1 8B 0D ? ? ? ? 57" ) ).get< void* > ( );
	const auto _process_interp_list = pattern::search( _( "client.dll" ) , _( "53 0F B7 1D ? ? ? ? 56" ) ).get< void* >( );
	const auto _accumulate_layers = pattern::search( _( "client.dll" ) , _( "55 8B EC 57 8B F9 8B 0D ? ? ? ? 8B 01 8B" ) ).get< void* >( );
	const auto _notify_on_layer_change_cycle = pattern::search( _( "client.dll" ) , _( "F3 0F 11 86 98 00 00 00 5E 5D C2 08 00" ) ).sub( 57 ).get< void* >( );
	const auto _notify_on_layer_change_weight = pattern::search( _( "client.dll" ) , _( "F3 0F 11 86 9C 00 00 00 5E 5D C2 08 00" ) ).sub( 57 ).get< void* >( );
	const auto _is_connected = vfunc<void*>( cs::i::engine , N( 27 ) );
	const auto _perform_flashbang_effect = pattern::search ( _ ( "client.dll" ), _ ( "E8 ? ? ? ? F3 0F 10 05 ? ? ? ? 8B 0D" ) ).resolve_rip().get< void* > ( );
	const auto _prediction_error_handler = pattern::search ( _ ( "client.dll" ), _ ( "55 8B EC 8B 45 10 53 56 8B F1 57" ) ).get< void* > ( );
	const auto _draw_cube_overlay = pattern::search ( _ ( "engine.dll" ), _ ( "55 8B EC F3 0F 10 45 28 8B 55 0C" ) ).get< void* > ( );
	//const auto _adjust_interp_amount = pattern::search ( _ ( "client.dll" ), _ ( "55 8B EC 83 EC 08 56 8B F1 F3 0F 11 4D" ) ).get< void* > ( );
	const auto _calc_view = pattern::search ( _ ( "client.dll" ), _ ( "55 8B EC 53 8B 5D 08 56 57 FF 75 18 8B F1" ) ).get< void* > ( );
	const auto _post_network_data_received = pattern::search ( _ ( "client.dll" ), _ ( "E8 ? ? ? ? 33 F6 6A 02" ) ).resolve_rip ( ).get< void* > ( );
	const auto _packet_start = pattern::search ( _ ( "engine.dll" ), _ ( "56 8B F1 E8 ? ? ? ? 8B 8E ? ? ? ? 3B" ) ).sub ( 32 ).get< void* > ( );
	const auto _get_client_interp_amount = pattern::search ( _ ( "client.dll" ), _ ( "E8 ? ? ? ? F3 0F 58 44 24" ) ).resolve_rip ( ).get< void* > ( );
	const auto _svc_msg_voice_data = pattern::search ( _ ( "engine.dll" ), _ ( "55 8B EC 83 E4 F8 A1 ? ? ? ? 81 EC ? ? ? ? 53 56 8B F1 B9 ? ? ? ? 57 FF 50 34 8B 7D 08 85 C0 74 13 8B 47 08" ) ).get< void* > ( );
	const auto _get_client_model_renderable = pattern::search ( _ ( "client.dll" ), _ ( "56 8B F1 80 BE ? ? ? ? ? 0F 84 ? ? ? ? 80 BE ? ? ? ? ? 0F 85 ? ? ? ? 8B 0D" ) ).get< void* > ( );

	//dbg_print ( _ ( "EndScene: 0x%X" ), _end_scene );
	//dbg_print ( _ ( "Reset: 0x%X" ), _reset );

	MH_Initialize( );

#define to_string( func ) #func
#define dbg_hook( a, b, c ) print_and_hook ( a, b, c, _( to_string ( b ) ) )
	auto print_and_hook = [ ] ( void* from, void* to, void** original, const char* func_name ) {
		if ( !from )
			return dbg_print( _( "Invalid target function: %s\n" ), func_name );

		//MessageBoxA( nullptr, func_name, func_name, 0 );

		if ( MH_CreateHook( from, to, original ) != MH_OK )
			return dbg_print( _( "Hook creation failed: %s\n" ), func_name );

		if ( MH_EnableHook( from ) != MH_OK )
			return dbg_print( _( "Hook enabling failed: %s\n" ), func_name );
		// dbg_print ( _ ( "Hooked: %s\n" ), func_name );
	};

	/*
	* void CIronSightController::RenderScopeEffect( int x, int y, int w, int h, CViewSetup *pViewSetup )
	* 55 8B EC 81 EC ? ? ? ? 53 56 57 8B D9 E8
	* Remove scope for ironsight weapons
	*/

	dbg_hook( _paint_traverse, paint_traverse, ( void** )&old::paint_traverse );
	dbg_hook( _create_move, create_move, ( void** )&old::create_move );
	dbg_hook( _frame_stage_notify, frame_stage_notify, ( void** )&old::frame_stage_notify );
	dbg_hook( _end_scene, end_scene, ( void** )&old::end_scene );
	dbg_hook( _reset, reset, ( void** )&old::reset );
	dbg_hook( _lock_cursor, lock_cursor, ( void** )&old::lock_cursor );
	dbg_hook( _scene_end, scene_end, ( void** )&old::scene_end );
	dbg_hook( _draw_model_execute, draw_model_execute, ( void** )&old::draw_model_execute );
	dbg_hook( _do_extra_bone_processing, do_extra_bone_processing, ( void** )&old::do_extra_bone_processing );
	dbg_hook( _get_eye_angles, get_eye_angles, ( void** )&old::get_eye_angles );
	dbg_hook( _get_int, get_int, ( void** )&old::get_int );
	dbg_hook( _override_view, override_view, ( void** )&old::override_view );
	dbg_hook( _is_hltv, is_hltv, ( void** )&old::is_hltv );
	//dbg_hook( _write_usercmd_delta_to_buffer, write_usercmd_delta_to_buffer, ( void** )&old::write_usercmd_delta_to_buffer );
	dbg_hook( _list_leaves_in_box, list_leaves_in_box, ( void** )&old::list_leaves_in_box );
	dbg_hook( _get_viewmodel_fov, get_viewmodel_fov, ( void** )&old::get_viewmodel_fov );
	dbg_hook( _in_prediction, in_prediction, ( void** )&old::in_prediction );
	dbg_hook( _send_datagram, send_datagram, ( void** )&old::send_datagram );
	dbg_hook( _should_skip_anim_frame, should_skip_anim_frame, ( void** )&old::should_skip_anim_frame );
	//dbg_hook( _emit_sound, emit_sound, ( void** )&old::emit_sound );
	//dbg_hook( _cs_blood_spray_callback, cs_blood_spray_callback, ( void** )&old::cs_blood_spray_callback );
	//dbg_hook( _modify_eye_pos, modify_eye_pos, ( void** )&old::modify_eye_pos );
	dbg_hook( _setup_bones, setup_bones, ( void** )&old::setup_bones );
	dbg_hook( _run_simulation, run_simulation, ( void** )&old::run_simulation );
	dbg_hook( _build_transformations, build_transformations, ( void** )&old::build_transformations );
	dbg_hook ( _base_interpolate_part1, base_interpolate_part1, ( void** ) &old::base_interpolate_part1 );
	//dbg_hook ( _temp_entities, temp_entities, ( void** ) &old::temp_entities );
	dbg_hook ( _update_clientside_animations, update_clientside_animations, ( void** ) &old::update_clientside_animations );
	//dbg_hook( _netmsg_tick , netmsg_tick , ( void** ) &old::netmsg_tick );
	dbg_hook( _process_interp_list , process_interp_list , ( void** ) &old::process_interp_list );
	//dbg_hook( _run_command , run_command , ( void** ) &old::run_command );
	dbg_hook( _accumulate_layers , accumulate_layers , ( void** ) &old::accumulate_layers );
	dbg_hook( _notify_on_layer_change_cycle , notify_on_layer_change_cycle , ( void** ) &old::notify_on_layer_change_cycle );
	dbg_hook( _notify_on_layer_change_weight , notify_on_layer_change_weight , ( void** ) &old::notify_on_layer_change_weight );
	//dbg_hook ( _is_connected, is_connected, ( void** ) &old::is_connected );
	dbg_hook ( _perform_flashbang_effect, perform_flashbang_effect, ( void** ) &old::perform_flashbang_effect );
	//dbg_hook ( _prediction_error_handler, prediction_error_handler, ( void** ) &old::prediction_error_handler );
	//dbg_hook ( _draw_cube_overlay, draw_cube_overlay, ( void** ) &old::draw_cube_overlay );
	//dbg_hook ( _adjust_interp_amount, adjust_interp_amount, ( void** ) &old::adjust_interp_amount );
	dbg_hook ( _calc_view, calc_view, ( void** ) &old::calc_view );
	dbg_hook ( _post_network_data_received, post_network_data_received, ( void** ) &old::post_network_data_received );
	//dbg_hook ( _packet_start, packet_start, ( void** ) &old::packet_start );
	//dbg_hook ( _get_client_interp_amount, get_client_interp_amount, ( void** ) &old::get_client_interp_amount );
	//dbg_hook ( _svc_msg_voice_data, svc_msg_voice_data, ( void** ) &old::svc_msg_voice_data );
	dbg_hook ( _get_client_model_renderable, get_client_model_renderable, ( void** ) &old::get_client_model_renderable );

	event_handler = std::make_unique< c_event_handler > ( );

	ent_listener = std::make_unique< c_entity_listener_mgr > ( );
	ent_listener->add ( );

	//cs::i::engine->client_cmd_unrestricted (_( "clear") );
	//dbg_print ( _("Success.\n" ));
	//VMP_END ( );
	END_FUNC;
}

#pragma optimize( "2", on )