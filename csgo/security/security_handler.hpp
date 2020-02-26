#include <windows.h>
#include <cstdlib>
#include "lazy_importer.hpp"
#include "xorstr.hpp"

namespace security_handler {
	static __forceinline void handle_tampering( ) {
		/* TODO: We should report the person to the server here. */
		/* Also, we need to corrupt valuable information and cause a hard-to-patch crash. */

		void* a;
		a = ( void* ) ( LI_FN( GetProcAddress )( LI_FN( GetModuleHandleA )( _( "ntoskrnl.exe" ) ), _( "KeBugCheckEx" ) ) );
		const auto KeBugCheckEx = ( int( * )( ULONG, ULONG_PTR, ULONG_PTR, ULONG_PTR, ULONG_PTR ) )a;
		KeBugCheckEx( 0, 0, 0, 0, 0 );

		LI_FN( RaiseException )( LI_FN( rand )( ), 0, 0, nullptr );

		LI_FN( exit )( 0 );
		LI_FN( abort )( );
	}
}