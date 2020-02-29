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
	static uint64_t g_text_section_hash;
	static uintptr_t g_this_module;

	typedef struct {
		LPVOID lpVirtualAddress;
		DWORD dwSizeOfRawData;
	} SECTIONINFO, * PSECTIONINFO;

	typedef struct {
		DWORD64 dwRealHash;
		SECTIONINFO SectionInfo;
	} HASHSET, * PHASHSET;

	static __forceinline int GetTextSectionInfo( uintptr_t lpModBaseAddr, PSECTIONINFO info ) {
		PIMAGE_NT_HEADERS pNtHdr = LI_FN( ImageNtHeader )( (void*)lpModBaseAddr );
		PIMAGE_SECTION_HEADER pSectionHeader = ( PIMAGE_SECTION_HEADER ) ( pNtHdr + 1 );

		LPVOID lpTextAddr = NULL;
		DWORD dwSizeOfRawData = NULL;

		for ( int i = 0; i < pNtHdr->FileHeader.NumberOfSections; ++i ) {
			char* name = ( char* ) pSectionHeader->Name;

			if ( !strcmp( name, _( ".text" ) ) ) {
				info->lpVirtualAddress = ( LPVOID ) ( ( DWORD64 ) lpModBaseAddr + pSectionHeader->VirtualAddress );
				info->dwSizeOfRawData = pSectionHeader->SizeOfRawData;
				break;
			}

			++pSectionHeader;
		}

		if ( info->dwSizeOfRawData == NULL ) {
			return -1;
		}

		return 0;
	}

	static __forceinline DWORD64 HashSection( LPVOID lpSectionAddress, DWORD dwSizeOfRawData ) {
		DWORD64 hash = 0;
		BYTE* str = ( BYTE* ) lpSectionAddress;
		for ( int i = 0; i < dwSizeOfRawData; ++i, ++str ) {
			if ( *str ) {
				hash = *str + ( hash << 6 ) + ( hash << 16 ) - hash;
			}
		}

		return hash;
	}
}