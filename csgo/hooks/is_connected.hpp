#pragma once
#include "../sdk/sdk.hpp"

namespace hooks {
	bool __fastcall is_connected( REG );

	namespace old {
		extern decltype( &hooks::is_connected ) is_connected;
	}
}