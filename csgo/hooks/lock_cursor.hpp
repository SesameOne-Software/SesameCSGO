#pragma once
#include <sdk.hpp>

namespace hooks {
	void __fastcall lock_cursor ( REG );

	namespace old {
		extern decltype( &lock_cursor ) lock_cursor;
	}
}