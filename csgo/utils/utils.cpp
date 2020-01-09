#include <iostream>
#include <cstdarg>
#include <cstdio>
#include <windows.h>
#include "utils.h"

FILE* utils::console = nullptr;

void utils::log( const wchar_t* msg, ... ) {
	wchar_t buf [ 500 ];
	std::va_list args;
	va_start( args, msg );
	vswprintf( buf, 500, msg, args );
	va_end( args );

	const auto prefix = L"[ " CHEAT_NAME L" ] ";

	WriteConsoleW( GetStdHandle( STD_OUTPUT_HANDLE ), prefix, std::wcslen( prefix ), nullptr, nullptr );
	WriteConsoleW( GetStdHandle( STD_OUTPUT_HANDLE ), buf, std::wcslen( buf ), nullptr, nullptr );
	WriteConsoleA( GetStdHandle( STD_OUTPUT_HANDLE ), "\n", 1, nullptr, nullptr );
}