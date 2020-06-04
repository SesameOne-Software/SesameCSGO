#ifndef OXUI_SUBTAB_HPP
#define OXUI_SUBTAB_HPP

#include <memory>
#include <vector>
#include "object.hpp"
#include "../types/types.hpp"

namespace oxui {
	class subtab : public obj {
		double scroll_offset = 0.0;
		int max_height = 0;

	public:
		std::vector< std::shared_ptr< obj > > objects;
		bool selected = false;
		str title;

		subtab ( const str& title ) {
			this->area = area;
			this->title = title;
			type = object_subtab;
		}

		~subtab ( ) {}

		void add_element( const std::shared_ptr< obj >& new_obj ) {
			new_obj->parent = this;
			objects.push_back( new_obj );
		}

		void think( );
		void draw( ) override;
	};
}

#endif // OXUI_GROUPBOX_HPP