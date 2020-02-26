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
	static void* g_this_module;

	typedef struct {
		LPVOID lpVirtualAddress;
		DWORD dwSizeOfRawData;
	} SECTIONINFO, * PSECTIONINFO;

	typedef struct {
		DWORD64 dwRealHash;
		SECTIONINFO SectionInfo;
	} HASHSET, * PHASHSET;

	static int GetAllModule( std::vector<LPVOID>& modules ) {
		MODULEENTRY32W mEntry;
		memset( &mEntry, 0, sizeof( mEntry ) );
		mEntry.dwSize = sizeof( MODULEENTRY32 );

		DWORD curPid = GetCurrentProcessId( );

		HANDLE hSnapshot = CreateToolhelp32Snapshot( TH32CS_SNAPMODULE, NULL );
		if ( Module32FirstW( hSnapshot, &mEntry ) ) {
			do {
				modules.emplace_back( mEntry.modBaseAddr );
			} while ( Module32NextW( hSnapshot, &mEntry ) );
		}

		CloseHandle( hSnapshot );

		if ( modules.empty( ) ) {
			return -1;
		}

		return 0;
	}

	static int GetTextSectionInfo( LPVOID lpModBaseAddr, PSECTIONINFO info ) {
		PIMAGE_NT_HEADERS pNtHdr = ImageNtHeader( lpModBaseAddr );
		PIMAGE_SECTION_HEADER pSectionHeader = ( PIMAGE_SECTION_HEADER ) ( pNtHdr + 1 );

		LPVOID lpTextAddr = NULL;
		DWORD dwSizeOfRawData = NULL;

		for ( int i = 0; i < pNtHdr->FileHeader.NumberOfSections; ++i ) {
			char* name = ( char* ) pSectionHeader->Name;

			if ( !strcmp( name, ".text" ) ) {
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

	static DWORD64 HashSection( LPVOID lpSectionAddress, DWORD dwSizeOfRawData ) {
		DWORD64 hash = 0;
		BYTE* str = ( BYTE* ) lpSectionAddress;
		for ( int i = 0; i < dwSizeOfRawData; ++i, ++str ) {
			if ( *str ) {
				hash = *str + ( hash << 6 ) + ( hash << 16 ) - hash;
			}
		}

		return hash;
	}

	static __forceinline void verify_text_section( ) {
		SECTIONINFO info;
		GetTextSectionInfo( g_this_module, &info );

		/* text section was patched */
		if ( g_text_section_hash != HashSection( info.lpVirtualAddress, info.dwSizeOfRawData ) )
			security_handler::handle_tampering( );
	}
}