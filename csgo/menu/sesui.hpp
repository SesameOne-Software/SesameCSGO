#pragma once
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>
#include <stdexcept>
#include <algorithm>
#include <cmath>
#include <array>
#include <map>
#include <unordered_map>

#include "../fmt/core.h"

/* do not modify */
#define SESUI_API __declspec( dllexport )

/* do not modify */
#define SESUI_VER "0.5.0"

namespace sesui {
	namespace globals {
		extern float dpi;
		extern float last_dpi;
	}

	inline float scale_dpi( float x ) {
		return x * globals::dpi;
	}

	inline float unscale_dpi( float x ) {
		return x / globals::dpi;
	}

	/* constants */
	constexpr auto pi = 3.14159265358979f;

	/* ----------------------------- */
	/* -- SESUI DEFINITIONS BELOW -- */
	/* ----------------------------- */

	/* 2d vector type used for storing positions */
	struct vec2 {
		float x, y;

		template < typename type >
		vec2( type x, type y ) {
			this->x = static_cast < float > ( x );
			this->y = static_cast < float > ( y );
		}

		vec2( ) {
			this->x = 0.0f;
			this->y = 0.0f;
		}

		vec2 operator+( const vec2& other ) const {
			return vec2( x + other.x, y + other.y );
		}

		vec2 operator-( const vec2& other ) const {
			return vec2( x - other.x, y - other.y );
		}

		vec2& operator=( const vec2& other ) {
			x = other.x;
			y = other.y;
			return *this;
		}
	};

	/* rectangle type used for storing rectanglular shapes */
	struct rect {
		float x, y, w, h;

		template < typename type >
		rect( type x, type y, type w, type h ) {
			this->x = static_cast < float > ( x );
			this->y = static_cast < float > ( y );
			this->w = static_cast < float > ( w );
			this->h = static_cast < float > ( h );
		}

		rect( ) {
			this->x = 0.0f;
			this->y = 0.0f;
			this->w = 0.0f;
			this->h = 0.0f;
		}

		rect& operator=( const rect& other ) {
			x = other.x;
			y = other.y;
			w = other.w;
			h = other.h;
			return *this;
		}
	};

	/* color type capable of storing *most* colors that we would ever need */
	struct color {
		float r, g, b, a;

		color( float r, float g, float b, float a ) {
			this->r = r;
			this->g = g;
			this->b = b;
			this->a = a;
		}

		color( ) {
			this->r = 0.0f;
			this->g = 0.0f;
			this->b = 0.0f;
			this->a = 0.0f;
		}

		color& operator=( const color& other ) {
			r = other.r;
			g = other.g;
			b = other.b;
			a = other.a;
			return *this;
		}

		color lerp( const color& to, float fraction ) {
			const auto clamped_fraction = std::clamp < float >( fraction, 0.0f, 1.0f );

			return color(
				( to.r - r ) * clamped_fraction + r,
				( to.g - g ) * clamped_fraction + g,
				( to.b - b ) * clamped_fraction + b,
				( to.a - a ) * clamped_fraction + a
			);
		}

		color to_rgb( ) {
			const auto h = this->r * 180.0f * 2.0f;
			const auto s = this->g;
			const auto v = this->b;

			auto r = 0.0f, g = 0.0f, b = 0.0f;

			const auto hi = static_cast < int > ( h / 60.0f ) % 6;
			const auto f = ( h / 60.0f ) - hi;
			const auto p = v * ( 1.0f - s );
			const auto q = v * ( 1.0f - s * f );
			const auto t = v * ( 1.0f - s * ( 1.0f - f ) );

			switch ( hi ) {
				case 0: r = v, g = t, b = p; break;
				case 1: r = q, g = v, b = p; break;
				case 2: r = p, g = v, b = t; break;
				case 3: r = p, g = q, b = v; break;
				case 4: r = t, g = p, b = v; break;
				case 5: r = v, g = p, b = q; break;
			}

			return color( r, g, b, this->a );
		}

