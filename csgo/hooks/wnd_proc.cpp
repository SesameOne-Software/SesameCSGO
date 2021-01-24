#include "wnd_proc.hpp"
#include "../menu/menu.hpp"

#include "../imgui/imgui.h"
#include "../imgui/imgui_impl_dx9.h"
#include "../imgui/imgui_impl_win32.h"
#include "../imgui/imgui_internal.h"

extern WNDPROC hooks::old::wnd_proc = nullptr;

// Forward declare message handler from imgui_impl_win32.cpp
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler ( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam );

LRESULT hooks::wnd_proc ( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam ) {
	auto skip_mouse_input_processing = false;

	switch ( uMsg ) {
	case WM_SYSCOMMAND:
		if ( ( wParam & 0xfff0 ) == SC_KEYMENU ) // Disable ALT application menu
			return 0;
		break;
	case WM_LBUTTONDOWN: case WM_LBUTTONDBLCLK:
	case WM_RBUTTONDOWN: case WM_RBUTTONDBLCLK:
	case WM_MBUTTONDOWN: case WM_MBUTTONDBLCLK:
	case WM_XBUTTONDOWN: case WM_XBUTTONDBLCLK: {
		MUTATE_START
		int button = 0;
		if ( uMsg == WM_LBUTTONDOWN || uMsg == WM_LBUTTONDBLCLK ) { button = 0; }
		if ( uMsg == WM_RBUTTONDOWN || uMsg == WM_RBUTTONDBLCLK ) { button = 1; }
		if ( uMsg == WM_MBUTTONDOWN || uMsg == WM_MBUTTONDBLCLK ) { button = 2; }
		if ( uMsg == WM_XBUTTONDOWN || uMsg == WM_XBUTTONDBLCLK ) { button = ( GET_XBUTTON_WPARAM ( wParam ) == XBUTTON1 ) ? 3 : 4; }
		mouse_down [ button ] = true;
		skip_mouse_input_processing = true;
		MUTATE_END
		break;
	}
	case WM_LBUTTONUP:
	case WM_RBUTTONUP:
	case WM_MBUTTONUP:
	case WM_XBUTTONUP: {
		MUTATE_START
		int button = 0;
		if ( uMsg == WM_LBUTTONUP ) { button = 0; }
		if ( uMsg == WM_RBUTTONUP ) { button = 1; }
		if ( uMsg == WM_MBUTTONUP ) { button = 2; }
		if ( uMsg == WM_XBUTTONUP ) { button = ( GET_XBUTTON_WPARAM ( wParam ) == XBUTTON1 ) ? 3 : 4; }
		mouse_down [ button ] = false;
		skip_mouse_input_processing = true;
		MUTATE_END
		break;
	}
	case WM_KEYDOWN:
	case WM_SYSKEYDOWN:
		if ( wParam < 256 )
			key_down [ wParam ] = true;
		break;
	case WM_KEYUP:
	case WM_SYSKEYUP:
		if ( wParam < 256 )
			key_down [ wParam ] = false;
		break;
	case WM_MOUSEWHEEL:
		break;
	}

	MUTATE_START
	ImGui_ImplWin32_WndProcHandler ( hWnd, uMsg, wParam, lParam );
	MUTATE_END
	if ( gui::opened && ( ( skip_mouse_input_processing || wParam <= VK_XBUTTON2 ) || ( uMsg == WM_MOUSEWHEEL ) ) )
		return true;

	MUTATE_START
	return CallWindowProcA ( old::wnd_proc, hWnd, uMsg, wParam, lParam );
	MUTATE_END
}