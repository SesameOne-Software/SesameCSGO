#pragma once
#include "../sdk/sdk.hpp"

namespace hooks {
	float __fastcall get_viewmodel_fov ( REG );

	namespace old {
		extern decltype( &hooks::get_viewmodel_fov ) get_viewmodel_fov;
	}
}