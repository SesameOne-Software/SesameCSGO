#pragma once
#include <Windows.h>
#include <TlHelp32.h>
#include <DbgHelp.h>
#include <stdio.h>
#include <string.h>
#include <thread>
#include <vector>
#include "security_handler.hpp"

namespace erase {
#define END_FUNC		\
	__asm nop			\
	__asm nop			\
	__asm nop			\
	__asm nop

	__forceinline void erase_func ( void* pfunc ) {
		//unsigned long old_prot = 0;
		//size_t func_len = 0;
		//
		//for ( func_len = 0; ; func_len++ ) {
		//	if ( *( uint32_t* ) ( uintptr_t ( pfunc ) + func_len ) == 0x90909090 ) {
		//		auto found_end = false;
		//
		//		for ( auto i = 0; i < 32; i++ ) {
		//			if ( *( uint8_t* ) ( uintptr_t ( pfunc ) + func_len + i ) == 0xc3 ) {
		//				func_len += i + 1;
		//				found_end = true;
		//				break;
		//			}
		//			else if ( *( uint8_t* ) ( uintptr_t ( pfunc ) + func_len + i ) == 0xc2 ) {
		//				func_len += i + 2;
		//				found_end = true;
		//				break;
		//			}
		//		}
		//
		//		if ( found_end )
		//			break;
		//	}
		//}
		//
		//LI_FN( VirtualProtect ) ( pfunc, func_len, PAGE_EXECUTE_READWRITE, &old_prot );
		//memset ( pfunc, 0, func_len );
		//LI_FN ( VirtualProtect ) ( pfunc, func_len, old_prot, &old_prot );
	}

	__forceinline void erase_headers ( uintptr_t base ) {
		//const auto dos = PIMAGE_DOS_HEADER ( base );
		//const auto nt = PIMAGE_NT_HEADERS ( base + dos->e_lfanew );
		//const auto size_of_headers = nt->OptionalHeader.SizeOfHeaders;
		//
		//unsigned long old_prot = 0;
		//LI_FN ( VirtualProtect ) ( reinterpret_cast< void* > ( base ), size_of_headers, PAGE_EXECUTE_READWRITE, &old_prot );
		//memset ( reinterpret_cast< void* > ( base ), 0, size_of_headers );
		//LI_FN ( VirtualProtect ) ( reinterpret_cast< void* > ( base ), size_of_headers, old_prot, &old_prot );
	}
}