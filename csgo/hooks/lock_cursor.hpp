#pragma once
#include "../sdk/sdk.hpp"

namespace hooks {
	void __fastcall lock_cursor ( REG );

	namespace old {
		extern decltype( &hooks::lock_cursor ) lock_cursor;
	}
}