#ifndef OXUI_DROPDOWN_HPP
#define OXUI_DROPDOWN_HPP

#include <memory>
#include <vector>
#include <functional>
#include "object.hpp"
#include "../types/types.hpp"

namespace oxui {
	class dropdown : public obj {
		int hovered_index;

	public:
		str label;
		std::vector< str > items;
		int value;
		bool opened;

		dropdown( const str& label, const std::vector< str >& items ) {
			this->hovered_index = 0;
			this->value = 0;
			this->opened = false;
			this->label = label;
			this->items = items;
			this->type = object_dropdown;
		}

		~dropdown( ) {}

		void think( );
		void draw( ) override;
	};
}

#endif // OXUI_DROPDOWN_HPP