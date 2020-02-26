#ifndef OXUI_SHAPES_HPP
#define OXUI_SHAPES_HPP

#include "bindings/bindings.hpp"
#include "../types/types.hpp"
#include "object.hpp"

namespace oxui {
	namespace shapes {
		extern bool click_switch;

		bool hovering( const rect& area, bool from_start = false );
		bool clicking( const rect& area, bool from_start = false );
		void box( const rect& area, const double& hover_time, bool highlight_on_hover, bool top, bool bottom, bool left, bool right, bool background = true );
	}
}

#endif // OXUI_SHAPES_HPP