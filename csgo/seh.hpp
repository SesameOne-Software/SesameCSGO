#pragma once
#include <Windows.h>

struct EH4_SCOPETABLE_RECORD {
	int EnclosingLevel;
	void* FilterFunc;
	void* HandlerFunc;
};

struct EH4_SCOPETABLE {
	int GSCookieOffset;
	int GSCookieXOROffset;
	int EHCookieOffset;
	int EHCookieXOROffset;
	struct EH4_SCOPETABLE_RECORD ScopeRecord [ ];
};

struct EH4_EXCEPTION_REGISTRATION_RECORD {
	void* SavedESP;
	EXCEPTION_POINTERS* ExceptionPointers;
	EXCEPTION_REGISTRATION_RECORD SubRecord;
	EH4_SCOPETABLE* EncodedScopeTable; //Xored with the __security_cookie
	unsigned int TryLevel;
};

inline void* g_ImageStartAddr = nullptr, *g_ImageEndAddr = nullptr;

static LONG NTAPI ExceptionHandler ( _EXCEPTION_POINTERS* ExceptionInfo ) {
	PVOID ExceptionAddress = ExceptionInfo->ExceptionRecord->ExceptionAddress;

	if ( ExceptionAddress < g_ImageStartAddr || ExceptionAddress > g_ImageEndAddr )
		return EXCEPTION_CONTINUE_SEARCH;

	EXCEPTION_REGISTRATION_RECORD* pFs = ( EXCEPTION_REGISTRATION_RECORD* ) __readfsdword ( 0 );

	if ( ( DWORD_PTR ) pFs > 0x1000 && ( DWORD_PTR ) pFs < 0xFFFFFFF0 ) {
		EH4_EXCEPTION_REGISTRATION_RECORD* EH4 = CONTAINING_RECORD ( pFs, EH4_EXCEPTION_REGISTRATION_RECORD, SubRecord );
		EXCEPTION_ROUTINE* EH4_ExceptionHandler = EH4->SubRecord.Handler;

		if ( EH4_ExceptionHandler > g_ImageStartAddr && EH4_ExceptionHandler < g_ImageEndAddr ) {
			EXCEPTION_DISPOSITION ExceptionDisposition = EH4_ExceptionHandler ( ExceptionInfo->ExceptionRecord, &EH4->SubRecord, ExceptionInfo->ContextRecord, nullptr );

			if ( ExceptionDisposition == ExceptionContinueExecution )
				return EXCEPTION_CONTINUE_EXECUTION;
		}
	}

	return EXCEPTION_CONTINUE_SEARCH;
}