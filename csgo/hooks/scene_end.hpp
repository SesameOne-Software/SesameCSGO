#pragma once
#include <sdk.hpp>

namespace hooks {
	void __fastcall scene_end ( REG );

	namespace old {
		extern decltype( &scene_end ) scene_end;
	}
}