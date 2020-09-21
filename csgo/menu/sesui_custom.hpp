#pragma once
#include "sesui.hpp"

namespace sesui {
	namespace custom {
		extern bool tab_open;
		extern float tab_open_timer;
		extern sesui::font tab_font;

		bool begin_group ( std::string_view name, const rect& fraction, const rect& extra );
		void end_group ( );

		bool begin_subtabs ( int count );
		void subtab ( std::string_view name, std::string_view desc, const rect& fraction, const rect& extra, int& selected );
		void end_subtabs ( );

		bool begin_tabs ( int count, float width = 0.2f );
		void tab ( std::string_view icon, std::string_view name, int& selected );
		void end_tabs ( );
		bool begin_window ( std::string_view name, const rect& bounds, bool& opened, uint32_t flags );
		void end_window ( );
	}
}