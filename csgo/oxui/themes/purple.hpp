#ifdef OXUI_THEME_PURPLE_HPP
#ifndef OXUI_THEME_HPP
#define OXUI_THEME_HPP

#include "../types/types.hpp"

namespace oxui {
	struct {
		int spacing = 26;
		double animation_speed = 0.4;
		color bg = color( 22, 18, 22, 255 );
		color text = color( 200, 200, 200, 255 );
		color container_bg = color( 38, 38, 38, 255 );
		color main = color( 156, 155, 255, 255 );
	} static theme;
}

#endif // OXUI_THEME_HPP
#endif // OXUI_THEME_PURPLE_HPP