		color to_hsv( ) {
			const auto r = this->r;
			const auto g = this->g;
			const auto b = this->b;

			auto h = 0.0f, s = 0.0f, v = 0.0f;

			const auto max = std::max < float >( r, std::max < float >( g, b ) );
			const auto min = std::min < float >( r, std::min < float >( g, b ) );

			v = max;

			if ( max == 0.0f ) {
				s = 0.0f;
				h = 0.0f;
			}
			else if ( max - min == 0.0f ) {
				s = 0.0f;
				h = 0.0f;
			}
			else {
				s = ( max - min ) / max;

				if ( max == r ) {
					h = 60.0f * ( ( g - b ) / ( max - min ) ) + 0.0f;
				}
				else if ( max == g ) {
					h = 60.0f * ( ( b - r ) / ( max - min ) ) + 120.0f;
				}
				else {
					h = 60.0f * ( ( r - g ) / ( max - min ) ) + 240.0f;
				}
			}

			if ( h < 0.0f )
				h += 360.0f;

			return color( h / 2.0f / 180.0f, std::clamp< float >( s, 0.01f, 1.0f - 0.01f ), v, this->a );
		}
	};

	/* font type, placeholder pointer to actual font type you will actually use (temporary for now) */
	struct font {
		void* data;
		std::string family;
		int size, weight;
		bool italic;

		font( std::string_view family, int size, int weight, bool italic ) {
			this->family = family;
			this->size = size;
			this->weight = weight;
			this->italic = italic;
		}

		font( ) {

		}
	};

	namespace math {
		template < typename type >
		constexpr type rad2deg( type rad ) {
			return rad * ( 180.0f / pi );
		}

		template < typename type >
		constexpr type deg2rad( type deg ) {
			return deg * ( pi / 180.0f );
		}

		template < typename type >
		constexpr type sin( type x ) {
			return std::sinf( x );
		}

		template < typename type >
		constexpr type cos( type x ) {
			return std::cosf( x );
		}
	}

	enum window_flags : uint32_t {
		none = ( 1 << 0 ),
		no_resize = ( 1 << 1 ),
		no_move = ( 1 << 2 ),
		no_title = ( 1 << 3 ),
		no_closebutton = ( 1 << 4 )
	};

	using layer = uint8_t;

	enum class layer_constants : layer {
		topmost = 0xff
	};

	namespace input {
		extern std::array< bool, 256 > key_state;
		extern std::array< bool, 256 > old_key_state;
		extern vec2 start_click_pos;
		extern vec2 mouse_pos;
		extern bool enabled;
		extern bool queue_enable;
		extern float scroll_amount;

		/* call before any drawing is handled to gather input */
		SESUI_API void enable_input( bool enabled );

		SESUI_API void get_scroll_amount( const float scroll_amount );

		SESUI_API void get_input( std::string_view window );

		SESUI_API bool key_pressed( int key );

		SESUI_API bool key_down( int key );

		SESUI_API bool key_released( int key );

		SESUI_API bool in_clip( const vec2& pos );

		SESUI_API bool mouse_in_region( const rect& bounds, bool force = false );

		SESUI_API bool click_in_region( const rect& bounds, bool force = false );
	}

	namespace globals {
		struct group_ctx_t {
			float calc_height = 0.0f;
			float scroll_amount = 0.0f;
			float scroll_amount_target = 0.0f;
		};

		struct window_ctx_t {
			rect bounds;
			layer layer;
			bool moving;
			bool resizing;
			vec2 click_offset;
			std::array< float, 512 > anim_time;
			std::array< float, 512 > hover_time;
			std::string selected_tooltip;
			std::string tooltip;
			std::vector< vec2 > cursor_stack;
			std::map< std::string, group_ctx_t > group_ctx;
			rect main_area;
			float last_cursor_offset = 0.0f;
			float tooltip_anim_time = 0.0f;
			float tab_width = 0.0f;
			float selected_tab_offset = 0.0f;
			int tab_count;
			int cur_index;
			int cur_tab_index;
			int cur_subtab_index;
			bool same_line = false;
			std::string cur_group;
		};

