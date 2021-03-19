#pragma once
#include "../sdk/sdk.hpp"

namespace hooks {
	void __fastcall perform_flashbang_effect ( REG, void* view_setup );

	namespace old {
		extern decltype( &hooks::perform_flashbang_effect ) perform_flashbang_effect;
	}
}