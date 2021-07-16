#pragma once
#include "../sdk/sdk.hpp"
#include "../animations/anims.hpp"

namespace features {
	namespace chams {
		extern vec3_t old_origin;
		extern bool in_model;

		void add_shot ( player_t* player, const anims::anim_info_t& anim_info );
		void cull_shots ( );
		void render_shots ( );
		void drawmodelexecute( void* ctx, void* state, const mdlrender_info_t& info, matrix3x4_t* bone_to_world );
	}
}