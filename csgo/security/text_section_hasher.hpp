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

	typedef struct {
		LPVOID lpVirtualAddress;
		DWORD dwSizeOfRawData;
	} SECTIONINFO, * PSECTIONINFO;

	typedef struct {
		DWORD64 dwRealHash;
		SECTIONINFO SectionInfo;
	} HASHSET, * PHASHSET;

	__forceinline DWORD64 HashSection() {
		DWORD64 hash = 0;

		for (int i = 0; i < g_text_section_size; ++i) {
			if (*reinterpret_cast<uint8_t*> (g_text_section + i)) {
				hash = *reinterpret_cast<uint8_t*> (g_text_section + i) + (hash << 6) + (hash << 16) - hash;
			}
		}

		return hash;
	}
}
