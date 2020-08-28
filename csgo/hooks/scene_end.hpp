#pragma once
#include "../sdk/sdk.hpp"

namespace hooks {
	void __fastcall scene_end ( REG );

	namespace old {
		extern decltype( &hooks::scene_end ) scene_end;
	}
}