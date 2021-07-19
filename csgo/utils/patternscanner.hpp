#pragma once
#include <memory>
#include <windows.h>
#include <psapi.h>

#include "../security/security_handler.hpp"
#include "../globals.hpp"

#define in_range( x, a, b ) ( x >= a && x <= b )
#define get_bits( x ) ( in_range( ( x & ( ~0x20 ) ), 'A', 'F' ) ? ( ( x & ( ~0x20 ) ) - 'A' + 0xA ) : ( in_range( x, '0', '9' ) ? x - '0' : 0 ) )
#define get_byte( x ) ( get_bits( x[ 0 ] ) << 4 | get_bits( x[ 1 ] ) )

class pattern {
	uintptr_t m_addr;

public:
	__forceinline pattern( std::uintptr_t addr ) {
		this->m_addr = addr;
	}

	template < typename t >
	__forceinline t get( ) {
		return t( this->m_addr );
	}

	__forceinline pattern sub( uintptr_t bytes ) {
		return pattern( this->m_addr - bytes );
	}

	__forceinline pattern add( std::uintptr_t bytes ) {
		return pattern( this->m_addr + bytes );
	}

	__forceinline pattern deref( ) {
		return pattern( *( uintptr_t* ) this->m_addr );
	}

	__forceinline pattern resolve_rip( ) {
		return pattern( this->m_addr + *( int* ) ( this->m_addr + 1 ) + 5 );
	}

	__forceinline static void dbg_print ( const char* msg, ... ) {
		if ( !msg )
			return;

		static void ( __cdecl * msg_fn )( const char*, va_list ) = ( decltype( msg_fn ) ) ( LI_FN ( GetProcAddress )( LI_FN ( GetModuleHandleA )( _ ( "tier0.dll" ) ), _ ( "Msg" ) ) );
		char buffer [ 989 ];
		va_list list;
		va_start ( list, msg );
		vsprintf ( buffer, msg, list );
		va_end ( list );
		msg_fn ( buffer, list );
	}

	static pattern search( const char* mod, const char* pat ) {
		auto pat1 = const_cast< char* >( pat );
		auto range_start = reinterpret_cast< uintptr_t >( LI_FN( GetModuleHandleA )( mod ) );

		MODULEINFO mi;
		LI_FN( K32GetModuleInformation )( LI_FN( GetCurrentProcess )( ), reinterpret_cast< HMODULE >( range_start ), &mi, sizeof( MODULEINFO ) );

		auto end = range_start + mi.SizeOfImage;

		uintptr_t first_match = 0;

		for ( uintptr_t current_address = range_start; current_address < end; current_address++ ) {
			if ( !*pat1 ) {
				return pattern ( first_match );
			}

			if ( *reinterpret_cast< uint8_t* >( pat1 ) == '\?' || *reinterpret_cast< uint8_t* >( current_address ) == get_byte ( pat1 ) ) {
				if ( !first_match )
					first_match = current_address;

				if ( !pat1 [ 2 ] ) {
						return pattern ( first_match );
				}

				if ( *reinterpret_cast< uint16_t* >( pat1 ) == '\?\?' || *reinterpret_cast< uint8_t* >( pat1 ) != '\?' )
					pat1 += 3;
				else
					pat1 += 2;
			}
			else {
				pat1 = const_cast< char* >( pat );
				first_match = 0;
			}
		}

		pattern::dbg_print ( _ ( "Failed to find pattern \"%s\".\n" ), pat );

		return pattern( 0 );
	}
};