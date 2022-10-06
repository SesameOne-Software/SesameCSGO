#include <windows.h>
#include <cstdint>
#include <iostream>
#include <thread>
#include <locale>
#include <codecvt>
#include <fstream>
#include <ShlObj.h>

#include "scripting/js_api.hpp"

#include "cjson/cJSON.h"
#include "utils/utils.hpp"
#include "utils/base64.hpp"
#include "minhook/minhook.h"
#include "sdk/sdk.hpp"
#include "hooks/hooks.hpp"
#include "globals.hpp"
#include "menu/menu.hpp"
#include "utils/networking.hpp"

#include "hooks/wnd_proc.hpp"

/* security */
#include "security/security_handler.hpp"
#include "plusaes.hpp"

#include "../imgui/imgui.h"
#include "../imgui/imgui_impl_dx9.h"
#include "../imgui/imgui_impl_win32.h"
#include "../imgui/imgui_internal.h"

#include "hitsounds.h"

#include "PH/PH_API.hpp"

#include "seh.hpp"

uint64_t anti_patch::g_text_section_hash;
uintptr_t anti_patch::g_text_section, anti_patch::g_text_section_size;
//anti_patch::s_header_data anti_patch::g_header_data;

extern std::string g_username;
extern bool upload_to_cloud;
extern char selected_config [ 128 ];
extern cJSON* cloud_config_list;
extern bool download_config_code;

std::string last_config_user;

#pragma optimize( "2", off )

unsigned __stdcall do_heartbeat( void* data ) {
	VMP_BEGINULTRA ( );

	while ( true ) {
		std::this_thread::sleep_for ( std::chrono::seconds ( ph_heartbeat::PH_SECONDS_INTERVAL ) );

		ph_heartbeat::send_heartbeat ( );
	}

	return 0;
	VMP_END ( );
}

int __stdcall DllMain ( void* loader_data, uint32_t reason, void* reserved );

unsigned __stdcall init_proxy( void* data ) {
	VMP_BEGINULTRA ( );

	/* wait for all modules to load */
	while ( !LI_FN ( GetModuleHandleA )( _ ( "serverbrowser.dll" ) ) )
		std::this_thread::sleep_for ( std::chrono::milliseconds ( N ( 100 ) ) );

	g::is_legacy = *reinterpret_cast< uint32_t* >( reinterpret_cast< uintptr_t >( LI_FN ( GetModuleHandleA )( _ ( "csgo.exe" ) ) ) + N( 0x120 ) ) == N( 0x5A2F1C6A ) /* Tuesday, 12.12.2017 00:01:46 UTC */;

	/* initialize hack */
	cs::init ( );
	netvars::init ( );
	js_api::init ( );
	hooks::init ( );

	//security_handler::store_text_section_hash( uintptr_t( loader_info->hMod ) );

	while ( !g::unload )
		std::this_thread::sleep_for ( std::chrono::seconds ( N ( 1 ) ) );

	std::this_thread::sleep_for( std::chrono::milliseconds( N( 500 ) ) );
	/* destroy imgui resources */
	ImGui_ImplDX9_Shutdown ( );
	ImGui_ImplWin32_Shutdown ( );
	ImGui::DestroyContext ( );

	LI_FN( SetWindowLongA )( LI_FN( FindWindowA )( nullptr, _ ( "Counter-Strike: Global Offensive - Direct3D 9" ) ), GWLP_WNDPROC, long( hooks::old::wnd_proc ) );

	MH_RemoveHook( MH_ALL_HOOKS );
	MH_Uninitialize( );

	std::this_thread::sleep_for( std::chrono::milliseconds( N( 200 ) ) );

	if ( g::local )
		g::local->animate ( ) = true;

	cs::i::input->m_camera_in_thirdperson = false;

	g::cvars::r_aspectratio->set_value ( 1.777777f );
	g::cvars::r_aspectratio->no_callback ( );

	/* reset world color */ {
		static auto load_named_sky = pattern::search( _( "engine.dll" ), _( "55 8B EC 81 EC ? ? ? ? 56 57 8B F9 C7 45" ) ).get< void( __fastcall* )( const char* ) >( );

		load_named_sky( _( "nukeblank" ) );

		for ( auto i = cs::i::mat_sys->first_material ( ); i != cs::i::mat_sys->invalid_material ( ); i = cs::i::mat_sys->next_material ( i ) ) {
			auto mat = cs::i::mat_sys->get_material ( i );

			if ( !mat || mat->is_error_material ( ) )
				continue;

			if ( strstr ( mat->get_texture_group_name ( ), _ ( "StaticProp" ) ) || strstr ( mat->get_texture_group_name ( ), _ ( "World" ) ) ) {
				mat->color_modulate ( 255, 255, 255 );
				mat->alpha_modulate ( 255 );
			}
		}
	}

#ifndef DEV_BUILD
	//mmunload( loader_info->hMod );
#else
	FreeLibraryAndExitThread( HMODULE( data ), 0 );
#endif

	return 0;
	VMP_END ( );
}

