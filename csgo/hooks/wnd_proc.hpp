#pragma once
#include "../sdk/sdk.hpp"

namespace hooks {
	LRESULT wnd_proc ( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam );

	namespace old {
		extern WNDPROC wnd_proc;
	}
}