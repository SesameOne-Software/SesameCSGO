#pragma once
#include "../sdk/sdk.hpp"

namespace hooks {
	bool __fastcall netmsg_tick ( REG, void* msg );

	namespace old {
		extern decltype( &hooks::netmsg_tick ) netmsg_tick;
	}
}
