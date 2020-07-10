#include "hooks.hpp"
#include "../security/security_handler.hpp"
#include "../menu/menu.hpp"
#include "../renderer/d3d9.hpp"
#include "../minhook/minhook.h"

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

#include "events.hpp"
#include "wnd_proc.hpp"

/* features */
#include "../features/esp.hpp"

void hooks::init ( ) {
	unsigned long font_count = 0;
	LI_FN ( AddFontMemResourceEx ) ( sesame_font_data, sizeof sesame_font_data, nullptr, &font_count );

	menu::init ( );
	erase::erase_func ( menu::init );

	/* create fonts */
	render::create_font ( ( void** ) &features::esp::dbg_font, _ ( L"Segoe UI" ), N ( 12 ), false );
	render::create_font ( ( void** ) &features::esp::esp_font, _ ( L"Segoe UI" ), N ( 18 ), false );
	render::create_font ( ( void** ) &features::esp::indicator_font, _ ( L"Segoe UI" ), N ( 14 ), true );
	render::create_font ( ( void** ) &features::esp::watermark_font, _ ( L"Segoe UI" ), N ( 18 ), false );

	/* load default config */
	//menu::load_default( );

	const auto clsm_numUsrCmdProcessTicksMax_clamp = pattern::search ( _ ( "engine.dll" ), _ ( "0F 4F F0 89 5D FC" ) ).get< void* > ( );

	if ( clsm_numUsrCmdProcessTicksMax_clamp ) {
		unsigned long old_prot = 0;
		LI_FN ( VirtualProtect )( clsm_numUsrCmdProcessTicksMax_clamp, 3, PAGE_EXECUTE_READWRITE, &old_prot );
		memset ( clsm_numUsrCmdProcessTicksMax_clamp, 0x90, 3 );
		LI_FN ( VirtualProtect )( clsm_numUsrCmdProcessTicksMax_clamp, 3, old_prot, &old_prot );
	}

	const auto _create_move = pattern::search ( _ ( "client.dll" ), _ ( "55 8B EC 8B 0D ? ? ? ? 85 C9 75 06 B0" ) ).get< void* > ( );
	const auto _frame_stage_notify = pattern::search ( _ ( "client.dll" ), _ ( "55 8B EC 8B 0D ? ? ? ? 8B 01 8B 80 74 01 00 00 FF D0 A2" ) ).get< void* > ( );
	const auto _end_scene = vfunc< void* > ( csgo::i::dev, N ( 42 ) );
	const auto _reset = vfunc< void* > ( csgo::i::dev, N ( 16 ) );
	const auto _lock_cursor = pattern::search ( _ ( "vguimatsurface.dll" ), _ ( "80 3D ? ? ? ? 00 8B 91 A4 02 00 00 8B 0D ? ? ? ? C6 05 ? ? ? ? 01" ) ).get< void* > ( );
	const auto _scene_end = vfunc< void* > ( csgo::i::render_view, N ( 9 ) );
	const auto _draw_model_execute = vfunc< void* > ( csgo::i::mdl_render, N ( 21 ) );
	const auto _do_extra_bone_processing = pattern::search ( _ ( "client.dll" ), _ ( "55 8B EC 83 E4 F8 81 EC FC 00 00 00 53 56 8B F1 57" ) ).get< void* > ( );
	const auto _get_eye_angles = pattern::search ( _ ( "client.dll" ), _ ( "56 8B F1 85 F6 74 32" ) ).get< void* > ( );
	const auto _get_bool = pattern::search ( _ ( "client.dll" ), _ ( "8B 51 1C 3B D1 75 06" ) ).get< void* > ( );
	const auto _override_view = pattern::search ( _ ( "client.dll" ), _ ( "55 8B EC 83 E4 F8 83 EC 58 56 57 8B 3D ? ? ? ? 85 FF" ) ).get< void* > ( );
	const auto _send_datagram = pattern::search ( _ ( "engine.dll" ), _ ( "55 8B EC 83 E4 F0 B8 ? ? ? ? E8 ? ? ? ? 56 57 8B F9 89 7C 24 18" ) ).get<void*> ( );
	const auto _should_skip_anim_frame = pattern::search ( _ ( "client.dll" ), _ ( "E8 ? ? ? ? 88 44 24 0B" ) ).add ( N ( 1 ) ).deref ( ).get< void* > ( );
	const auto _is_hltv = vfunc< void* > ( csgo::i::engine, N ( 93 ) );
	const auto _write_usercmd_delta_to_buffer = vfunc< void* > ( csgo::i::client, N ( 24 ) );
	const auto _list_leaves_in_box = vfunc< void* > ( csgo::i::engine->get_bsp_tree_query ( ), N ( 6 ) );
	const auto _paint_traverse = vfunc< void* > ( csgo::i::panel, N ( 41 ) );
	const auto _get_viewmodel_fov = vfunc< void* > ( **( void*** ) ( ( *( uintptr_t** ) csgo::i::client ) [ 10 ] + 5 ), N ( 35 ) );
	const auto _in_prediction = vfunc< void* > ( csgo::i::pred, N ( 14 ) );
	const auto _send_net_msg = pattern::search ( _ ( "engine.dll" ), _ ( "55 8B EC 83 EC 08 56 8B F1 8B 86 ? ? ? ? 85 C0" ) ).get<void*> ( );
	const auto _emit_sound = pattern::search ( _ ( "engine.dll" ), _ ( "E8 ? ? ? ? 8B E5 5D C2 3C 00 55" ) ).resolve_rip ( ).get<void*> ( );
	const auto _cs_blood_spray_callback = pattern::search ( _ ( "client.dll" ), _ ( "55 8B EC 8B 4D 08 F3 0F 10 51 ? 8D 51 18" ) ).get<void*> ( );
	const auto _modify_eye_pos = pattern::search ( _ ( "client.dll" ), _ ( "57 E8 ? ? ? ? 8B 06 8B CE FF 90" ) ).add ( 1 ).resolve_rip ( ).get<void*> ( );
	const auto _setup_bones = pattern::search ( _ ( "client.dll" ), _ ( "55 8B EC 83 E4 F0 B8 ? ? ? ? E8 ? ? ? ? 56 57 8B F9" ) ).get< void* > ( );

	MH_Initialize ( );

#define to_string( func ) #func
#define dbg_hook( a, b, c ) print_and_hook ( a, b, c, _( to_string ( b ) ) )
	auto print_and_hook = [ ] ( void* from, void* to, void** original, const char* func_name ) {
		MH_CreateHook ( from, to, original );
		MH_EnableHook ( from );
		// dbg_print ( _ ( "Hooked: %s\n" ), func_name );
		//std::this_thread::sleep_for ( std::chrono::seconds ( N ( 1 ) ) );
	};

	dbg_hook ( _paint_traverse, paint_traverse, ( void** ) &old::paint_traverse );
	dbg_hook ( _create_move, create_move, ( void** ) &old::create_move );
	dbg_hook ( _frame_stage_notify, frame_stage_notify, ( void** ) &old::frame_stage_notify );
	dbg_hook ( _end_scene, end_scene, ( void** ) &old::end_scene );
	dbg_hook ( _reset, reset, ( void** ) &old::reset );
	dbg_hook ( _lock_cursor, lock_cursor, ( void** ) &old::lock_cursor );
	dbg_hook ( _scene_end, scene_end, ( void** ) &old::scene_end );
	dbg_hook ( _draw_model_execute, draw_model_execute, ( void** ) &old::draw_model_execute );
	dbg_hook ( _do_extra_bone_processing, do_extra_bone_processing, ( void** ) &old::do_extra_bone_processing );
	dbg_hook ( _get_eye_angles, get_eye_angles, ( void** ) &old::get_eye_angles );
	dbg_hook ( _get_bool, get_bool, ( void** ) &old::get_bool );
	dbg_hook ( _override_view, override_view, ( void** ) &old::override_view );
	dbg_hook ( _is_hltv, is_hltv, ( void** ) &old::is_hltv );
	dbg_hook ( _write_usercmd_delta_to_buffer, write_usercmd_delta_to_buffer, ( void** ) &old::write_usercmd_delta_to_buffer );
	dbg_hook ( _list_leaves_in_box, list_leaves_in_box, ( void** ) &old::list_leaves_in_box );
	dbg_hook ( _get_viewmodel_fov, get_viewmodel_fov, ( void** ) &old::get_viewmodel_fov );
	dbg_hook ( _in_prediction, in_prediction, ( void** ) &old::in_prediction );
	dbg_hook( _send_datagram, send_datagram, ( void** ) &old::send_datagram );
	dbg_hook ( _should_skip_anim_frame, should_skip_anim_frame, ( void** ) &old::should_skip_anim_frame );
	dbg_hook ( _send_net_msg, send_net_msg, ( void** ) &old::send_net_msg );
	dbg_hook ( _emit_sound, emit_sound, ( void** ) &old::emit_sound );
	dbg_hook ( _cs_blood_spray_callback, cs_blood_spray_callback, ( void** ) &old::cs_blood_spray_callback );
	dbg_hook ( _modify_eye_pos, modify_eye_pos, ( void** ) &old::modify_eye_pos );
	dbg_hook ( _setup_bones, setup_bones, ( void** ) &old::setup_bones );

	event_handler = std::make_unique< c_event_handler > ( );

	old::wnd_proc = ( WNDPROC ) LI_FN ( SetWindowLongA )( LI_FN ( FindWindowA )( _ ( "Valve001" ), nullptr ), GWLP_WNDPROC, long ( wnd_proc ) );

	END_FUNC
}