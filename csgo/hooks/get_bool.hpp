#pragma once
#include "../sdk/sdk.hpp"

namespace hooks {
	bool __fastcall get_int ( REG );

	namespace old {
		extern decltype( &hooks::get_int ) get_int;
	}
}