#pragma once
#include <Windows.h>
#include <TlHelp32.h>
#include <DbgHelp.h>
#include <stdio.h>
#include <string.h>
#include <thread>
#include <vector>
#include "security_handler.hpp"

namespace anti_patch {
	extern uint64_t g_text_section_hash;
	extern uintptr_t g_text_section;
	extern uintptr_t g_text_section_size;

	struct s_header_data {
		size_t m_num_of_sections;
		std::vector< IMAGE_SECTION_HEADER > m_sections;
	};

	extern s_header_data g_header_data;

	typedef struct {
		LPVOID lpVirtualAddress;
		DWORD dwSizeOfRawData;
	} SECTIONINFO, * PSECTIONINFO;

	typedef struct {
		DWORD64 dwRealHash;
		SECTIONINFO SectionInfo;
	} HASHSET, * PHASHSET;

	__forceinline int GetTextSectionInfo( uintptr_t lpModBaseAddr ) {
		for ( int i = 0; i < anti_patch::g_header_data.m_num_of_sections; ++i ) {
			if ( !strcmp( ( char* ) anti_patch::g_header_data.m_sections [ i ].Name, _( ".text" ) ) ) {
				g_text_section = ( uintptr_t ) ( ( DWORD64 ) lpModBaseAddr + anti_patch::g_header_data.m_sections [ i ].VirtualAddress );
				g_text_section_size = anti_patch::g_header_data.m_sections [ i ].SizeOfRawData;
				break;
			}
		}

		return 0;
	}

	__forceinline DWORD64 HashSection( ) {
		DWORD64 hash = 0;

		for ( int i = 0; i < g_text_section_size; ++i ) {
			if ( *reinterpret_cast< uint8_t* > ( g_text_section + i ) ) {
				hash = *reinterpret_cast< uint8_t* > ( g_text_section + i ) + ( hash << 6 ) + ( hash << 16 ) - hash;
			}
		}

		return hash;
	}
}