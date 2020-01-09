#pragma once
#include <windows.h>
#include <memory>
#include <vector>
#include <map>
#include <functional>
#include <string_view>

namespace oxui {
	/* structs, classes, types */

	class point {
	public:
		int x, y;

		point( ) {
			x = y = 0;
		}

		point( int _x, int _y ) {
			x = _x;
			y = _y;
		}

		~point( ) { }
	};

	class dimension {
	public:
		int w, h;

		dimension( int _w, int _h ) {
			w = _w;
			h = _h;
		}

		dimension( ) {
			w = h = 0;
		}

		~dimension( ) { }
	};

	class rect : public point, public dimension {
	public:
		rect( ) {
			x = y = w = h = 0;
		}

		rect( int _x, int _y, int _w, int _h ) {
			x = _x;
			y = _y;
			w = _w;
			h = _h;
		}

		~rect( ) { }
	};

	struct color {
		int r, g, b, a;
	};

	using font = void*;

	enum class window_flags {
		none = std::uint32_t( 1 << 0 ),
		no_title = std::uint32_t( 1 << 1 ),
		no_border = std::uint32_t( 1 << 2 ),
		no_mouse = std::uint32_t( 1 << 3 )
	};

	/* important keys */
	enum class keys {
		mouse1 = VK_LBUTTON,
		insert = VK_INSERT
	};

	enum class option_type {
		boolean,
		integer,
		floating,
		color,
		string
	};

	/* menu style */
	namespace style {
		constexpr auto obj_offset_dist = 22;
		constexpr auto animation_transition_time = 0.2f;

		extern std::map< std::string_view, color > colors;
	}

	/* gui variables */
	namespace vars {
		union option_val {
			bool b;
			int i;
			float f;
			color c;
			char s [ 256 ];
		};

		struct option {
			float tab_anim_start_time = 0.0f;
			int scrolled_amount;
			option_type type = option_type::boolean;
			option_val val;
		};

		extern float scroll_amount;
		extern bool opening_dropdown;
		extern point dropdown_point;
		extern bool disable_menu_input;
		extern int* selected_dropdown_item_ptr;
		extern std::vector< std::string_view > selected_dropdown_items;
		extern int tab;
		extern int cur_tab_num;
		extern int iter_tab_num;
		extern int clicked_tab;
		extern float tab_anim_start_time;
		extern float anim_time;
		extern int tab_count;
		extern rect selected_tab_area;
		extern point next_tab_pos;
		extern point click_start;
		extern point window_start;
		extern rect tab_window;
		extern point click_delta;
		extern point next_item_pos;
		extern dimension last_categories_dim;
		extern dimension last_menu_dim;
		extern point pos;
		extern bool open_pressed;
		extern bool mouse1_pressed;
		extern bool clicked;
		extern bool open;
		extern std::map< std::string_view, font > fonts;
		extern std::map< std::string_view, option > items;
	}

	/* gui implementation functions */
	namespace impl {
		using create_font_func = std::function< font( const std::string_view&, int, bool ) >;
		using destroy_font_func = std::function< void( font ) >;
		using draw_text_func = std::function< void( point, color, font, const std::string_view&, bool ) >;
		using draw_rect_func = std::function< void( rect, color ) >;
		using draw_filled_rect_func = std::function< void( rect, color ) >;
		using text_scale_func = std::function< dimension( font, const std::string_view& ) >;
		using key_pressed_func = std::function< bool( keys ) >;
		using curser_pos_func = std::function< point( ) >;
		using clip_func = std::function< void( rect ) >;
		using draw_circle_func = std::function< void( point, int, int, color ) >;
		using remove_clip_func = std::function< void( ) >;
		using draw_line_func = std::function< void( point, point, color ) >;

		extern create_font_func			create_font;
		extern destroy_font_func		destroy_font;
		extern draw_text_func			draw_text;
		extern draw_rect_func			draw_rect;
		extern draw_filled_rect_func	draw_filled_rect;
		extern text_scale_func			text_scale;
		extern key_pressed_func			key_pressed;
		extern curser_pos_func			cursor_pos;
		extern clip_func				clip;
		extern draw_circle_func			draw_circle;
		extern remove_clip_func			remove_clip;
		extern draw_line_func			draw_line;
	}

	/* gui functions */
	extern bool combo_button( const std::string_view& title, bool opened = false, bool selected = false, dimension override_size = dimension( 125, 20 ) );
	extern bool clicking( rect rect );
	extern bool hovering( rect rect );
	extern void set_cursor_pos( point new_pos );
	extern void destroy( );
	extern void update_anims( float time );
	extern bool start_control( const std::string_view& id );
	extern void end_control( );
	extern bool begin( const dimension area, const std::string_view& title, const window_flags flags );
	extern bool begin_tabs( int tab_count );
	extern bool tab( const std::string_view& title, bool selected = false, color color = style::colors [ "tab" ] );
	extern void end_tabs( );
	extern bool begin_category( const std::string_view& title, dimension dimensions );
	extern void end_category( );
	extern bool button( const std::string_view& title, bool extra_space = true );
	extern void checkbox( const std::string_view& title, const std::string_view& id );
	extern void slider( const std::string_view& title, const std::string_view& id, float min, float max, bool integer_slider = false );
	extern void dropdown( const std::string_view& title, const std::string_view& id, const std::vector< std::string_view > items );
	extern void listbox( const std::string_view& title, const std::string_view& id, const std::vector< std::string_view > items, dimension size = dimension( 100, 200 ) );
	extern void end( );
}