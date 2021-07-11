#pragma once
#include "../sdk/sdk.hpp"

namespace hooks {
	void __fastcall packet_start ( REG, int incoming, int outgoing );

	namespace old {
		extern decltype( &hooks::packet_start ) packet_start;
	}
}