#include <windows.h>
#include <cstdint>
#include <iostream>
#include <thread>
#include "utils/utils.hpp"
#include "minhook/minhook.h"
#include "sdk/sdk.hpp"
#include "hooks/hooks.hpp"
#include "globals.hpp"
#include "javascript/js_api.hpp"

#include "hooks/wnd_proc.hpp"

/* security */
#include "security/security_handler.hpp"

FILE* g_fp;

uint64_t anti_patch::g_text_section_hash;
uintptr_t anti_patch::g_text_section, anti_patch::g_text_section_size;
anti_patch::s_header_data anti_patch::g_header_data;

PVOID g_ImageStartAddr, g_ImageEndAddr;

struct EH4_SCOPETABLE_RECORD
{
	int EnclosingLevel;
	void* FilterFunc;
	void* HandlerFunc;
};

struct EH4_SCOPETABLE
{
	int GSCookieOffset;
	int GSCookieXOROffset;
	int EHCookieOffset;
	int EHCookieXOROffset;
	struct EH4_SCOPETABLE_RECORD ScopeRecord [ ];
};

struct EH4_EXCEPTION_REGISTRATION_RECORD
{
	void* SavedESP;
	EXCEPTION_POINTERS* ExceptionPointers;
	EXCEPTION_REGISTRATION_RECORD SubRecord;
	EH4_SCOPETABLE* EncodedScopeTable; //Xored with the __security_cookie
	unsigned int TryLevel;
};

LONG NTAPI ExceptionHandler ( _EXCEPTION_POINTERS* ExceptionInfo )
{
	//making sure to only process exceptions from the manual mapped code:
	PVOID ExceptionAddress = ExceptionInfo->ExceptionRecord->ExceptionAddress;
	if ( ExceptionAddress < g_ImageStartAddr || ExceptionAddress > g_ImageEndAddr )
		return EXCEPTION_CONTINUE_SEARCH;

	EXCEPTION_REGISTRATION_RECORD* pFs = ( EXCEPTION_REGISTRATION_RECORD* ) __readfsdword ( 0 ); // mov pFs, large fs:0 ; <= reading from the segment register
	if ( ( DWORD_PTR ) pFs > 0x1000 && ( DWORD_PTR ) pFs < 0xFFFFFFF0 ) //validate pointer
	{
		EH4_EXCEPTION_REGISTRATION_RECORD* EH4 = CONTAINING_RECORD ( pFs, EH4_EXCEPTION_REGISTRATION_RECORD, SubRecord );
		EXCEPTION_ROUTINE* EH4_ExceptionHandler = EH4->SubRecord.Handler;

		if ( EH4_ExceptionHandler > g_ImageStartAddr && EH4_ExceptionHandler < g_ImageEndAddr )//validate pointer
		{
			//calling the compiler generated function to do the work :D
			EXCEPTION_DISPOSITION ExceptionDisposition = EH4_ExceptionHandler ( ExceptionInfo->ExceptionRecord, &EH4->SubRecord, ExceptionInfo->ContextRecord, nullptr );
			if ( ExceptionDisposition == ExceptionContinueExecution )
				return EXCEPTION_CONTINUE_EXECUTION;
		}
	}
	return EXCEPTION_CONTINUE_SEARCH;
}