		extern rect clip;
		extern bool clip_enabled;
		extern std::map< std::string, window_ctx_t > window_ctx;
		extern std::string cur_window;
	}

	struct style_t {
		color window_background = color( 0.153f, 0.161f, 0.239f, 1.0f );
		color window_foreground = color( 0.122f, 0.129f, 0.192f, 1.0f );
		color window_node_desc = color( 0.106f, 0.114f, 0.153f, 1.0f );
		color window_borders = color( 0.35f, 0.36f, 0.38f, 1.0f );
		color window_accent = color( 0.85f, 0.31f, 0.8f, 1.0f );
		color window_accent_borders = color( 0.9f, 0.04f, 0.43f, 1.0f );

		vec2 window_min_size = vec2( 560.0f, 420.0f );

		/* percentage */
		float titlebar_height = 0.1f /* 1/10 */;
		float group_titlebar_height = 0.05f /* 1/20 */;

		vec2 initial_offset = vec2( 20.0f, 20.0f );
		float spacing = 8.0f;
		float padding = 6.0f;
		float resize_grab_radius = 6.0f;
		float same_line_offset = 145.0f;
		float scroll_arrow_height = 12.0f;

		vec2 inline_button_size = vec2( 80.0f, 20.0f );

		float animation_speed = 4.0f;

		/* control colors */
		color control_background = color( 0.153f, 0.161f, 0.239f, 1.0f );
		color control_borders = color( 0.106f, 0.114f, 0.153f, 1.0f );
		color control_text = color( 0.71f, 0.71f, 0.71f, 1.0f );
		color control_text_hovered = color( 1.0f, 1.0f, 1.0f, 1.0f );
		color control_accent = color( 0.85f, 0.31f, 0.8f, 1.0f );
		color control_accent_borders = color( 0.85f + 0.1f, 0.31f + 0.1f, 0.8f + 0.1f, 1.0f );
		color tab_selected = color( 0.9f, 0.04f, 0.43f, 1.0f );

		float rounding = 6.0f;
		float control_rounding = 4.0f;
		float tooltip_hover_time = 1.25f;

		/* control stuff */
		vec2 checkbox_size = vec2( 14.0f, 14.0f );
		vec2 slider_size = vec2( 225.0f, 8.0f );
		vec2 button_size = vec2( 225.0f, 20.0f );
		vec2 color_popup = vec2( 220.0f, 220.0f );
		float color_bar_width = 10.0f;

		font control_font = font( "sesame_ui", 16, 525, false );
		font tab_font = font( "sesame_ui", 24, 260, false );
	};

	extern style_t style;

	namespace fonts {

	}

	enum class side_t : int {
		side_top = 0,
		side_left,
		side_bottom,
		side_right
	};

	/* draw list object, keeps all polygons and text that will need to be drawn by the ui framework */
	class c_draw_list {
		enum class object_type : uint8_t {
			polygon = 0,
			texture,
			polygon_multicolor,
			text,
			clip,
		};

		enum class clip_mode : uint8_t {
			none = 0,
			begin,
			end
		};

		struct object_t {
			/* stores type of objects we are trying to draw (all types stored in object_type enum)*/
			object_type type;

			/* color applies to both polygons and text */
			color color;

			/* clip region */
			rect clip_rect;
			clip_mode clip_mode;

			/* polygon information */
			std::vector< vec2 > verticies;
			bool filled;

			/* text information */
			vec2 text_pos;
			font font;
			std::string text;
			bool text_shadow;

			std::vector< sesui::color > colors;

			std::vector< uint8_t > texture_data;
			vec2 texture_scale;
		};

