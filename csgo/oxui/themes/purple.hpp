#ifdef OXUI_THEME_PURPLE_HPP
#ifndef OXUI_THEME_HPP
#define OXUI_THEME_HPP

#include "../types/types.hpp"

namespace oxui {
	struct {
		int spacing = 22;
		double animation_speed = 0.4;
		color bg = color( 229, 236, 236, 255 );
		color text = color( 0, 0, 0, 255 );
		color container_bg = color( 252, 253, 253, 255 );
		color main = color( 43, 163, 69, 255 );
		color title_text = color( 191, 191, 191, 255 );
		color title_bar = color( 42, 42, 42, 255 );
	} static theme;
}

#endif // OXUI_THEME_HPP
#endif // OXUI_THEME_PURPLE_HPP