#include <windows.h>
#include <cstdint>
#include <iostream>
#include <thread>
#include "utils/utils.h"
#include "minhook/minhook.h"
#include "sdk/sdk.h"
#include "hooks.h"

void init( HMODULE mod ) {
	while ( !GetModuleHandleA( "serverbrowser.dll" ) )
		std::this_thread::sleep_for( std::chrono::milliseconds( 100 ) );

	// initialize cheat
	csgo::init( );
	netvars::init( );
	hooks::init( );

	// wait for input
	while ( !GetAsyncKeyState( VK_END ) )
		std::this_thread::sleep_for( std::chrono::milliseconds( 500 ) );

	// unhook functions and restore window
	MH_RemoveHook( MH_ALL_HOOKS );
	MH_Uninitialize( );
	SetWindowLongA( FindWindowA( "Valve001", nullptr ), GWLP_WNDPROC, ( long ) hooks::o_wndproc );

	FreeLibraryAndExitThread( mod, 0 );
}

int __stdcall DllMain( HINSTANCE inst, std::uint32_t reason, void* reserved ) {
	if ( reason == 1 ) {
		CreateThread( nullptr, 0, reinterpret_cast< LPTHREAD_START_ROUTINE >( init ), reinterpret_cast< HMODULE >( inst ), 0, nullptr );
	}

	return 1;
}