		/* list of objects to draw*/
		std::array < std::vector< object_t >, static_cast< int > ( layer_constants::topmost ) > objects { };

	public:
		using draw_texture_fn = std::add_pointer_t< void( const std::vector< uint8_t >& data, const vec2& pos, const vec2& dim, const vec2& scale, const color& color ) noexcept >;

		/* mostly geometric primitives */
		using draw_polygon_fn = std::add_pointer_t< void( const std::vector< vec2 >& verticies, const color& color, bool filled ) noexcept >;

		/* mostly geometric primitives */
		using draw_multicolor_polygon_fn = std::add_pointer_t< void( const std::vector< sesui::vec2 >& verticies, const std::vector< sesui::color >& colors, bool filled ) noexcept >;

		/* text rendering */
		using draw_text_fn = std::add_pointer_t< void( const vec2& pos, const font& font, const std::string& text, const color& color ) noexcept >;
		using get_text_size_fn = std::add_pointer_t< void( const font& font, const std::string& text, vec2& dim_out ) noexcept >;

		/* other stuff */
		using get_frametime_fn = std::add_pointer_t< float( ) noexcept >;

		using begin_clip_fn = std::add_pointer_t< void( const rect& region ) noexcept >;
		using end_clip_fn = std::add_pointer_t< void( ) noexcept >;

		/* create font */
		using create_font_fn = std::add_pointer_t< void( font& font, bool force ) noexcept >;

		/* these methods must be defined in order for sesui to render any objects */
		draw_texture_fn draw_texture;
		draw_polygon_fn draw_polygon;
		draw_multicolor_polygon_fn draw_multicolor_polygon;
		draw_text_fn draw_text;
		get_text_size_fn get_text_size;
		get_frametime_fn get_frametime;
		begin_clip_fn begin_clip;
		end_clip_fn end_clip;
		create_font_fn create_font;

		void add_texture( const std::vector< uint8_t >& data, const vec2& pos, const vec2& dim, const vec2& scale, const color& color, bool topmost = false ) {
			const auto scaled_w = scale_dpi( dim.x );
			const auto scaled_h = scale_dpi( dim.y );

			objects [ topmost ? static_cast < int >( layer_constants::topmost ) - 1 : static_cast < int >( globals::window_ctx [ globals::cur_window ].layer ) ].push_back( object_t {
				object_type::texture,
				color,
				rect( pos.x, pos.y, scaled_w, scaled_h ),
				clip_mode::none,
				{},
				true,
				{},
				{},
				"",
				false,
				{},
				data,
				scale
				} );
		}

		void add_rounded_rect( const rect& rectangle, float rad, const color& color, bool filled, bool topmost = false ) {
			const auto scaled_rad = scale_dpi( rad );
			const auto scaled_resolution = static_cast< int > ( scaled_rad * 0.666f + 0.5f );

			const auto scaled_w = scale_dpi( rectangle.w );
			const auto scaled_h = scale_dpi( rectangle.h );

			std::vector< vec2 > verticies( 4 * scaled_resolution );

			for ( auto i = 0; i < 4; i++ ) {
				const auto x = rectangle.x + ( ( i < 2 ) ? ( scaled_w - scaled_rad ) : scaled_rad );
				const auto y = rectangle.y + ( ( i % 3 ) ? ( scaled_h - scaled_rad ) : scaled_rad );
				const auto a = 90.0f * i;

				for ( auto j = 0; j < scaled_resolution; j++ ) {
					const auto a1 = math::deg2rad( a + ( j / ( float )( scaled_resolution - 1 ) ) * 90.0f );

					verticies [ i * scaled_resolution + j ] = { x + scaled_rad * math::sin( a1 ), y - scaled_rad * math::cos( a1 ) };
				}
			}

			objects [ topmost ? static_cast < int >( layer_constants::topmost ) - 1 : static_cast < int >( globals::window_ctx [ globals::cur_window ].layer ) ].push_back( object_t {
				object_type::polygon,
				color,
				{},
				clip_mode::none,
				verticies,
				filled,
				{},
				{},
				"",
				false,
				{},
				{},
				{}
				} );
		}

