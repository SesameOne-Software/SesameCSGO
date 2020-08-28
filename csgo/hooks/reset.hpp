#pragma once
#include "../sdk/sdk.hpp"

namespace hooks {
	long __fastcall reset ( REG, IDirect3DDevice9* device, D3DPRESENT_PARAMETERS* presentation_params );

	namespace old {
		extern decltype( &hooks::reset ) reset;
	}
}