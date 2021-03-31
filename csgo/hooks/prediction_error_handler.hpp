#pragma once
#include "../sdk/sdk.hpp"

namespace hooks {
	int __fastcall prediction_error_handler ( REG, int a2, int a3, int a4 );

	namespace old {
		extern decltype( &hooks::prediction_error_handler ) prediction_error_handler;
	}
}