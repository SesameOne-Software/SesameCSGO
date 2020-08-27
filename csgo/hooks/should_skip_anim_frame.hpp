#pragma once
#include "../sdk/sdk.hpp"

namespace hooks {
	bool __fastcall should_skip_anim_frame ( REG );

	namespace old {
		extern decltype( &hooks::should_skip_anim_frame ) should_skip_anim_frame;
	}
}