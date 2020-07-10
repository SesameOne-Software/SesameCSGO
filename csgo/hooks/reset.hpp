#pragma once
#include <sdk.hpp>

namespace hooks {
	long __fastcall reset ( REG, IDirect3DDevice9* device, D3DPRESENT_PARAMETERS* presentation_params );

	namespace old {
		extern decltype( &reset ) reset;
	}
}