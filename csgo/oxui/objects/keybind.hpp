#ifndef OXUI_KEYBIND_HPP
#define OXUI_KEYBIND_HPP

#include <memory>
#include <vector>
#include <functional>
#include "object.hpp"
#include "../types/types.hpp"

namespace oxui {
	class keybind : public obj {
	public:
		str label;
		bool searching;
		int key;

		keybind ( const str& label ) {
			this->label = label;
			this->key = -1;
			this->searching = false;
			type = object_keybind;
		}

		~keybind ( ) {}

		void think( );
		void draw( ) override;
	};
}

#endif // OXUI_KEYBIND_HPP