#include <windows.h>
#include <cstdint>
#include <iostream>
#include <thread>
#include "utils/utils.hpp"
#include "minhook/minhook.h"
#include "sdk/sdk.hpp"
#include "hooks.hpp"
#include "globals.hpp"

/* security */
#include "security/text_section_hasher.hpp"
#include "security/lazy_importer.hpp"
#include "security/xorstr.hpp"

void init( HMODULE mod ) {
	/* for anti-tamper */
	LI_FN( LoadLibraryA )( _("ntoskrnl.exe") );

	/* for anti-patch */
	anti_patch::g_this_module = mod;

	/* wait for all modules to load */
	while ( !LI_FN( GetModuleHandleA )( _("serverbrowser.dll") ) )
		std::this_thread::sleep_for( std::chrono::milliseconds( 100 ) );

	/* initialize hack */
	csgo::init( );
	netvars::init( );
	hooks::init( );

	/* wait for unload key */
	while ( !LI_FN( GetAsyncKeyState )( VK_END ) )
		std::this_thread::sleep_for( std::chrono::milliseconds( 500 ) );

	if ( g::local )
		g::local->animate( ) = true;

	/* unload */
	MH_RemoveHook( MH_ALL_HOOKS );
	MH_Uninitialize( );
	LI_FN( SetWindowLongA )( LI_FN( FindWindowA )( _( "Valve001" ), nullptr ), GWLP_WNDPROC, long( hooks::o_wndproc ) );

	FreeLibraryAndExitThread( mod, 0 );
}

int __stdcall DllMain( HINSTANCE inst, std::uint32_t reason, void* reserved ) {
	if ( reason == 1 )
		LI_FN( CreateThread )( nullptr, 0, LPTHREAD_START_ROUTINE( init ), HMODULE( inst ), 0, nullptr );

	return 1;
}