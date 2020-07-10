#pragma once
#include <sdk.hpp>

namespace hooks {
	int __fastcall send_datagram ( REG, void* datagram );

	namespace old {
		extern decltype( &send_datagram ) send_datagram;
	}
}