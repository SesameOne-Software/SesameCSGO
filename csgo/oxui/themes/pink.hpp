#ifdef OXUI_THEME_PINK_HPP
#ifndef OXUI_THEME_HPP
#define OXUI_THEME_HPP

#include "../types/types.hpp"

namespace oxui {
	struct {
		int spacing = 26;
		double animation_speed = 0.25;
		color bg = color( 22, 18, 22, 255 );
		color text = color( 200, 200, 200, 255 );
		color container_bg = color( 24, 24, 24, 255 );
		color main = color( 164, 155, 182, 255 );
	} static theme;
}

#endif
#endif // OXUI_THEME_PINK_HPP