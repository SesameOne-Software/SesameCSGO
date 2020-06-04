#ifndef OXUI_LABEL_HPP
#define OXUI_LABEL_HPP

#include <memory>
#include <vector>
#include <functional>
#include "object.hpp"
#include "../types/types.hpp"

namespace oxui {
	class label : public obj {
	public:
		bool left_side = false;
		str text;

		label ( const str& text, bool left_side = false ) {
			this->text = text;
			this->left_side = left_side;
			type = object_label;
		}

		~label ( ) {}

		void think( );
		void draw( ) override;
	};
}

#endif // OXUI_LABEL_HPP