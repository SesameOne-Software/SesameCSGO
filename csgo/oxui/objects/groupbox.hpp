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
		double scroll_offset = 0.0;
		double scroll_offset_from = 0.0;
		double scroll_offset_target = 0.0;
		double scroll_time = 0.0;
		int max_height = 0;

	public:
		std::vector< std::shared_ptr< obj > > objects;
		bool selected = false;
		str title;
		std::vector< float > fractions;
		bool hide_title;
		bool extend_separator;

		group( const str& title, const std::vector< float >& fractions, bool hide_title = false, bool extend_separator = false ) {
			this->area = area;
			this->title = title;
			this->fractions = fractions;
			this->hide_title = hide_title;
			this->extend_separator = extend_separator;
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