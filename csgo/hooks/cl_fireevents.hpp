#pragma once
#include "../sdk/sdk.hpp"

namespace hooks {
	void __stdcall cl_fireevents ( );

	namespace old {
		extern decltype( &hooks::cl_fireevents ) cl_fireevents;
	}
}
