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

	LI_FN( SetWindowLongA )( LI_FN( FindWindowA )( _ ( "Valve001" ), nullptr ), GWLP_WNDPROC, long( hooks::old::wnd_proc ) );

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

int __stdcall DllMain( void* loader_data, uint32_t reason, void* reserved ) {
	VMP_BEGINULTRA ( );

	if ( reason == DLL_PROCESS_ATTACH ) {
		const auto hearbeat_info = reinterpret_cast< ph_heartbeat::heartbeat_info* > ( loader_data );

		g_ImageStartAddr = reinterpret_cast< void* >( hearbeat_info->image_base );
		g_ImageEndAddr = ( uint8_t* ) g_ImageStartAddr + get_image_size ( reinterpret_cast< void* >( hearbeat_info->image_base ) );

		AddVectoredExceptionHandler ( 1, ExceptionHandler );
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