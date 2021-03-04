#pragma once
#include "../sdk/sdk.hpp"

namespace hooks {
	int process_interp_list ( );

	namespace old {
		extern decltype( &hooks::process_interp_list ) process_interp_list;
	}
}