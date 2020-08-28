#pragma once
#include "../sdk/sdk.hpp"

namespace hooks {
	void __fastcall override_view ( REG, void* setup );

	namespace old {
		extern decltype( &hooks::override_view ) override_view;
	}
}