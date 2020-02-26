#ifndef OXUI_SLIDER_HPP
#define OXUI_SLIDER_HPP

#include <memory>
#include <vector>
#include "object.hpp"
#include "../types/types.hpp"

namespace oxui {
	class slider : public obj {
	public:
		str label;
		double changed_time = 0.0;
		double old_value = 0.0;
		double value = 0.0, min, max;

		slider( const str& label, const double& value, const double& min, const double& max ) {
			this->label = label;
			this->value = value;
			this->min = min;
			this->max = max;
			type = object_slider;
		}

		~slider( ) {}

		void think( );
		void draw( ) override;
	};
}

#endif // OXUI_SLIDER_HPP