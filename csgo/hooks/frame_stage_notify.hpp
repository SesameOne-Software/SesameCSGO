#pragma once
#include "../sdk/sdk.hpp"

namespace hooks {
	void __fastcall frame_stage_notify ( REG, int stage );

	namespace old {
		extern decltype( &hooks::frame_stage_notify ) frame_stage_notify;
	}
}