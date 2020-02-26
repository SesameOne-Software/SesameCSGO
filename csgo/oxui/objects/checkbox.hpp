#ifndef OXUI_CHECKBOX_HPP
#define OXUI_CHECKBOX_HPP

#include <memory>
#include <vector>
#include "object.hpp"
#include "../types/types.hpp"

namespace oxui {
	class checkbox : public obj {
	public:
		str label;
		double checked_time = 0.0;
		bool checked = false;

		checkbox( const str& label ) {
			this->label = label;
			type = object_checkbox;
		}

		~checkbox( ) {}

		void think( );
		void draw( ) override;
	};
}

#endif // OXUI_CHECKBOX_HPP