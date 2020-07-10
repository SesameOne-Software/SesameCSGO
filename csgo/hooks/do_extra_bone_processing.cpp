#include "do_extra_bone_processing.hpp"

decltype( &hooks::do_extra_bone_processing ) hooks::old::do_extra_bone_processing = nullptr;

void __fastcall hooks::do_extra_bone_processing ( REG, int a2, int a3, int a4, int a5, int a6, int a7 ) {
	return;
}