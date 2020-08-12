#include "wnd_proc.hpp"
#include "../menu/menu.hpp"

extern WNDPROC hooks::old::wnd_proc = nullptr;

long __stdcall hooks::wnd_proc ( HWND hwnd, std::uint32_t msg, std::uintptr_t wparam, std::uint32_t lparam ) {
	auto skip_mouse_input_processing = false;

	switch ( msg ) {
	case WM_LBUTTONDOWN: case WM_LBUTTONDBLCLK:
	case WM_RBUTTONDOWN: case WM_RBUTTONDBLCLK:
	case WM_MBUTTONDOWN: case WM_MBUTTONDBLCLK:
	case WM_XBUTTONDOWN: case WM_XBUTTONDBLCLK: {
		int button = 0;
		if ( msg == WM_LBUTTONDOWN || msg == WM_LBUTTONDBLCLK ) { button = 0; }
		if ( msg == WM_RBUTTONDOWN || msg == WM_RBUTTONDBLCLK ) { button = 1; }
		if ( msg == WM_MBUTTONDOWN || msg == WM_MBUTTONDBLCLK ) { button = 2; }
		if ( msg == WM_XBUTTONDOWN || msg == WM_XBUTTONDBLCLK ) { button = ( GET_XBUTTON_WPARAM ( wparam ) == XBUTTON1 ) ? 3 : 4; }
		mouse_down [ button ] = true;
		skip_mouse_input_processing = true;
		break;
	}
	case WM_LBUTTONUP:
	case WM_RBUTTONUP:
	case WM_MBUTTONUP:
	case WM_XBUTTONUP: {
		int button = 0;
		if ( msg == WM_LBUTTONUP ) { button = 0; }
		if ( msg == WM_RBUTTONUP ) { button = 1; }
		if ( msg == WM_MBUTTONUP ) { button = 2; }
		if ( msg == WM_XBUTTONUP ) { button = ( GET_XBUTTON_WPARAM ( wparam ) == XBUTTON1 ) ? 3 : 4; }
		mouse_down [ button ] = false;
		skip_mouse_input_processing = true;
		break;
	}
	case WM_KEYDOWN:
	case WM_SYSKEYDOWN:
		if ( wparam < 256 )
			key_down [ wparam ] = true;
		break;
	case WM_KEYUP:
	case WM_SYSKEYUP:
		if ( wparam < 256 )
			key_down [ wparam ] = false;
		break;
	case WM_MOUSEWHEEL:
		sesui::input::scroll_amount += static_cast< float > ( GET_WHEEL_DELTA_WPARAM ( wparam ) ) / static_cast< float > ( WHEEL_DELTA );
		break;
	}

	if ( gui::opened && ( ( skip_mouse_input_processing || wparam <= VK_XBUTTON2 ) || ( msg == WM_MOUSEWHEEL ) ) )
		return true;

	return CallWindowProcA ( old::wnd_proc, hwnd, msg, wparam, lparam );
}