__forceinline size_t get_image_size ( void* base ) {
	if ( !base )
		return 0;

	size_t image_size = 0;

	MEMORY_BASIC_INFORMATION memInfo {};
	VirtualQuery ( base, &memInfo, sizeof ( memInfo ) );

	while ( memInfo.BaseAddress && memInfo.AllocationBase == base ) {
		image_size += memInfo.RegionSize;
		VirtualQuery ( ( LPVOID ) ( ( LPBYTE ) base + image_size ), &memInfo, sizeof ( memInfo ) );
	}

	return image_size;
}

inline bool add_dll_handle_to_safe_list ( HMODULE dll ) {
	std::array<uint8_t, 0x1000> fake_pe_header = { 0 };

	void* module_start = reinterpret_cast< void* >( dll );
	const size_t image_size = get_image_size ( module_start );
	void* module_end = reinterpret_cast< void* >( reinterpret_cast< uint8_t* >( dll ) + image_size );

	*( uint32_t* ) ( fake_pe_header.data ( ) + 0x3C ) = 0;
	*( uint16_t* ) ( fake_pe_header.data ( ) + 0x18 ) = 0x10B;
	*( uint16_t* ) ( fake_pe_header.data ( ) + 0x8 ) = 4;
	*( ptrdiff_t* ) ( fake_pe_header.data ( ) + 0x2C ) = ( ptrdiff_t ) ( ( uintptr_t ) module_start - ( uintptr_t ) fake_pe_header.data ( ) );
	*( uint32_t* ) ( fake_pe_header.data ( ) + 0x50 ) = image_size;

	using InitSafeModuleFn = unsigned int ( __fastcall* )( void*, void* );

	if ( const auto func = pattern::search ( _ ( "client.dll" ), _ ( "56 8B 71 3C B8" ) ).get<InitSafeModuleFn> ( ) )
		( *func ) ( fake_pe_header.data ( ), nullptr );
	else
		return false;

	if ( const auto func = pattern::search ( _ ( "engine.dll" ), _ ( "56 8B 71 3C B8" ) ).get<InitSafeModuleFn> ( ) )
		( *func ) ( fake_pe_header.data ( ), nullptr );
	else
		return false;

	const std::vector < std::string > module_list = {
		/*XOR( "antitamper.dll" ),
		XOR( "inputsystem.dll" ),*/
		_ ( "materialsystem.dll" ),
		/*XOR( "server.dll" ),
		XOR( "matchmaking.dll" ),
		XOR( "cairo.dll" ),
		XOR( "datacache.dll" ),
		XOR( "filesystem_stdio.dll" ),
		XOR( "icui18n.dll" ),
		XOR( "icuuc.dll" ),
		XOR( "imemanager.dll" ),
		XOR( "launcher.dll" ),
		XOR( "libavcodec-56.dll" ),
		XOR( "libavformat-56.dll" ),
		XOR( "libavresample-2.dll" ),
		XOR( "libavutil-54.dll" ),
		XOR( "libfontconfig-1.dll" ),
		XOR( "libfreetype-6.dll" ),
		XOR( "libglib-2.0-0.dll" ),
		XOR( "libgmodule-2.0-0.dll" ),
		XOR( "libgobject-2.0-0.dll" ),
		XOR( "libpango-1.0-0.dll" ),
		XOR( "libpangoft2-1.0-0.dll" ),
		XOR( "libswscale-3.dll" ),
		XOR( "localize.dll" ),
		XOR( "mss32.dll" ),
		XOR( "panorama.dll" ),
		XOR( "panorama_text_pango.dll" ),
		XOR( "panoramauiclient.dll" ),
		XOR( "parsifal.dll" ),
		XOR( "phonon.dll" ),
		XOR( "scenefilecache.dll" ),
		XOR( "serverbrowser.dll" ),
		XOR( "shaderapidx9.dll" ),
		XOR( "soundemittersystem.dll" ),
		XOR( "soundsystem.dll" ),
		XOR( "stdshader_dbg.dll" ),
		XOR( "stdshared_dx9.dll" ),
		XOR( "steam_api.dll" ),
		XOR( "steamnetworkingsockets.dll" ),*/
		_ ( "studiorender.dll" ),
		/*XOR( "tier0.dll" ),
		XOR( "v8.dll" ),
		XOR( "v8_libbase.dll" ),
		XOR( "v8_libplatform.dll" ),
		XOR( "valve_avi.dll" ),
		XOR( "vaudio_celt.dll" ),
		XOR( "vaudio_miles.dll" ),
		XOR( "vaudio_speex.dll" ),
		XOR( "vgui2.dll" ),
		XOR( "vguimatsurface.dll" ),
		XOR( "video.dll" ),
		XOR( "vphysics.dll" ),
		XOR( "vscript.dll" ),
		XOR( "vstdlib.dll" ),
		XOR( "vtex_dll.dll" ),
		XOR( "fmod.dll" ),
		XOR( "fmodstudio.dll" )*/
	};

	for ( const auto& mod : module_list ) {
		for ( bool* call_check_list = pattern::search ( mod.c_str ( ), _ ( "8B F0 83 3C 95" ) ).add ( 5 ).deref ( ).get<bool*> ( );
			*call_check_list != 0;
			call_check_list += 20 )
			*call_check_list = false;

		if ( const auto func = pattern::search ( mod.c_str( ), _ ( "53 56 8B 35 ? ? ? ? 8B DA 57" ) ).get<InitSafeModuleFn> ( ) ) {
			( *func ) (
				module_start,
				module_end );
		}
	}

	return true;
}

