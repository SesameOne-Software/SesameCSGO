#pragma once
#include "../sdk/sdk.hpp"

namespace hooks {
	long __fastcall end_scene ( REG, IDirect3DDevice9* device );

	namespace old {
		extern decltype( &hooks::end_scene ) end_scene;
	}
}