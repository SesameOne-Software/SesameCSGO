#include "get_viewmodel_fov.hpp"

#include "../menu/menu.hpp"
#include "../globals.hpp"

decltype( &hooks::get_viewmodel_fov ) hooks::old::get_viewmodel_fov = nullptr;

float __fastcall hooks::get_viewmodel_fov ( REG ) {
	OPTION ( double, viewmodel_fov, "Sesame->C->Other->Removals->Viewmodel FOV", oxui::object_slider );
	return static_cast < float > ( viewmodel_fov );
}