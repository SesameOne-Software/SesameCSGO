#ifndef OXUI_SHAPES_HPP
#define OXUI_SHAPES_HPP

#include "bindings/bindings.hpp"
#include "../types/types.hpp"
#include "object.hpp"

namespace oxui {
	namespace shapes {
		extern bool finished_input_frame;
		extern bool click_switch;
		extern bool old_click_switch;
		extern oxui::pos click_start;

		bool hovering( const rect& area, bool from_start = false, bool override = false );
		bool clicking( const rect& area, bool from_start = false, bool override = false );
		void box( const rect& area, const double& hover_time, bool highlight_on_hover, bool top, bool bottom, bool left, bool right, bool background = true );
	}
}

#endif // OXUI_SHAPES_HPP