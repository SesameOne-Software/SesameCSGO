#pragma once
#include "../sdk/sdk.hpp"

namespace hooks {
	void __fastcall post_network_data_received ( REG, int commands_acknowledged );

	namespace old {
		extern decltype( &hooks::post_network_data_received ) post_network_data_received;
	}
}
