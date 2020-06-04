#ifndef OXUI_TEXTBOX_HPP
#define OXUI_TEXTBOX_HPP

#include <memory>
#include <vector>
#include <functional>
#include "object.hpp"
#include "../types/types.hpp"

namespace oxui {
	class textbox : public obj {
		bool opened = false;
		bool opened_shortcut_menu = false;
		bool last_rkey = false;
		pos rclick_pos;
		int hovered_index = 0;

	public:
		str label;
		str buf;
		bool hide_input;
		size_t len_max;

		textbox( const str& label, const str& buf, bool hide_input = false, size_t len_max = 32 ) {
			this->label = label;
			this->buf = buf;
			this->hide_input = hide_input;
			this->len_max = len_max;
			type = object_textbox;
		}

		~textbox( ) {}

		void handle_input( wchar_t key );
		void think( );
		void draw( ) override;
	};
}

#endif // OXUI_TEXTBOX_HPP