#include <ShlObj.h>
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
#include "send_net_msg.hpp"
#include "should_skip_anim_frame.hpp"
#include "write_usercmd_delta_to_buffer.hpp"
#include "setup_bones.hpp"
#include "run_command.hpp"
#include "run_simulation.hpp"
#include "build_transformations.hpp"
#include "base_interpolate_part1.hpp"
#include "cl_fireevents.hpp"

#include "events.hpp"
#include "wnd_proc.hpp"

/* features */
#include "../features/esp.hpp"

#include "../menu/menu.hpp"

#include "../menu/options.hpp"

#include "ent_listener.hpp"

/* resources */

std::unique_ptr< c_entity_listener_mgr > ent_listener;

void hooks::init( ) {
	g::resources::init ( );

	/* initialize cheat config */
	gui::init( );
	erase::erase_func( gui::init );

	/* create fonts */
	const uint16_t custom_font_ranges [ ] = {
		0x0020, 0x00FF, // Basic Latin + Latin Supplement
		0x2000, 0x206F, // General Punctuation
		0x3000, 0x30FF, // CJK Symbols and Punctuations, Hiragana, Katakana
		0x31F0, 0x31FF, // Katakana Phonetic Extensions
		0xFF00, 0xFFEF, // Half-width characters
		0x4e00, 0x9FAF, // CJK Ideograms
		0x3131, 0x3163, // Korean alphabets
		0xAC00, 0xD7A3, // Korean characters
		0x0400, 0x052F, // Cyrillic + Cyrillic Supplement
		0x2DE0, 0x2DFF, // Cyrillic Extended-A
		0xA640, 0xA69F, // Cyrillic Extended-B
		0x2010, 0x205E, // Punctuations
		0x0E00, 0x0E7F, // Thai
		// Vietnamese
		0x0102, 0x0103,
		0x0110, 0x0111,
		0x0128, 0x0129,
		0x0168, 0x0169,
		0x01A0, 0x01A1,
		0x01AF, 0x01B0,
		0x1EA0, 0x1EF9,
		0
	};

	render::create_font ( g::resources::sesame_ui, g::resources::sesame_ui_size, _ ( "dbg_font" ), 16.0f );
	render::create_font ( g::resources::sesame_ui, g::resources::sesame_ui_size, _ ( "esp_font" ), 16.0f, custom_font_ranges );
	render::create_font ( g::resources::sesame_ui, g::resources::sesame_ui_size, _ ( "indicator_font" ), 32.0f );
	render::create_font ( g::resources::sesame_ui, g::resources::sesame_ui_size, _ ( "watermark_font" ), 16.0f );

	/* load default config */
	//menu::load_default( );

	const auto clsm_numUsrCmdProcessTicksMax_clamp = pattern::search( _( "engine.dll" ), _( "0F 4F F0 89 5D FC" ) ).get< void* >( );

	if ( clsm_numUsrCmdProcessTicksMax_clamp ) {
		unsigned long old_prot = 0;
		LI_FN( VirtualProtect )( clsm_numUsrCmdProcessTicksMax_clamp, 3, PAGE_EXECUTE_READWRITE, &old_prot );
		memset( clsm_numUsrCmdProcessTicksMax_clamp, 0x90, 3 );
		LI_FN( VirtualProtect )( clsm_numUsrCmdProcessTicksMax_clamp, 3, old_prot, &old_prot );
	}

	const auto _create_move = pattern::search( _( "client.dll" ), _( "55 8B EC 8B 0D ? ? ? ? 85 C9 75 06 B0" ) ).get< void* >( );
	const auto _frame_stage_notify = pattern::search( _( "client.dll" ), _( "55 8B EC 8B 0D ? ? ? ? 8B 01 8B 80 74 01 00 00 FF D0 A2" ) ).get< void* >( );
	const auto _end_scene = vfunc< void* >( csgo::i::dev, N( 42 ) );
	const auto _reset = vfunc< void* >( csgo::i::dev, N( 16 ) );
	const auto _lock_cursor = pattern::search( _( "vguimatsurface.dll" ), _( "80 3D ? ? ? ? 00 8B 91 A4 02 00 00 8B 0D ? ? ? ? C6 05 ? ? ? ? 01" ) ).get< void* >( );
	const auto _scene_end = vfunc< void* >( csgo::i::render_view, N( 9 ) );
	const auto _draw_model_execute = vfunc< void* >( csgo::i::mdl_render, N( 21 ) );
	const auto _do_extra_bone_processing = pattern::search( _( "client.dll" ), _( "55 8B EC 83 E4 F8 81 EC FC 00 00 00 53 56 8B F1 57" ) ).get< void* >( );
	const auto _get_eye_angles = pattern::search( _( "client.dll" ), _( "56 8B F1 85 F6 74 32" ) ).get< void* >( );
	const auto _get_int = pattern::search( _( "client.dll" ), _( "8B 51 1C 3B D1 75 06" ) ).get< void* >( );
	const auto _override_view = pattern::search( _( "client.dll" ), _( "55 8B EC 83 E4 F8 83 EC 58 56 57 8B 3D ? ? ? ? 85 FF" ) ).get< void* >( );
	const auto _send_datagram = pattern::search( _( "engine.dll" ), _( "55 8B EC 83 E4 F0 B8 ? ? ? ? E8 ? ? ? ? 56 57 8B F9 89 7C 24 18" ) ).get<void*>( );
	const auto _should_skip_anim_frame = pattern::search( _( "client.dll" ), _( "E8 ? ? ? ? 88 44 24 0B" ) ).resolve_rip( ).get< void* >( );
	const auto _is_hltv = vfunc< void* >( csgo::i::engine, N( 93 ) );
	const auto _write_usercmd_delta_to_buffer = vfunc< void* >( csgo::i::client, N( 24 ) );
	const auto _list_leaves_in_box = vfunc< void* >( csgo::i::engine->get_bsp_tree_query( ), N( 6 ) );
	const auto _paint_traverse = vfunc< void* >( csgo::i::panel, N( 41 ) );
	const auto _get_viewmodel_fov = vfunc< void* >( **( void*** )( ( *( uintptr_t** )csgo::i::client ) [ 10 ] + 5 ), N( 35 ) );
	const auto _in_prediction = vfunc< void* >( csgo::i::pred, N( 14 ) );
	const auto _send_net_msg = pattern::search( _( "engine.dll" ), _( "55 8B EC 83 EC 08 56 8B F1 8B 86 ? ? ? ? 85 C0" ) ).get<void*>( );
	const auto _emit_sound = pattern::search( _( "engine.dll" ), _( "E8 ? ? ? ? 8B E5 5D C2 3C 00 55" ) ).resolve_rip( ).get<void*>( );
	const auto _cs_blood_spray_callback = pattern::search( _( "client.dll" ), _( "55 8B EC 8B 4D 08 F3 0F 10 51 ? 8D 51 18" ) ).get<void*>( );
	const auto _modify_eye_pos = pattern::search( _( "client.dll" ), _( "57 E8 ? ? ? ? 8B 06 8B CE FF 90" ) ).add( 1 ).resolve_rip( ).get<void*>( );
	const auto _setup_bones = pattern::search( _( "client.dll" ), _( "55 8B EC 83 E4 F0 B8 ? ? ? ? E8 ? ? ? ? 56 57 8B F9" ) ).get< void* >( );
	const auto _run_command = vfunc<void*>( csgo::i::pred, 19 );
	const auto _run_simulation = pattern::search( _( "client.dll" ), _( "55 8B EC 83 EC 08 53 8B 5D 10 56" ) ).get< void* >( );
	const auto _build_transformations = pattern::search ( _ ( "client.dll" ), _ ( "55 8B EC 83 E4 F0 81 EC ? ? ? ? 56 57 8B F9 8B 0D ? ? ? ? 89 7C 24 1C" ) ).get< void* > ( );
	const auto _base_interpolate_part1 = pattern::search ( _ ( "client.dll" ), _ ( "55 8B EC 51 8B 45 14 56" ) ).get< void* > ( );
	const auto _cl_fireevents = pattern::search ( _ ( "engine.dll" ), _ ( "E8 ? ? ? ? 84 DB 0F 84 ? ? ? ? 8B 0D" ) ).resolve_rip().get< void* > ( );

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
	dbg_hook( _write_usercmd_delta_to_buffer, write_usercmd_delta_to_buffer, ( void** )&old::write_usercmd_delta_to_buffer );
	dbg_hook( _list_leaves_in_box, list_leaves_in_box, ( void** )&old::list_leaves_in_box );
	dbg_hook( _get_viewmodel_fov, get_viewmodel_fov, ( void** )&old::get_viewmodel_fov );
	dbg_hook( _in_prediction, in_prediction, ( void** )&old::in_prediction );
	dbg_hook( _send_datagram, send_datagram, ( void** )&old::send_datagram );
	dbg_hook( _should_skip_anim_frame, should_skip_anim_frame, ( void** )&old::should_skip_anim_frame );
	dbg_hook( _send_net_msg, send_net_msg, ( void** )&old::send_net_msg );
	dbg_hook( _emit_sound, emit_sound, ( void** )&old::emit_sound );
	dbg_hook( _cs_blood_spray_callback, cs_blood_spray_callback, ( void** )&old::cs_blood_spray_callback );
	dbg_hook( _modify_eye_pos, modify_eye_pos, ( void** )&old::modify_eye_pos );
	dbg_hook( _setup_bones, setup_bones, ( void** )&old::setup_bones );
	dbg_hook( _run_command, run_command, ( void** )&old::run_command );
	//dbg_hook( _run_simulation, run_simulation, ( void** )&old::run_simulation );
	dbg_hook( _build_transformations, build_transformations, ( void** )&old::build_transformations );
	dbg_hook ( _base_interpolate_part1, base_interpolate_part1, ( void** ) &old::base_interpolate_part1 );
	dbg_hook ( _cl_fireevents, cl_fireevents, ( void** ) &old::cl_fireevents );

	event_handler = std::make_unique< c_event_handler >( );
	ent_listener = std::make_unique< c_entity_listener_mgr > ( );

	ent_listener->add ( );

	///* load default config for testing */ {
	//	char appdata [ MAX_PATH ];
	//
	//	if ( SUCCEEDED ( LI_FN ( SHGetFolderPathA )( nullptr, N ( 5 ), nullptr, N ( 0 ), appdata ) ) ) {
	//		LI_FN ( CreateDirectoryA )( ( std::string ( appdata ) + _ ( "\\sesame" ) ).data ( ), nullptr );
	//		LI_FN ( CreateDirectoryA )( ( std::string ( appdata ) + _ ( "\\sesame\\configs" ) ).data ( ), nullptr );
	//	}
	//
	//	options::load ( options::vars, std::string ( appdata ) + _ ( "\\sesame\\configs\\hvh max desync.xml" ) );
	//}

	old::wnd_proc = ( WNDPROC )LI_FN( SetWindowLongA )( LI_FN( FindWindowA )( _( "Valve001" ), nullptr ), GWLP_WNDPROC, long( wnd_proc ) );

	END_FUNC
}