#pragma once
#include "../sdk/sdk.hpp"

namespace hooks {
	void* __fastcall get_client_model_renderable ( REG );

	namespace old {
		extern decltype( &hooks::get_client_model_renderable ) get_client_model_renderable;
	}
}