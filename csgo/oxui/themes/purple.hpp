#ifdef OXUI_THEME_PURPLE_HPP
#ifndef OXUI_THEME_HPP
#define OXUI_THEME_HPP

#include "../types/types.hpp"

namespace oxui {
	struct _theme {
		/*
		int spacing = 20;
		int list_spacing = 0;
		double animation_speed = 0.4;
		color bg = color( 229, 236, 236, 255 );
		color text = color( 0, 0, 0, 255 );
		color container_bg = color( 252, 253, 253, 255 );
		color main = color( 43, 163, 69, 255 );
		color title_text = color( 191, 191, 191, 255 );
		color title_bar = color( 42, 42, 42, 255 );
		*/

		int spacing = 20;
		double animation_speed = 0.4;
		int list_spacing = 0;
		color bg = color( 0x27, 0x29, 0x3d, 255 );
		color text = color( 0x6d, 0x6e, 0x7c, 255 );
		color title_text = color( 0xf9, 0xf9, 0xf9, 255 );
		color container_bg = color( 0x17, 0x19, 0x25, 255 );
		color main = color( 0xd8, 0x50, 0xd4, 255 );
		color title_bar = color ( 0xd8, 0x50, 0xd4, 255 );
		color title_bar_low = color( 193, 83, 237, 255 );
	};

	extern _theme theme;
}

#endif // OXUI_THEME_HPP
#endif // OXUI_THEME_PURPLE_HPP