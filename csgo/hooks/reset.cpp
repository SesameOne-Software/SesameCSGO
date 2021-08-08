#include "reset.hpp"
#include "../globals.hpp"
#include "../menu/menu.hpp"

#include "../features/esp.hpp"

#include "../imgui/imgui.h"
#include "../imgui/imgui_impl_dx9.h"
#include "../imgui/imgui_impl_win32.h"
#include "../imgui/imgui_internal.h"

#include "../features/skinchanger.hpp"

decltype( &hooks::reset ) hooks::old::reset = nullptr;

long __fastcall hooks::reset( REG, IDirect3DDevice9* device, D3DPRESENT_PARAMETERS* presentation_params ) {
	ImGui_ImplDX9_InvalidateDeviceObjects ( );

	auto hr = old::reset( REG_OUT, device, presentation_params );

	if ( SUCCEEDED( hr ) ) {
		ImGui_ImplDX9_CreateDeviceObjects ( );
	}

	return hr;
}