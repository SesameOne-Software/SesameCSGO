#pragma once
#include <Windows.h>

inline void* g_ImageStartAddr = nullptr;
inline void* g_ImageEndAddr = nullptr;

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

static LONG NTAPI ExceptionHandler ( _EXCEPTION_POINTERS* ExceptionInfo ) {
    PVOID ExceptionAddress = ExceptionInfo->ExceptionRecord->ExceptionAddress;

    if ( ExceptionAddress < g_ImageStartAddr || ExceptionAddress > g_ImageEndAddr )
        return EXCEPTION_CONTINUE_SEARCH;

    DWORD RegisterESP = ExceptionInfo->ContextRecord->Esp;
    EXCEPTION_REGISTRATION_RECORD* pFs = ( EXCEPTION_REGISTRATION_RECORD* ) __readfsdword ( 0 ); // mov pFs, large fs:0 ; <= reading the segment register

    if ( ( DWORD_PTR ) pFs > ( RegisterESP - 0x10000 ) && ( DWORD_PTR ) pFs < ( RegisterESP + 0x10000 ) ) {
        EXCEPTION_ROUTINE* ExceptionHandlerRoutine = pFs->Handler;
        if ( ExceptionHandlerRoutine > g_ImageStartAddr && ExceptionHandlerRoutine < g_ImageEndAddr ) {
            EXCEPTION_DISPOSITION ExceptionDisposition = ExceptionHandlerRoutine ( ExceptionInfo->ExceptionRecord, pFs, ExceptionInfo->ContextRecord, nullptr );

            if ( ExceptionDisposition == ExceptionContinueExecution )
                return EXCEPTION_CONTINUE_EXECUTION;
        }
    }

    return EXCEPTION_CONTINUE_SEARCH;
}