		void add_half_rounded_rect( const rect& rectangle, float rad, const color& color, bool filled, side_t flat_side, bool topmost = false ) {
			const auto scaled_rad = scale_dpi( rad );
			const auto scaled_resolution = static_cast< int > ( scaled_rad * 0.666f + 0.5f );

			const auto scaled_w = scale_dpi( rectangle.w );
			const auto scaled_h = scale_dpi( rectangle.h );

			std::vector< vec2 > verticies;

			for ( auto i = 0; i < 4; i++ ) {
				const auto x = rectangle.x + ( ( i < 2 ) ? ( scaled_w - scaled_rad ) : scaled_rad );
				const auto y = rectangle.y + ( ( i % 3 ) ? ( scaled_h - scaled_rad ) : scaled_rad );
				const auto a = 90.0f * i;

				if ( flat_side == side_t::side_bottom && ( i == 1 || i == 2 ) ) {
					if ( i == 1 )
						verticies.push_back( { rectangle.x + scaled_w, rectangle.y + scaled_h } );
					else
						verticies.push_back( { rectangle.x, rectangle.y + scaled_h } );

					continue;
				}
				else if ( flat_side == side_t::side_top && ( i == 0 || i == 3 ) ) {
					if ( i == 0 )
						verticies.push_back( { rectangle.x + scaled_w, rectangle.y } );
					else
						verticies.push_back( { rectangle.x, rectangle.y } );

					continue;
				}
				else if ( flat_side == side_t::side_left && ( i == 3 || i == 2 ) ) {
					if ( i == 3 )
						verticies.push_back( { rectangle.x, rectangle.y } );
					else
						verticies.push_back( { rectangle.x, rectangle.y + scaled_h } );

					continue;
				}
				else if ( flat_side == side_t::side_right && ( i == 0 || i == 1 ) ) {
					if ( i == 0 )
						verticies.push_back( { rectangle.x + scaled_w, rectangle.y } );
					else
						verticies.push_back( { rectangle.x + scaled_w, rectangle.y + scaled_h } );

					continue;
				}

				for ( auto j = 0; j < scaled_resolution; j++ ) {
					const auto a1 = math::deg2rad( a + ( j / ( float )( scaled_resolution - 1 ) ) * 90.0f );

					verticies.push_back( { x + scaled_rad * math::sin( a1 ), y - scaled_rad * math::cos( a1 ) } );
				}
			}

			objects [ topmost ? static_cast < int >( layer_constants::topmost ) - 1 : static_cast < int >( globals::window_ctx [ globals::cur_window ].layer ) ].push_back( object_t {
				object_type::polygon,
				color,
				{},
				clip_mode::none,
				verticies,
				filled,
				{},
				{},
				"",
				false,
				{},
				{},
				{}
				} );
		}

		void add_rect( const rect& rectangle, const color& color, bool filled, bool topmost = false ) {
			const auto scaled_w = scale_dpi( rectangle.w );
			const auto scaled_h = scale_dpi( rectangle.h );

			std::vector< vec2 > verticies {
				vec2( rectangle.x, rectangle.y ),
				vec2( rectangle.x + scaled_w, rectangle.y ),
				vec2( rectangle.x + scaled_w, rectangle.y + scaled_h ),
				vec2( rectangle.x, rectangle.y + scaled_h )
			};

			objects [ topmost ? static_cast < int >( layer_constants::topmost ) - 1 : static_cast < int >( globals::window_ctx [ globals::cur_window ].layer ) ].push_back( object_t {
				object_type::polygon,
				color,
				{},
				clip_mode::none,
				verticies,
				filled,
				{},
				{},
				"",
				false,
				{},
				{},
				{}
				} );
		}

