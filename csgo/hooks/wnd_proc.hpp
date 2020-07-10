#pragma once
#include <sdk.hpp>

namespace hooks {
	long __stdcall wnd_proc ( HWND hwnd, std::uint32_t msg, std::uintptr_t wparam, std::uint32_t lparam );

	namespace old {
		extern WNDPROC wnd_proc;
	}
}