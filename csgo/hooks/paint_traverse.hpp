#pragma once
#include <sdk.hpp>

namespace hooks {
	void __fastcall paint_traverse ( REG, int ipanel, bool force_repaint, bool allow_force );

	namespace old {
		extern decltype( &paint_traverse ) paint_traverse;
	}
}