		void add_circle( const vec2& pos, float rad, const color& color, bool filled, bool topmost = false ) {
			const auto scaled_rad = scale_dpi( rad );
			const auto scaled_resolution = static_cast< int > ( scaled_rad * 2 + 2 );

			std::vector< vec2 > verticies { };

			for ( auto i = 0; i < scaled_resolution; i++ ) {
				verticies.push_back( vec2(
					pos.x - scaled_rad * std::cosf( pi * ( ( i - 1 ) / ( scaled_resolution / 2.0f ) ) ),
					pos.y - scaled_rad * std::sinf( pi * ( ( i - 1 ) / ( scaled_resolution / 2.0f ) ) )
				) );
			}

			objects [ topmost ? static_cast < int >( layer_constants::topmost ) - 1 : static_cast < int >( globals::window_ctx [ globals::cur_window ].layer ) ].push_back( object_t {
				object_type::polygon,
				color,
				{},
				clip_mode::none,
				verticies,
				filled,
				{},
				{},
				"",
				false,
				{},
				{},
				{}
				} );
		}

		void add_rect_gradient( const rect& rectangle, const color& color_from, const color& color_to, bool horizontal, bool filled, bool topmost = false ) {
			const auto scaled_w = scale_dpi( rectangle.w );
			const auto scaled_h = scale_dpi( rectangle.h );

			std::vector< vec2 > verticies {
				vec2( rectangle.x, rectangle.y ),
				vec2( rectangle.x + scaled_w, rectangle.y ),
				vec2( rectangle.x + scaled_w, rectangle.y + scaled_h ),
				vec2( rectangle.x, rectangle.y + scaled_h )
			};

			std::vector< color > colors {
				color_from,
				horizontal ? color_to : color_from,
				color_to,
				horizontal ? color_from : color_to
			};

			objects [ topmost ? static_cast < int >( layer_constants::topmost ) - 1 : static_cast < int >( globals::window_ctx [ globals::cur_window ].layer ) ].push_back( object_t {
				object_type::polygon_multicolor,
				color_from,
				{},
				clip_mode::none,
				verticies,
				filled,
				{},
				{},
				"",
				false,
				colors,
				{},
				{}
				} );
		}

		void add_rect_multicolor( const rect& rectangle, const color& color_from_top, const color& color_to_top, const color& color_from_bottom, const color& color_to_bottom, bool filled, bool topmost = false ) {
			const auto scaled_w = scale_dpi( rectangle.w );
			const auto scaled_h = scale_dpi( rectangle.h );

			std::vector< vec2 > verticies {
				vec2( rectangle.x, rectangle.y ),
				vec2( rectangle.x + scaled_w, rectangle.y ),
				vec2( rectangle.x + scaled_w, rectangle.y + scaled_h ),
				vec2( rectangle.x, rectangle.y + scaled_h )
			};

			std::vector< color > colors {
				color_from_top,
				color_to_top,
				color_to_bottom,
				color_from_bottom
			};

			objects [ topmost ? static_cast < int >( layer_constants::topmost ) - 1 : static_cast < int >( globals::window_ctx [ globals::cur_window ].layer ) ].push_back( object_t {
				object_type::polygon_multicolor,
				color_from_top,
				{},
				clip_mode::none,
				verticies,
				filled,
				{},
				{},
				"",
				false,
				colors,
				{},
				{}
				} );
		}

		void add_line( const vec2& p1, const vec2& p2, const color& color, bool topmost = false ) {
			std::vector< vec2 > verticies {
				p1,
				p2
			};

			objects [ topmost ? static_cast < int >( layer_constants::topmost ) - 1 : static_cast < int >( globals::window_ctx [ globals::cur_window ].layer ) ].push_back( object_t {
				object_type::polygon,
				color,
				{},
				clip_mode::none,
				verticies,
				false,
				{},
				{},
				"",
				false,
				{},
				{},
				{}
				} );
		}

