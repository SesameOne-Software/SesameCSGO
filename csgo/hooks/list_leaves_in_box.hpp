#pragma once
#include <sdk.hpp>

namespace hooks {
	int __fastcall list_leaves_in_box ( REG, vec3_t& mins, vec3_t& maxs, uint16_t* list, int list_max );

	namespace old {
		extern decltype( &list_leaves_in_box ) list_leaves_in_box;
	}
}