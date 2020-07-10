#pragma once
#include <sdk.hpp>

namespace hooks {
	void __fastcall do_extra_bone_processing ( REG, int a2, int a3, int a4, int a5, int a6, int a7 );

	namespace old {
		extern decltype( &do_extra_bone_processing ) do_extra_bone_processing;
	}
}