__declspec( noreturn ) void __stdcall mmunload ( void* img_base ) {
	void* exit_thread = GetProcAddress ( GetModuleHandleA ( _ ( "Kernel32.dll" ) ), _ ( "ExitThread" ) );
	void* virtual_free = GetProcAddress ( GetModuleHandleA ( _ ( "Kernel32.dll" ) ), _ ( "VirtualFree" ) );

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

int __stdcall init( uintptr_t mod ) {
	PIMAGE_NT_HEADERS pNtHdr = PIMAGE_NT_HEADERS ( uintptr_t ( mod ) + PIMAGE_DOS_HEADER ( mod )->e_lfanew );
	PIMAGE_SECTION_HEADER pSectionHeader = ( PIMAGE_SECTION_HEADER ) ( pNtHdr + 1 );

	g_ImageStartAddr = PVOID ( mod );
	g_ImageEndAddr = PVOID( mod + pNtHdr->OptionalHeader.SizeOfImage );

	/* fix SEH */
	AddVectoredExceptionHandler ( 1, ExceptionHandler );

	anti_patch::g_header_data.m_num_of_sections = pNtHdr->FileHeader.NumberOfSections;

	for ( int i = 0; i < anti_patch::g_header_data.m_num_of_sections; ++i )
		anti_patch::g_header_data.m_sections.push_back ( *pSectionHeader );

		/* open console debug window */
#ifdef DEV_BUILD
	//LI_FN( AllocConsole )( );
	//freopen_s( &g_fp, _( "CONOUT$" ), _( "w" ), stdout );
#endif // DEV_BUILD

	/* for anti-tamper */
	//LI_FN( LoadLibraryA )( _("ntoskrnl.exe") );

	/* wait for all modules to load */
		while( !LI_FN( GetModuleHandleA )( _( "serverbrowser.dll" ) ) )
		std::this_thread::sleep_for( std::chrono::milliseconds( N( 100 ) ) );

		/* initialize hack */
		csgo::init( );
		erase::erase_func ( csgo::init );
		netvars::init ( );
		erase::erase_func ( netvars::init );
		js::init ( );
		erase::erase_func ( js::init );
	hooks::init ( );
	erase::erase_func ( hooks::init );

	erase::erase_headers ( mod );

	//LI_FN ( SetWindowLongA )( LI_FN ( FindWindowA )( _ ( "Valve001" ), nullptr ), GWLP_WNDPROC, long ( hooks::o_wndproc ) );
	//
	//MH_RemoveHook ( MH_ALL_HOOKS );
	//MH_Uninitialize ( );
	//
	//std::this_thread::sleep_for ( std::chrono::milliseconds ( N ( 200 ) ) );
	//
	//if ( g::local )
	//	g::local->animate ( ) = true;
	//
	//FreeLibraryAndExitThread ( HMODULE ( mod ), 0 );

	END_FUNC

	return 0;
}

int __stdcall init_proxy ( uintptr_t mod ) {
	init ( mod );
	erase::erase_func ( init );
	security_handler::store_text_section_hash ( mod );

	while ( !g::unload )
		std::this_thread::sleep_for ( std::chrono::milliseconds ( N ( 100 ) ) );

	std::this_thread::sleep_for ( std::chrono::milliseconds ( N ( 500 ) ) );
	
	LI_FN ( SetWindowLongA )( LI_FN ( FindWindowA )( _ ( "Valve001" ), nullptr ), GWLP_WNDPROC, long ( hooks::old::wnd_proc ) );
	
	MH_RemoveHook ( MH_ALL_HOOKS );
	MH_Uninitialize ( );
	
	std::this_thread::sleep_for ( std::chrono::milliseconds ( N ( 200 ) ) );
	
	if ( g::local )
		g::local->animate ( ) = true;

	csgo::i::input->m_camera_in_thirdperson = false;
	
	static auto r_aspect_ratio = pattern::search ( _ ( "engine.dll" ), _ ( "81 F9 ? ? ? ? 75 16 F3 0F 10 0D ? ? ? ? F3 0F 11 4D ? 81 75 ? ? ? ? ? EB 18 8B 01 8B 40 30 FF D0 F3 0F 10 0D ? ? ? ? 8B 0D ? ? ? ? D9 5D FC F3 0F 10 45 ? 0F 2F 05 ? ? ? ? 76 34" ) ).add ( 2 ).deref ( ).get< void* > ( );
	auto as_float = 1.777777f;
	*reinterpret_cast< uintptr_t* > ( uintptr_t ( r_aspect_ratio ) + 0x2c ) = *reinterpret_cast< uintptr_t* > ( &as_float ) ^ uintptr_t ( r_aspect_ratio );

	/* reset world color */ {
		static auto load_named_sky = pattern::search ( _ ( "engine.dll" ), _ ( "55 8B EC 81 EC ? ? ? ? 56 57 8B F9 C7 45" ) ).get< void ( __fastcall* )( const char* ) > ( );

		load_named_sky ( _ ( "nukeblank" ) );

		for ( auto i = csgo::i::mat_sys->first_material ( ); i != csgo::i::mat_sys->invalid_material ( ); i = csgo::i::mat_sys->next_material ( i ) ) {
			auto mat = csgo::i::mat_sys->get_material ( i );

			if ( !mat || mat->is_error_material ( ) )
				continue;

			if ( strstr ( mat->get_texture_group_name ( ), _ ( "StaticProp" ) ) || strstr ( mat->get_texture_group_name ( ), _ ( "World" ) ) ) {
				mat->color_modulate ( 255, 255, 255 );
				mat->alpha_modulate ( 255 );
			}
		}
	}

	FreeLibraryAndExitThread ( HMODULE ( mod ), 0 );
	//mmunload ( reinterpret_cast< void* > ( mod ) );

	return 0;
}

int __stdcall DllMain( HINSTANCE inst, std::uint32_t reason, void* reserved ) {
	if ( reason == 1 )
		CreateThread ( nullptr, N ( 0 ), LPTHREAD_START_ROUTINE ( init_proxy ), HMODULE ( inst ), N ( 0 ), nullptr );

	return 1;
}