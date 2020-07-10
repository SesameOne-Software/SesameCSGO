#pragma once
#include <sdk.hpp>

namespace hooks {
	bool __fastcall write_usercmd_delta_to_buffer ( REG, int slot, void* buf, int from, int to, bool new_cmd );

	namespace old {
		extern decltype( &write_usercmd_delta_to_buffer ) write_usercmd_delta_to_buffer;
	}
}