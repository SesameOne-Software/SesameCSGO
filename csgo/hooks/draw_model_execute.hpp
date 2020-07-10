#pragma once
#include <sdk.hpp>

namespace hooks {
	void __fastcall draw_model_execute ( REG, void* ctx, void* state, const mdlrender_info_t& info, matrix3x4_t* bone_to_world );

	namespace old {
		extern decltype( &draw_model_execute ) draw_model_execute;
	}
}