#pragma once
#include "../sdk/sdk.hpp"

namespace hooks {
	int __fastcall send_datagram ( REG, void* datagram );

	namespace old {
		extern decltype( &hooks::send_datagram ) send_datagram;
	}
}