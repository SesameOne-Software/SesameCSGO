#pragma once
#include <sdk.hpp>

namespace features {
	namespace chams {
		extern vec3_t old_origin;
		extern bool in_model;

		void drawmodelexecute( void* ctx, void* state, const mdlrender_info_t& info, matrix3x4_t* bone_to_world );
	}
}