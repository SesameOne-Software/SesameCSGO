#pragma once
#include "../sdk/sdk.hpp"

namespace hooks {
	bool __fastcall get_bool ( REG );

	namespace old {
		extern decltype( &hooks::get_bool ) get_bool;
	}
}