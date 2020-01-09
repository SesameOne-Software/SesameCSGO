#pragma once
#include <vector>
#include <windows.h>

// simple and clean detour class
template < typename t >
class detour {
	void* m_addr = nullptr, *m_dst = nullptr, *m_trampoline = nullptr;
	std::size_t m_len;
	std::vector< std::uint8_t > m_bytes { };
	t o_func;

public:
	__forceinline detour( void* addr, void* dst, std::size_t len ) {
		m_addr = addr;
		m_dst = dst;
		m_len = len;
		m_trampoline = VirtualAlloc( nullptr, m_len + 5, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE );
	}

	__forceinline bool hook( bool trampoline = true ) {
		if ( m_len < 5 )
			return false;

		unsigned long prot = 0;
		VirtualProtect( m_addr, m_len, PAGE_EXECUTE_READWRITE, &prot );

		for ( int i = 0; i < m_len; i++ ) {
			auto& byte = *reinterpret_cast< std::uint8_t* >( reinterpret_cast< std::uintptr_t >( m_addr ) + i );
			*reinterpret_cast< std::uint8_t* >( reinterpret_cast< std::uintptr_t >( m_trampoline ) + i ) = byte;
			m_bytes.push_back( byte );
			byte = 0x90;
		}

		const auto rel32 = reinterpret_cast< std::uintptr_t >( m_dst ) - reinterpret_cast< std::uintptr_t >( m_addr ) - 5;

		*reinterpret_cast< std::uint8_t* >( m_addr ) = 0xE9;
		*reinterpret_cast< std::uint32_t* >( m_addr ) ( reinterpret_cast< std::uintptr_t >( m_addr ) + 1 ) = rel32;

		VirtualProtect( m_addr, m_len, prot, &prot );

		const auto target = reinterpret_cast< std::uintptr_t >( m_trampoline ) + m_len;
		const auto new_src = reinterpret_cast< std::uintptr_t >( m_addr ) + m_len;
		const auto trampoline_rel32 = reinterpret_cast< std::uintptr_t >( new_src ) - reinterpret_cast< std::uintptr_t >( target ) - 5;

		*reinterpret_cast< std::uint8_t* >( target ) = 0xE9;
		*reinterpret_cast< std::uint32_t* >( target + 1 ) = trampoline_rel32;

		o_func = trampoline ? reinterpret_cast< t >( m_trampoline ) : reinterpret_cast< t >( reinterpret_cast< std::uintptr_t >( m_addr ) + m_len );

		return true;
	}

	__forceinline bool unhook( ) {
		if ( !m_bytes.size( ) )
			return false;

		unsigned long prot = 0;
		VirtualProtect( m_addr, m_len, PAGE_EXECUTE_READWRITE, &prot );

		std::uintptr_t i = 0;

		for ( auto& byte : m_bytes ) {
			*( std::uint8_t* ) ( ( std::uintptr_t ) m_addr + i ) = byte;
			i++;
		}

		VirtualProtect( m_addr, m_len, prot, &prot );
		VirtualFree( m_trampoline, m_len + 5, MEM_RELEASE );

		return true;
	}

	__forceinline t original( ) {
		return o_func;
	}
};