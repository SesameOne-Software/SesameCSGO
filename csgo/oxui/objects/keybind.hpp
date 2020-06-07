#ifndef OXUI_KEYBIND_HPP
#define OXUI_KEYBIND_HPP

#include <memory>
#include <vector>
#include <functional>
#include "object.hpp"
#include "../types/types.hpp"

namespace oxui {
	enum class keybind_mode : int {
		hold = 0,
		toggle,
		always
	};

	class keybind : public obj {
	public:
		str label;
		bool searching;
		int key;
		keybind_mode mode;

		bool opened_shortcut_menu;
		int hovered_index = 0;
		pos rclick_pos;
		bool last_rkey = false;

		keybind ( const str& label ) {
			this->label = label;
			this->key = -1;
			this->searching = false;
			this->mode = keybind_mode::hold;
			type = object_keybind;
		}

		~keybind ( ) {}

		void think( );
		void draw( ) override;
	};
}

#endif // OXUI_KEYBIND_HPP