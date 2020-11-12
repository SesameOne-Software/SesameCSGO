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

FILE* g_fp;

uint64_t anti_patch::g_text_section_hash;
uintptr_t anti_patch::g_text_section, anti_patch::g_text_section_size;
//anti_patch::s_header_data anti_patch::g_header_data;

PVOID g_ImageStartAddr, g_ImageEndAddr;

struct EH4_SCOPETABLE_RECORD {
	int EnclosingLevel;
	void* FilterFunc;
	void* HandlerFunc;
};

struct EH4_SCOPETABLE {
	int GSCookieOffset;
	int GSCookieXOROffset;
	int EHCookieOffset;
	int EHCookieXOROffset;
	struct EH4_SCOPETABLE_RECORD ScopeRecord [ ];
};

struct EH4_EXCEPTION_REGISTRATION_RECORD {
	void* SavedESP;
	EXCEPTION_POINTERS* ExceptionPointers;
	EXCEPTION_REGISTRATION_RECORD SubRecord;
	EH4_SCOPETABLE* EncodedScopeTable; //Xored with the __security_cookie
	unsigned int TryLevel;
};

LONG NTAPI ExceptionHandler( _EXCEPTION_POINTERS* ExceptionInfo ) {
	//making sure to only process exceptions from the manual mapped code:
	PVOID ExceptionAddress = ExceptionInfo->ExceptionRecord->ExceptionAddress;
	if ( ExceptionAddress < g_ImageStartAddr || ExceptionAddress > g_ImageEndAddr )
		return EXCEPTION_CONTINUE_SEARCH;

	EXCEPTION_REGISTRATION_RECORD* pFs = ( EXCEPTION_REGISTRATION_RECORD* )__readfsdword( 0 ); // mov pFs, large fs:0 ; <= reading from the segment register
	if ( ( DWORD_PTR )pFs > 0x1000 && ( DWORD_PTR )pFs < 0xFFFFFFF0 ) //validate pointer
	{
		EH4_EXCEPTION_REGISTRATION_RECORD* EH4 = CONTAINING_RECORD( pFs, EH4_EXCEPTION_REGISTRATION_RECORD, SubRecord );
		EXCEPTION_ROUTINE* EH4_ExceptionHandler = EH4->SubRecord.Handler;

		if ( EH4_ExceptionHandler > g_ImageStartAddr && EH4_ExceptionHandler < g_ImageEndAddr )//validate pointer
		{
			//calling the compiler generated function to do the work :D
			EXCEPTION_DISPOSITION ExceptionDisposition = EH4_ExceptionHandler( ExceptionInfo->ExceptionRecord, &EH4->SubRecord, ExceptionInfo->ContextRecord, nullptr );
			if ( ExceptionDisposition == ExceptionContinueExecution )
				return EXCEPTION_CONTINUE_EXECUTION;
		}
	}
	return EXCEPTION_CONTINUE_SEARCH;
}

__declspec( noreturn ) void __stdcall mmunload( void* img_base ) {
	void* exit_thread = GetProcAddress( GetModuleHandleA( _( "Kernel32.dll" ) ), _( "ExitThread" ) );
	void* virtual_free = GetProcAddress( GetModuleHandleA( _( "Kernel32.dll" ) ), _( "VirtualFree" ) );

	__asm {
		push 0
		push exit_thread
		push img_base
		push 0
		push MEM_RELEASE
		push exit_thread
		mov esi, virtual_free
		jmp esi
	}
}

typedef void* ( _stdcall* padd_veh )( ULONG, PVECTORED_EXCEPTION_HANDLER );
typedef HMODULE( _stdcall* pget_mod )( LPCSTR );
typedef void( _stdcall* psleep )( DWORD );
typedef void( *perase_func )( void* );
typedef bool( *pcsgo_init )( void );
typedef bool( *pnetvars_init )( void );
typedef void( *pjs_init )( void );
typedef void( *phooks_init )( void );

typedef struct _init_data {
	padd_veh add_veh;
	void* exception_handler;
	pget_mod get_mod;
	psleep sleep;
	perase_func erase_func;
	pcsgo_init csgo_init;
	pnetvars_init netvars_init;
	pjs_init js_init;
	phooks_init hooks_init;
	const char* server_browser;
} init_data, * pinit_data;

std::string decrypt_cbc( const std::string& encrypted, const unsigned char* key,
	size_t key_size,
	const unsigned char* iv,
	const bool padding ) {

	plusaes::Error e;

	std::vector<unsigned char> decrypted( encrypted.size( ) );
	unsigned long padded = 0;
	if ( padding ) {
		e = plusaes::decrypt_cbc( ( const unsigned char* )&encrypted.data( ) [ 0 ], ( unsigned long )encrypted.size( ), key, ( int )key_size, iv, &decrypted [ 0 ], ( unsigned long )decrypted.size( ), &padded );
	}
	else {
		e = plusaes::decrypt_cbc( ( const unsigned char* )&encrypted.data( ) [ 0 ], ( unsigned long )encrypted.size( ), key, ( int )key_size, iv, &decrypted [ 0 ], ( unsigned long )decrypted.size( ), 0 );
	}

	const std::string s( decrypted.begin( ), decrypted.end( ) - padded );

	if ( padding ) {
		const std::vector<unsigned char> ok_pad( padded ); \
			memcmp( &decrypted [ decrypted.size( ) - padded ], &ok_pad [ 0 ], padded );
	}

	return std::string( reinterpret_cast< const char* >( decrypted.data( ) ) );
}

