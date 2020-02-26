#include <windows.h>
#include <cstdint>
#include <iostream>
#include <thread>
#include "utils/utils.hpp"
#include "minhook/minhook.h"
#include "sdk/sdk.hpp"
#include "hooks.hpp"
#include "globals.hpp"

void init( HMODULE mod ) {
	/* wait for all modules to load */
	while ( !GetModuleHandleA( "serverbrowser.dll" ) )
		std::this_thread::sleep_for( std::chrono::milliseconds( 100 ) );

	/* initialize hack */
	csgo::init( );
	netvars::init( );
	hooks::init( );

	/* wait for unload key */
	while ( !GetAsyncKeyState( VK_END ) )
		std::this_thread::sleep_for( std::chrono::milliseconds( 500 ) );

	if ( g::local )
		g::local->animate( ) = true;

	/* unload */
	MH_RemoveHook( MH_ALL_HOOKS );
	MH_Uninitialize( );
	SetWindowLongA( FindWindowA( "Valve001", nullptr ), GWLP_WNDPROC, long( hooks::o_wndproc ) );

	FreeLibraryAndExitThread( mod, 0 );
}

int __stdcall DllMain( HINSTANCE inst, std::uint32_t reason, void* reserved ) {
	if ( reason == 1 )
		CreateThread( nullptr, 0, LPTHREAD_START_ROUTINE( init ), HMODULE( inst ), 0, nullptr );

	return 1;
}