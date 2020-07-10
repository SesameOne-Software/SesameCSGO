#pragma once
#include <sdk.hpp>

namespace hooks {
	bool __fastcall send_net_msg ( REG, void* msg, bool force_reliable, bool voice );

	namespace old {
		extern decltype( &send_net_msg ) send_net_msg;
	}
}