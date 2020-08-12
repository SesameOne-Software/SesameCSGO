#pragma once
#include <cstdint>
#include <string_view>
#include <functional>
#include <d3d9.h>
#include <d3dx9.h>
#include <windows.h>
#include "sesui.hpp"

namespace sesui {
	namespace binds {
		extern float frame_time;

		void draw_texture ( const std::vector< uint8_t >& data, const vec2& pos, const vec2& dim, const vec2& scale, const color& color ) noexcept;
		void create_font ( sesui::font& font, bool force ) noexcept;
		void polygon ( const std::vector< sesui::vec2 >& verticies, const sesui::color& color, bool filled ) noexcept;
		void multicolor_polygon ( const std::vector< sesui::vec2 >& verticies, const std::vector< sesui::color >& colors, bool filled ) noexcept;
		void text ( const sesui::vec2& pos, const sesui::font& font, const sesui::ses_string& text, const sesui::color& color ) noexcept;
		void get_text_size ( const sesui::font& font, const sesui::ses_string& text, sesui::vec2& dim_out ) noexcept;
		float get_frametime ( ) noexcept;
		void begin_clip ( const sesui::rect& region ) noexcept;
		void end_clip ( ) noexcept;
	}
}