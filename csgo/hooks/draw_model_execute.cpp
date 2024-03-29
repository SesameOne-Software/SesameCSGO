﻿#include <deque>

#include "draw_model_execute.hpp"

#include "../features/chams.hpp"

decltype( &hooks::draw_model_execute ) hooks::old::draw_model_execute = nullptr;

void __fastcall hooks::draw_model_execute ( REG, void* ctx, void* state, const mdlrender_info_t& info, matrix3x4_t* bone_to_world ) {
	return features::chams::drawmodelexecute ( ctx, state, info, bone_to_world );
}