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

uint64_t anti_patch::g_text_section_hash;
uintptr_t anti_patch::g_text_section, anti_patch::g_text_section_size;
//anti_patch::s_header_data anti_patch::g_header_data;

extern std::string g_username;
extern bool upload_to_cloud;
extern char selected_config [ 128 ];
extern cJSON* cloud_config_list;
extern bool download_config_code;

std::string last_config_user;

int __stdcall init_proxy( PLoader_Info loader_info ) {
	VM_SHARK_BLACK_START

	/* wait for all modules to load */
	while ( !LI_FN ( GetModuleHandleA )( _ ( "serverbrowser.dll" ) ) )
		std::this_thread::sleep_for ( std::chrono::milliseconds ( N ( 100 ) ) );

	/* initialize hack */
	cs::init ( );
	netvars::init ( );
	js_api::init ( );
	hooks::init ( );
	
	security_handler::store_text_section_hash( uintptr_t( loader_info->hMod ) );

	while ( !g::unload )
		std::this_thread::sleep_for ( std::chrono::seconds ( N ( 1 ) ) );

	std::this_thread::sleep_for( std::chrono::milliseconds( N( 500 ) ) );

	/* destroy imgui resources */
	ImGui_ImplDX9_Shutdown ( );
	ImGui_ImplWin32_Shutdown ( );
	ImGui::DestroyContext ( );

	LI_FN( SetWindowLongA )( LI_FN( FindWindowA )( nullptr, _ ( "Counter-Strike: Global Offensive" ) ), GWLP_WNDPROC, long( hooks::old::wnd_proc ) );

	MH_RemoveHook( MH_ALL_HOOKS );
	MH_Uninitialize( );

	std::this_thread::sleep_for( std::chrono::milliseconds( N( 200 ) ) );

	if ( g::local )
		g::local->animate( ) = true;

	cs::i::input->m_camera_in_thirdperson = false;

	g::cvars::r_aspectratio->set_value ( 1.777777f );
	g::cvars::r_aspectratio->no_callback ( );

	/* reset world color */ {
		static auto load_named_sky = pattern::search( _( "engine.dll" ), _( "55 8B EC 81 EC ? ? ? ? 56 57 8B F9 C7 45" ) ).get< void( __fastcall* )( const char* ) >( );

		load_named_sky( _( "nukeblank" ) );

		for ( auto i = cs::i::mat_sys->first_material( ); i != cs::i::mat_sys->invalid_material( ); i = cs::i::mat_sys->next_material( i ) ) {
			auto mat = cs::i::mat_sys->get_material( i );

			if ( !mat || mat->is_error_material( ) )
				continue;

			if ( strstr( mat->get_texture_group_name( ), _( "StaticProp" ) ) || strstr( mat->get_texture_group_name( ), _( "World" ) ) ) {
				mat->color_modulate( 255, 255, 255 );
				mat->alpha_modulate( 255 );
			}
		}
	}

	VM_SHARK_BLACK_END

#ifndef DEV_BUILD
	mmunload( loader_info->hMod );
#else
	FreeLibraryAndExitThread( HMODULE( loader_info ), 0 );
#endif

	return 0;
}

int __stdcall DllMain( void* loader_data, std::uint32_t reason, void* reserved ) {
#ifndef DEV_BUILD
	g::loader_data = reinterpret_cast< PLoader_Info >( loader_data );
#endif

	if ( reason == 1 )
		_beginthreadex( nullptr, 0, reinterpret_cast< _beginthreadex_proc_type >( init_proxy ), loader_data, 0, nullptr );
		
	return 1;
}
