#ifndef OXUI_OBJ_HPP
#define OXUI_OBJ_HPP

#include <memory>
#include <vector>
#include "../types/types.hpp"
#include "../bindings/bindings.hpp"
#include "../themes/purple.hpp"

namespace oxui {
	enum object_type {
		object_panel,
		object_window,
		object_tab,
		object_group,
		object_checkbox,
		object_slider,
		object_button,
		object_dropdown
	};

	/*
	*	INFO: Base object class. All objects inherit the base object properties.
	*/
	class obj {
	public:
		obj* parent = nullptr;
		double time = 0.0;
		double start_hover_time = 0.0;
		double start_click_time = 0.0;
		double fade_timer = 0.0;
		double fading_animation_timer = 0.0;
		double end_hover_time = 0.0;
		rect area = rect( );
		object_type type;

		template < typename type >
		type& find_parent( object_type type_to_find ) {
			auto parent_iter = this;

			try {
				while ( parent_iter->type != type_to_find )
					parent_iter = parent_iter->parent;
			}
			catch ( std::exception& ) {

			}

			return *static_cast< type* >( parent_iter );
		}

		void animate( const rect& area );

		bool hovering( ) {
			pos mouse_pos;
			binds::mouse_pos( mouse_pos );
			return mouse_pos.x >= area.x && mouse_pos.y >= area.y && mouse_pos.x <= area.x + area.w && mouse_pos.y <= area.y + area.h;
		}

		bool clicking( ) {
			return hovering( ) && GetKeyState( VK_LBUTTON ) & KF_UP;
		}

		virtual void draw( ) = 0;

		void draw_ex( );
	};
}

#endif // OXUI_OBJ_HPP