		void add_arrow( const vec2& pos, float size, float rotation, const color& color, bool filled, bool topmost = false ) {
			const auto scaled_size = scale_dpi( size );

			const auto ang1 = math::deg2rad( -90.0f + rotation );
			const auto ang2 = math::deg2rad( 90.0f + rotation );
			const auto ang3 = math::deg2rad( 0.0f + rotation );

			std::vector< vec2 > verticies;

			if ( !filled ) {
				verticies.push_back( vec2( pos.x - math::cos( ang2 ) * scaled_size, pos.y - math::sin( ang2 ) * scaled_size ) );
				verticies.push_back( vec2( pos.x - math::cos( ang3 ) * scaled_size, pos.y - math::sin( ang3 ) * scaled_size ) );
				verticies.push_back( vec2( pos.x - math::cos( ang1 ) * scaled_size, pos.y - math::sin( ang1 ) * scaled_size ) );
				verticies.push_back( vec2( pos.x - math::cos( ang3 ) * scaled_size, pos.y - math::sin( ang3 ) * scaled_size ) );
			}
			else {
				verticies.push_back( vec2( pos.x - math::cos( ang2 ) * scaled_size, pos.y - math::sin( ang2 ) * scaled_size ) );
				verticies.push_back( vec2( pos.x - math::cos( ang3 ) * scaled_size, pos.y - math::sin( ang3 ) * scaled_size ) );
				verticies.push_back( vec2( pos.x - math::cos( ang1 ) * scaled_size, pos.y - math::sin( ang1 ) * scaled_size ) );
			}

			objects [ topmost ? static_cast < int >( layer_constants::topmost ) - 1 : static_cast < int >( globals::window_ctx [ globals::cur_window ].layer ) ].push_back( object_t {
				object_type::polygon,
				color,
				{},
				clip_mode::none,
				verticies,
				filled,
				{},
				{},
				"",
				false,
				{},
				{},
				{}
				} );
		}

		void add_text( const vec2& pos, const font& font, std::string_view text, bool text_shadow, const color& color, bool topmost = false ) {
			if ( !font.data )
				throw "Attempted to add text using invalid font to draw list.";

			objects [ topmost ? static_cast < int >( layer_constants::topmost ) - 1 : static_cast < int >( globals::window_ctx [ globals::cur_window ].layer ) ].push_back( object_t {
				object_type::text,
				color,
				{},
				clip_mode::none,
				{},
				false,
				pos,
				font,
				text.data(),
				text_shadow,
				{},
				{},
				{}
				} );
		}

		void add_clip( const rect& area, bool topmost = false ) {
			globals::clip_enabled = true;
			globals::clip = area;

			objects [ topmost ? static_cast < int >( layer_constants::topmost ) - 1 : static_cast < int >( globals::window_ctx [ globals::cur_window ].layer ) ].push_back( object_t {
				object_type::clip,
				color( 0,0,0,0 ),
				rect( area.x, area.y, scale_dpi( area.w ), scale_dpi( area.h ) ),
				clip_mode::begin,
				{},
				false,
				{},
				{ },
				"",
				false,
				{},
				{},
				{}
				} );
		}

		void remove_clip( bool topmost = false ) {
			globals::clip_enabled = false;

			objects [ topmost ? static_cast < int >( layer_constants::topmost ) - 1 : static_cast < int >( globals::window_ctx [ globals::cur_window ].layer ) ].push_back( object_t {
				object_type::clip,
				color( 0,0,0,0 ),
				{},
				clip_mode::end,
				{},
				false,
				{},
				{ },
				"",
				false,
				{},
				{},
				{}
				} );
		}

