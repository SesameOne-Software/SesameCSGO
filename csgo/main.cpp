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
#include "security/security_handler.hpp"

FILE* g_fp;

int __stdcall init( uintptr_t mod ) {
	OBF_BEGIN

		/* open console debug window */
#ifdef DEV_BUILD
	// LI_FN( AllocConsole )( );
	// freopen_s( &g_fp, _( "CONOUT$" ), _( "w" ), stdout );
#endif // DEV_BUILD

	/* for anti-tamper */
	//LI_FN( LoadLibraryA )( _("ntoskrnl.exe") );

	/* wait for all modules to load */
		WHILE( !LI_FN( GetModuleHandleA )( _( "serverbrowser.dll" ) ) )
		std::this_thread::sleep_for( std::chrono::milliseconds( N( 100 ) ) );
	ENDWHILE

		/* initialize hack */
		csgo::init( );
	netvars::init( );
	hooks::init( );

	std::this_thread::sleep_for( std::chrono::seconds( N( 1 ) ) );

	//static const auto hash_once = security_handler::store_text_section_hash( V( mod ) );

	/* wait for quick unload key (only if dev build) */
#ifdef DEV_BUILD
	WHILE( !LI_FN( GetAsyncKeyState )( N( VK_END ) ) )
#elif
	WHILE( true )
#endif // DEV_BUILD
		/* run anti-debug */
		//security_handler::update( );
		std::this_thread::sleep_for( std::chrono::milliseconds( N( 500 ) ) );
	ENDWHILE

		/* unload */
		MH_RemoveHook( MH_ALL_HOOKS );
	MH_Uninitialize( );

	IF( g::local )
		g::local->animate( ) = true;
	ENDIF

#ifdef DEV_BUILD
		// LI_FN( FreeConsole )( );
#endif // DEV_BUILD

		LI_FN( SetWindowLongA )( LI_FN( FindWindowA )( _( "Valve001" ), nullptr ), GWLP_WNDPROC, long( hooks::o_wndproc ) );

	OBF_END

		LI_FN( FreeLibraryAndExitThread )( HMODULE( mod ), N( 0 ) );
}

int __stdcall DllMain( HINSTANCE inst, std::uint32_t reason, void* reserved ) {
	if ( reason == N( 1 ) )
		LI_FN( CreateThread )( nullptr, N( 0 ), LPTHREAD_START_ROUTINE( init ), HMODULE( inst ), N( 0 ), nullptr );

	return N( 1 );
}