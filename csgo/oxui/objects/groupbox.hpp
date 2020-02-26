#ifndef OXUI_GROUPBOX_HPP
#define OXUI_GROUPBOX_HPP

#include <memory>
#include <vector>
#include "object.hpp"
#include "../types/types.hpp"

namespace oxui {
	/*
	*	INFO: Container for control objects.
	*/
	class group : public obj {
	public:
		std::vector< std::shared_ptr< obj > > objects;
		str title;

		group( const str& title ) {
			this->area = area;
			this->title = title;
			type = object_group;
		}

		~group( ) {}

		void add_element( const std::shared_ptr< obj >& new_obj ) {
			new_obj->parent = this;
			objects.push_back( new_obj );
		}

		void think( );
		void draw( ) override;
	};
}

#endif // OXUI_GROUPBOX_HPP