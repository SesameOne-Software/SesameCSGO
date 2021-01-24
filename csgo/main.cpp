#include <windows.h>
#include <cstdint>
#include <iostream>
#include <thread>
#include <locale>
#include <codecvt>
#include <fstream>
#include <ShlObj.h>

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
	hooks::init ( );
	
	security_handler::store_text_section_hash( uintptr_t( loader_info->hMod ) );

	while ( !g::unload ) {
		///* save config to server if requested */
		//if ( upload_to_cloud ) {
		//	char appdata [ MAX_PATH ];

		//	if ( SUCCEEDED ( LI_FN ( SHGetFolderPathA )( nullptr, N ( 5 ), nullptr, N ( 0 ), appdata ) ) ) {
		//		LI_FN ( CreateDirectoryA )( ( std::string ( appdata ) + _ ( "\\sesame" ) ).c_str ( ), nullptr );
		//		LI_FN ( CreateDirectoryA )( ( std::string ( appdata ) + _ ( "\\sesame\\configs" ) ).c_str ( ), nullptr );
		//	}

		//	std::ifstream cfg_file ( std::string ( appdata ) + _ ( "\\sesame\\configs\\" ) + selected_config + _ ( ".xml" ) );

		//	if ( cfg_file.is_open ( ) ) {
		//		std::string total = _ ( "" );
		//		std::string line = _ ( "" );

		//		while ( std::getline ( cfg_file, line ) )
		//			total += line;

		//		const auto post_obj = cJSON_CreateObject ( );

		//		if ( post_obj ) {
		//			cJSON_AddStringToObject ( post_obj, _ ( "config_name" ), selected_config );
		//			cJSON_AddStringToObject ( post_obj, _ ( "username" ), g_username.c_str ( ) );
		//			cJSON_AddStringToObject ( post_obj, _ ( "description" ), gui::config_description );
		//			cJSON_AddNumberToObject ( post_obj, _ ( "access" ), gui::config_access );
		//			cJSON_AddStringToObject ( post_obj, _ ( "data" ), total.c_str ( ) );

		//			const auto out_str = cJSON_Print ( post_obj );

		//			const auto out = networking::post (
		//				_ ( "sesame.one/api/cloud_configs/upload.php" ),
		//				out_str
		//			);

		//			if ( !out.empty ( ) ) {
		//				if ( out == _ ( "ERROR" ) || out == _ ( "NULL" ) ) {
		//					dbg_print ( _ ( "Failed to upload config to cloud.\n" ) );
		//				}
		//				else {
		//					if ( out == _ ( "1" ) )
		//						dbg_print ( _ ( "User has too many saved configurations (maximum 16).\n" ) );
		//					else if ( out == _ ( "2" ) )
		//						dbg_print ( _ ( "Config name is too long (maximum 64 characters).\n" ) );
		//					else if ( out == _ ( "3" ) )
		//						dbg_print ( _ ( "Config size is too large (maximum 2MB).\n" ) );
		//					else if ( out == _ ( "4" ) )
		//						dbg_print ( _ ( "Config description is too long (maximum 64 characters).\n" ) );
		//					else if ( out == _ ( "5" ) )
		//						dbg_print ( _ ( "Incomplete request.\n" ) );
		//					else if ( out == _ ( "6" ) )
		//						dbg_print ( _ ( "User not found.\n" ) );
		//					else if ( out == _ ( "7" ) )
		//						dbg_print ( _ ( "Unauthorized request.\n" ) );
		//					else {
		//						strcpy_s ( gui::config_code, out.c_str() );

		//						dbg_print ( _ ( "Uploaded config to cloud successfully! Config code: %s.\n" ), out.c_str ( ) );
		//						csgo::i::engine->client_cmd_unrestricted ( _ ( "play ui\\buttonclick" ) );
		//					}
		//				}
		//			}
		//			else {
		//				dbg_print ( _ ( "Empty response from server.\n" ) );
		//			}

		//			cJSON_Delete ( post_obj );
		//		}

		//		cfg_file.close ( );
		//	}

		//	upload_to_cloud = false;
		//}

		///* sync data with server */
		//const auto post_obj = cJSON_CreateObject ( );

		//cJSON_AddStringToObject ( post_obj, _ ( "username" ), g_username.c_str ( ) );
		//cJSON_AddStringToObject ( post_obj, _ ( "username_search" ), gui::config_user );

		//const auto out = networking::post (
		//	_ ( "sesame.one/api/cloud_configs/list.php" ),
		//	cJSON_Print ( post_obj )
		//);

		//if ( out == _ ( "ERROR" ) || out == _ ( "NULL" ) ) {
		//	dbg_print ( _ ( "Failed to grab config list from cloud.\n" ) );
		//}
		//else {
		//	//if ( out == _ ( "1" ) )
		//	//	dbg_print ( _ ( "User was not specified.\n" ) );
		//	//else if ( out == _ ( "2" ) )
		//	//	dbg_print ( _ ( "User or configs by this user were not found.\n" ) );
		//	//else if ( out == _ ( "0" ) );
		//	//dbg_print ( _ ( "Config list loaded.\n" ) );
		//}

		//cJSON_Delete ( post_obj );

		//gui::gui_mutex.lock ( );
		//if ( cloud_config_list )
		//	cJSON_Delete ( cloud_config_list );

		//cloud_config_list = cJSON_Parse ( out.c_str ( ) );
		//last_config_user = gui::config_user;
		//gui::gui_mutex.unlock ( );

		///* check if we should download the config */
		//if ( download_config_code ) {
		//	gui::gui_mutex.lock ( );
		//	const auto post_obj = cJSON_CreateObject ( );

		//	cJSON_AddStringToObject ( post_obj, _ ( "username" ), g_username.c_str ( ) );
		//	cJSON_AddStringToObject ( post_obj, _ ( "username_search" ), gui::config_user );
		//	cJSON_AddStringToObject ( post_obj, _ ( "config_code" ), gui::config_user );

		//	const auto out = networking::post (
		//		_ ( "sesame.one/api/cloud_configs/download.php" ),
		//		cJSON_Print ( post_obj )
		//	);

		//	cJSON_Delete ( post_obj );

		//	if ( out == _ ( "ERROR" ) || out == _ ( "NULL" ) ) {
		//		dbg_print ( _ ( "Failed to download config from cloud.\n" ) );
		//	}
		//	else {
		//		const auto as_json = cJSON_Parse ( out.c_str ( ) );

		//		if ( as_json ) {
		//			const auto config_id = cJSON_GetObjectItemCaseSensitive ( as_json, _ ( "config_id" ) );
		//			const auto config_name = cJSON_GetObjectItemCaseSensitive ( as_json, _ ( "config_name" ) );
		//			const auto config_code = cJSON_GetObjectItemCaseSensitive ( as_json, _ ( "config_code" ) );
		//			const auto description = cJSON_GetObjectItemCaseSensitive ( as_json, _ ( "description" ) );
		//			const auto creation_date = cJSON_GetObjectItemCaseSensitive ( as_json, _ ( "creation_date" ) );
		//			const auto data = cJSON_GetObjectItemCaseSensitive ( as_json, _ ( "data" ) );

		//			if ( config_id && cJSON_IsNumber( config_id )
		//				&& config_name && cJSON_IsString ( config_name ) && config_name->valuestring
		//				&& config_code && cJSON_IsString ( config_code ) && config_name->valuestring
		//				&& description && cJSON_IsString ( description ) && config_name->valuestring
		//				&& creation_date && cJSON_IsString ( creation_date ) && config_name->valuestring
		//				&& data && cJSON_IsString ( data ) && config_name->valuestring ) {
		//				//if ( out == _ ( "1" ) )
		//		//	dbg_print ( _ ( "User was not specified.\n" ) );
		//		//else if ( out == _ ( "2" ) )
		//		//	dbg_print ( _ ( "User or configs by this user were not found.\n" ) );
		//		//else if ( out == _ ( "0" ) );
		//		//dbg_print ( _ ( "Config list loaded.\n" ) );

		//				char appdata [ MAX_PATH ];

		//				if ( SUCCEEDED ( LI_FN ( SHGetFolderPathA )( nullptr, N ( 5 ), nullptr, N ( 0 ), appdata ) ) ) {
		//					LI_FN ( CreateDirectoryA )( ( std::string ( appdata ) + _ ( "\\sesame" ) ).c_str ( ), nullptr );
		//					LI_FN ( CreateDirectoryA )( ( std::string ( appdata ) + _ ( "\\sesame\\configs" ) ).c_str ( ), nullptr );
		//				}

		//				std::ofstream file_out ( std::string ( appdata ) + _ ( "\\sesame\\configs\\" ) + config_name->valuestring + _ ( ".xml" ) );

		//				if ( file_out.is_open ( ) ) {
		//					file_out << data->valuestring;
		//					file_out.close ( );
		//				}

		//				gui::load_cfg_list ( );

		//				csgo::i::engine->client_cmd_unrestricted ( _ ( "play ui\\buttonclick" ) );
		//			}

		//			cJSON_Delete ( as_json );
		//		}
		//	}

		//	strcpy_s ( gui::config_code, "" );
		//	download_config_code = false;

		//	gui::gui_mutex.unlock ( );
		//}

		std::this_thread::sleep_for ( std::chrono::seconds ( N ( 1 ) ) );
	}

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
