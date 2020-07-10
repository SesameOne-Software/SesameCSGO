#pragma once
#include <sdk.hpp>

namespace hooks {
	void __fastcall override_view ( REG, void* setup );

	namespace old {
		extern decltype( &override_view ) override_view;
	}
}