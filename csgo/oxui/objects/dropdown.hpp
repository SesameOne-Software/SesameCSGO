#ifndef OXUI_DROPDOWN_HPP
#define OXUI_DROPDOWN_HPP

#include <memory>
#include <vector>
#include <functional>
#include "object.hpp"
#include "../types/types.hpp"

namespace oxui {
	class dropdown : public obj {
	public:
		str label;
		std::vector< str > items;

		dropdown( const str& label, const std::vector< str >& items ) {
			this->label = label;
			this->items = items;
			type = object_dropdown;
		}

		~dropdown( ) {}

		void think( );
		void draw( ) override;
	};
}

#endif // OXUI_DROPDOWN_HPP