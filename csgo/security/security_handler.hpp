#pragma once
#include <windows.h>
#include <cstdlib>
#include "lazy_importer.hpp"
#include "xorstr.hpp"
#include "anti_debug.hpp"
#include "text_section_hasher.hpp"
#include "obfy.hpp"

namespace security_handler {
	static __forceinline int handle_tampering( ) {
		OBF_BEGIN
		/* TODO: We should report the person to the server here. */
		/* Also, we need to corrupt valuable information and cause a hard-to-patch crash. */

		const auto KeBugCheckEx = (void*) ( LI_FN( GetProcAddress )( LI_FN( GetModuleHandleA )( _( "ntoskrnl.exe" ) ), _( "KeBugCheckEx" ) ) );
		( ( int( * )( ULONG, ULONG_PTR, ULONG_PTR, ULONG_PTR, ULONG_PTR ) )KeBugCheckEx)( 0, 0, 0, 0, 0 );

		LI_FN( RaiseException )( LI_FN( rand )( ), 0, 0, nullptr );

		LI_FN( exit )( 0 );
		LI_FN( abort )( );
		OBF_END
	}

	static __forceinline int store_text_section_hash( uintptr_t target_module ) {
#ifdef ANTI_DEBUG
		OBF_BEGIN
		V( anti_patch::g_this_module ) = target_module;
		anti_patch::SECTIONINFO info;
		anti_patch::GetTextSectionInfo( V( anti_patch::g_this_module ), &info );
		V( anti_patch::g_text_section_hash ) = anti_patch::HashSection( info.lpVirtualAddress, info.dwSizeOfRawData );
		OBF_END
#endif // ANTI_DEBUG
	}

	static __forceinline int verify_text_section_integrity( ) {
		OBF_BEGIN
		anti_patch::SECTIONINFO info;
		anti_patch::GetTextSectionInfo( V( anti_patch::g_this_module ), &info );

		/* text section was patched */
		IF ( V( anti_patch::g_text_section_hash ) != anti_patch::HashSection( info.lpVirtualAddress, info.dwSizeOfRawData ) )
			handle_tampering( );
		ENDIF
		OBF_END
	}

	static __forceinline int update( ) {
#ifdef ANTI_DEBUG
		OBF_BEGIN
		/* check if text section was patched */
		verify_text_section_integrity( );

		/* check if csgo hooks, or current thread is being tampered with */
		IF ( int( security::check_security( ) ) != N( int( security::internal::debug_results::none ) ) )
			security_handler::handle_tampering( );
		ENDIF
		OBF_END
#endif // ANTI_DEBUG
	}
}