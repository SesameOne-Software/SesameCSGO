#pragma once
#include <string>

namespace js_api {
	void init ( );
	void load_script ( std::string_view file_name );
}