typedef int( __stdcall* pinit )( void* );

void js_init_placeholder ( ) {
	csgo::i::engine->client_cmd_unrestricted ( _("clear"));
}

void call_init( PLoader_Info loader_info ) {
	g_ImageStartAddr = PVOID( loader_info->hMod );
	g_ImageEndAddr = PVOID( loader_info->hMod + loader_info->hMod_sz );

	anti_patch::g_text_section = uintptr_t( loader_info->section );
	anti_patch::g_text_section_size = loader_info->section_sz;

	init_data initdata;
	memset( &initdata, 0, sizeof( init_data ) );

	initdata.add_veh = AddVectoredExceptionHandler;
	initdata.csgo_init = csgo::init;
	initdata.erase_func = erase::erase_func;
	initdata.exception_handler = ExceptionHandler;
	initdata.get_mod = GetModuleHandleA;
	initdata.hooks_init = hooks::init;
	initdata.js_init = js_init_placeholder;
	initdata.netvars_init = netvars::init;
	initdata.server_browser = _( "serverbrowser.dll" );
	initdata.sleep = Sleep;

	std::string decrypted = decrypt_cbc( base64_decode( loader_info->init ), ( const unsigned char* )base64_decode( ( char* )loader_info->key ).data( ), 32, ( const unsigned char* )base64_decode( ( char* )loader_info->iv ).data( ), true );

	if ( decrypted.size( ) <= 0 )
		return;

	char* init_addr = &decrypted [ 0 ];

	DWORD old = NULL;
	VirtualProtect( init_addr, decrypted.size( ), PAGE_EXECUTE_READWRITE, &old );

	reinterpret_cast< pinit >( init_addr )( &initdata );

	VirtualProtect( init_addr, decrypted.size( ), old, &old );

	memset( &initdata, 0, sizeof( init_data ) );
	VirtualFree( ( void* )loader_info->init, 0, MEM_RELEASE );
}

int __stdcall init( uintptr_t mod ) {
	PIMAGE_NT_HEADERS pNtHdr = PIMAGE_NT_HEADERS( uintptr_t( mod ) + PIMAGE_DOS_HEADER( mod )->e_lfanew );
	PIMAGE_SECTION_HEADER pSectionHeader = ( PIMAGE_SECTION_HEADER )( pNtHdr + 1 );

	g_ImageStartAddr = PVOID( mod );
	g_ImageEndAddr = PVOID( mod + pNtHdr->OptionalHeader.SizeOfImage );

	/* fix SEH */
	AddVectoredExceptionHandler( 1, ExceptionHandler );

	/* wait for all modules to load */
	while ( !LI_FN( GetModuleHandleA )( _( "serverbrowser.dll" ) ) )
		std::this_thread::sleep_for( std::chrono::milliseconds( N( 100 ) ) );

	/* initialize hack */
	csgo::init( );
	erase::erase_func( csgo::init );
	
	netvars::init( );
	erase::erase_func( netvars::init );
	
	//js::init( );
	//erase::erase_func( js::init );
	
	hooks::init( );
	erase::erase_func( hooks::init );
	
	erase::erase_headers( mod );

	END_FUNC

		return 0;
}

extern std::string g_username;
extern bool upload_to_cloud;
extern char selected_config [ 128 ];
extern cJSON* cloud_config_list;
extern bool download_config_code;

std::string last_config_user;

int __stdcall init_proxy( PLoader_Info loader_info ) {
	//std::locale::global ( std::locale ( _("en_US.UTF-8") ) );

#ifndef DEV_BUILD
	call_init( loader_info );
#else
	init( uintptr_t( loader_info ) );
#endif
	
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

	csgo::i::input->m_camera_in_thirdperson = false;

	g::cvars::r_aspectratio->set_value ( 1.777777f );
	g::cvars::r_aspectratio->no_callback ( );

	/* reset world color */ {
		static auto load_named_sky = pattern::search( _( "engine.dll" ), _( "55 8B EC 81 EC ? ? ? ? 56 57 8B F9 C7 45" ) ).get< void( __fastcall* )( const char* ) >( );

		load_named_sky( _( "nukeblank" ) );

		for ( auto i = csgo::i::mat_sys->first_material( ); i != csgo::i::mat_sys->invalid_material( ); i = csgo::i::mat_sys->next_material( i ) ) {
			auto mat = csgo::i::mat_sys->get_material( i );

			if ( !mat || mat->is_error_material( ) )
				continue;

			if ( strstr( mat->get_texture_group_name( ), _( "StaticProp" ) ) || strstr( mat->get_texture_group_name( ), _( "World" ) ) ) {
				mat->color_modulate( 255, 255, 255 );
				mat->alpha_modulate( 255 );
			}
		}
	}

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
