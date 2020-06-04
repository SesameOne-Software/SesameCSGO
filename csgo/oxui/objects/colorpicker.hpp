#ifndef OXUI_COLORPICKER_HPP
#define OXUI_COLORPICKER_HPP

#include <memory>
#include <vector>
#include <functional>
#include "object.hpp"
#include "../types/types.hpp"

namespace oxui {
	class color_picker : public obj {
		int hovered_index;
		bool last_rkey;
		bool opened_shortcut_menu;
		pos rclick_pos;

	public:
		str label;
		color clr;
		bool opened;
		color clr_raw;
		double brightness_coeff;
		pos mcursor_pos;
		pos mcursor_pos1;
		pos mcursor_pos2;

		color_picker( const str& label, color clr ) {
			this->mcursor_pos = pos ( 0, 0 );
			this->mcursor_pos1 = pos ( 0, 0 );
			this->mcursor_pos2 = pos ( 0, 0 );
			this->hovered_index = 0;
			this->opened = false;
			this->label = label;
			this->clr = clr;
			this->clr_raw = clr;
			this->brightness_coeff = 1.0;
			this->last_rkey = false;
			this->opened_shortcut_menu = false;
			this->type = object_colorpicker;
		}

		~color_picker( ) {}

		void calculate_phase ( const pos& offset, color& color_out );
		void think( );
		void draw( ) override;
	};
}

#endif // OXUI_COLORPICKER_HPP