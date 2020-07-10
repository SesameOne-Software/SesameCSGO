#include "should_skip_anim_frame.hpp"

decltype( &hooks::should_skip_anim_frame ) hooks::old::should_skip_anim_frame = nullptr;

bool __fastcall hooks::should_skip_anim_frame ( REG ) {
	return false;
}