		void render( ) {
			if ( !draw_polygon
				|| !draw_multicolor_polygon
				|| !draw_text
				|| !get_text_size
				|| !get_frametime
				|| !begin_clip
				|| !end_clip
				|| !create_font )
				throw "One or more of the methods required to render the draw list were undefined.";

			/* organized in layer order */
			for ( auto& layer : objects ) {
				if ( layer.empty( ) )
					continue;

				/* loop through all objects, sort by layer, and draw all at once */
				for ( const auto& object : layer ) {
					switch ( object.type ) {
						case object_type::polygon:
							draw_polygon( object.verticies, object.color, object.filled );
							break;
						case object_type::polygon_multicolor:
							draw_multicolor_polygon( object.verticies, object.colors, object.filled );
							break;
						case object_type::text:
							if ( !object.font.data )
								throw "Attempted to draw text using invalid font.";

							if ( object.text_shadow )
								draw_text( object.text_pos + vec2( 1.0f, 1.0f ), object.font, object.text, color( 0.0f, 0.0f, 0.0f, static_cast< int >( object.color.a ) ) );

							draw_text( object.text_pos, object.font, object.text, object.color );
							break;
						case object_type::clip:
							if ( object.clip_mode == clip_mode::begin )
								begin_clip( object.clip_rect );
							else if ( object.clip_mode == clip_mode::end )
								end_clip( );
							break;
						case object_type::texture:
							draw_texture( object.texture_data, vec2( object.clip_rect.x, object.clip_rect.y ), vec2( object.clip_rect.w, object.clip_rect.h ), object.texture_scale, object.color );
							break;
						default:
							break;
					}
				}

				/* clear draw list after we already rendered all the objects! */
				layer.clear( );
			}
		}
	};

	extern c_draw_list draw_list;

	/* ------------------------------- */
	/* -- SESUI API FUNCTIONS BELOW -- */
	/* ------------------------------- */

	/* splits text into 2 parts: 1. label 2. hidden id*/
	std::pair< std::string, std::string > split( std::string val );

	/* begins ui frame */
	SESUI_API void begin_frame( std::string_view window );

	/* ends ui frame */
	SESUI_API void end_frame( );

	/* creates new window */
	SESUI_API bool begin_window( std::string_view name, const rect& bounds, bool& opened, uint32_t flags = window_flags::none );

	SESUI_API void end_window( );

	SESUI_API void same_line( );

	/* groups */
	SESUI_API bool begin_group( std::string_view name, const rect& fraction, const rect& extra );

	SESUI_API void end_group( );

	/* tabs */
	SESUI_API bool begin_tabs( int count, float width = 0.2f );

	SESUI_API void tab( std::string_view name, int& selected );

	SESUI_API void end_tabs( );

	/* other gui stuff */
	SESUI_API void tooltip( std::string_view tooltip );

	/* gui controls */
	SESUI_API void checkbox( std::string_view name, bool& option );

	SESUI_API void text( std::string_view name );

	SESUI_API bool button( std::string_view name );

	SESUI_API void colorpicker( std::string_view name, color& option );

	SESUI_API void combobox( std::string_view name, int& option, const std::vector< std::string_view >& list );

	SESUI_API void textbox( std::string_view name, std::string& option );

	SESUI_API void multiselect( std::string_view name, const std::vector< std::pair< std::string_view, bool& > >& list );

	SESUI_API void slider_ex( std::string_view name, float& option, float min, float max, std::string_view value_str );

	SESUI_API void keybind( std::string_view name, int& key, int& mode );

	template < typename type >
	void slider( std::string_view name, type& option, type min, type max, std::string_view fmt = "" ) {
		std::string formatted;

		if ( !fmt.empty ( ) ) {
			formatted = fmt::format ( fmt, option );
		}
		else {
			if constexpr ( std::is_floating_point< type >::value )
				formatted = fmt::format ( "{:.1f}", option );
			else
				formatted = fmt::format ( "{}", option );
		}

		float tmp = static_cast < float > ( option );
		slider_ex( name, tmp, min, max, formatted );
		option = tmp;
	}

	/* call function after all ui logic to render all ui objects */
	SESUI_API void render( );
}