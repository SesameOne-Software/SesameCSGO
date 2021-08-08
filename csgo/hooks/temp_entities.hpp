#pragma once
#include "../sdk/sdk.hpp"

namespace hooks {
	bool __fastcall temp_entities ( REG, void* msg );

	namespace old {
		extern decltype( &hooks::temp_entities ) temp_entities;
	}
}
