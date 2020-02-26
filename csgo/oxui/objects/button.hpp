#ifndef OXUI_BUTTON_HPP
#define OXUI_BUTTON_HPP

#include <memory>
#include <vector>
#include <functional>
#include "object.hpp"
#include "../types/types.hpp"

namespace oxui {
	class button : public obj {
	public:
		str label;
		std::function< void( ) > callback;

		button( const str& label, std::function< void( ) > callback ) {
			this->label = label;
			this->callback = callback;
			type = object_button;
		}

		~button( ) {}

		void think( );
		void draw( ) override;
	};
}

#endif // OXUI_BUTTON_HPP