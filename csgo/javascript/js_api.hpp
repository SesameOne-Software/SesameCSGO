#pragma once
#include <string>

namespace js {
	void init ( );
	void destroy_fonts ( );
	void reset_fonts ( );
	void process_render_callbacks ( );
	void process_net_update_callbacks ( );
	void process_net_update_end_callbacks ( );
	void process_create_move_callbacks ( );
	void load ( const std::wstring& script );
	void reset ( );
}