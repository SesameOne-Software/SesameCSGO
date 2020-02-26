#ifndef OXUI_TAB_HPP
#define OXUI_TAB_HPP

#include <memory>
#include <vector>
#include "object.hpp"
#include "../types/types.hpp"
#include "groupbox.hpp"

namespace oxui {
	/*
	*	INFO: Container for control objects.
	*
	*	USAGE: const auto main_panel = std::make_shared< oxui::panel >( oxui::rect( 0, 0, screen.w, screen.h ) );
	*/
	class tab : public obj {
		dividers divider;

	public:
		std::vector< std::shared_ptr< obj > > objects;
		bool selected = false;
		str title;

		tab( const str& title ) {
			this->title = title;
			type = object_tab;
		}
		~tab( ) {}

		void add_columns( int columns ) {
			divider.columns_per_row.push_back( columns );
		}

		void add_element( const std::shared_ptr< obj >& new_obj ) {
			new_obj->parent = this;
			objects.push_back( new_obj );
		}

		void add_group( const std::shared_ptr< group >& new_group ) {
			add_element( new_group );
		}

		void draw( ) override;
	};
}

#endif // OXUI_TAB_HPP