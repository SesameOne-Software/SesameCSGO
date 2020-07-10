#pragma once
#include <sdk.hpp>

namespace hooks {
	float __fastcall get_viewmodel_fov ( REG );

	namespace old {
		extern decltype( &get_viewmodel_fov ) get_viewmodel_fov;
	}
}