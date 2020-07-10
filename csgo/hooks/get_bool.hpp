#pragma once
#include <sdk.hpp>

namespace hooks {
	bool __fastcall get_bool ( REG );

	namespace old {
		extern decltype( &get_bool ) get_bool;
	}
}