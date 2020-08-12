#pragma once
#include <windows.h>
#include <cstdlib>
#include <intrin.h>
#include "lazy_importer.hpp"
#include "xorstr.hpp"
#include "anti_debug.hpp"
#include "text_section_hasher.hpp"
#include "obfy.hpp"
#include "erase.hpp"

namespace security_handler {
	__forceinline int handle_tampering() {
		OBF_BEGIN
			/* TODO: We should report the person to the server here. */
			/* Also, we need to corrupt valuable information and cause a hard-to-patch crash. */

			__fastfail(1);
		LI_FN(RaiseException)(LI_FN(rand)(), 0, 0, nullptr);
		__fastfail(1);
		LI_FN(exit)(1);
		LI_FN(abort)();
		__fastfail(1);
		OBF_END
	}

	__forceinline int store_text_section_hash(uintptr_t target_module) {
		OBF_BEGIN
			anti_patch::g_text_section_hash = anti_patch::HashSection();
		OBF_END
			END_FUNC
	}

	__forceinline int verify_text_section_integrity() {
		OBF_BEGIN
			/* text section was patched */
			IF(!anti_patch::g_text_section_hash || anti_patch::g_text_section_hash != anti_patch::HashSection())
			handle_tampering();
		ENDIF
			OBF_END
	}

	__forceinline int update() {
		static time_t last_check_time = time(nullptr);

		OBF_BEGIN
			/* check if text section was patched */
		//if ( abs ( time ( nullptr ) - last_check_time ) > 5 ) {
		//	verify_text_section_integrity ( );
		//	last_check_time = time ( nullptr );
		//}

		/* check if csgo hooks, or current thread is being tampered with */
		//IF( int( security::check_security( ) ) != N( int( security::internal::debug_results::none ) ) )
		//	security_handler::handle_tampering( );
		//ENDIF
			OBF_END
	}
}
