#pragma once
#include <sdk.hpp>

namespace features {
	namespace chams {
		extern bool in_model;

		bool create_materials( );
		void update_mats( );
		void drawmodelexecute( void* ctx, void* state, const mdlrender_info_t& info, matrix3x4_t* bone_to_world );
	}
}