int __stdcall DllMain( void* loader_data, uint32_t reason, void* reserved ) {
	VMP_BEGINULTRA ( );

	if ( reason == DLL_PROCESS_ATTACH ) {
#ifdef DEV_BUILD
		const auto nt_headers = reinterpret_cast< IMAGE_NT_HEADERS* > ( reinterpret_cast< uintptr_t >( loader_data ) + reinterpret_cast< IMAGE_DOS_HEADER* >( loader_data )->e_lfanew );

		g_ImageStartAddr = reinterpret_cast< void* >( nt_headers->OptionalHeader.ImageBase );
		g_ImageEndAddr = ( uint8_t* ) g_ImageStartAddr + get_image_size ( reinterpret_cast< void* >( nt_headers->OptionalHeader.ImageBase ) );
#else
		const auto hearbeat_info = reinterpret_cast< ph_heartbeat::heartbeat_info* > ( loader_data );

		g_ImageStartAddr = reinterpret_cast< void* >( hearbeat_info->image_base );
		g_ImageEndAddr = ( uint8_t* ) g_ImageStartAddr + get_image_size ( reinterpret_cast< void* >( hearbeat_info->image_base ) );
#endif

		const auto exh = AddVectoredExceptionHandler ( 1, ExceptionHandler );

		if ( !add_dll_handle_to_safe_list ( reinterpret_cast<HMODULE>( loader_data ) ) ) {
			if ( exh )
				RemoveVectoredExceptionHandler ( exh );

			MessageBoxA ( nullptr, _ ( "Failed to add DLL to safe module list." ), _ ( "Error" ), MB_OK );
			ExitProcess ( 1 );

			return FALSE;
		}
	}

	if ( reason == DLL_PROCESS_ATTACH ) {
		_beginthreadex ( nullptr, 0, reinterpret_cast< _beginthreadex_proc_type >( init_proxy ), loader_data, 0, nullptr );

//#ifndef DEV_BUILD
//		ph_heartbeat::initialize_heartbeat ( reinterpret_cast< ph_heartbeat::heartbeat_info* >( loader_data ) );
//		_beginthreadex ( nullptr, 0, reinterpret_cast< _beginthreadex_proc_type >( do_heartbeat ), nullptr, 0, nullptr );
//#endif
	}
		
	return TRUE;

	VMP_END ( );
	END_FUNC;
}

#pragma optimize( "2", on )