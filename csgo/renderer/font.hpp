#pragma once
#include <d3d9.h>
#include <d3dx9.h>
#include <string>
#include <map>
#include <optional>
#include <memory>

#define STB_TRUETYPE_IMPLEMENTATION
#include "../stb/stb_truetype.h"

namespace truetype {
	enum class text_flags_t {
		text_flags_none = 0,
		text_flags_dropshadow,
		text_flags_outline
	};

	struct font {
		std::string font_name;
		float size;
		stbtt_fontinfo font_info;

		void set_font_size ( float x );
		void text_size ( const std::string& string, float& x_out, float& y_out );
		IDirect3DTexture9* create_texture ( const std::string& string );
		void draw_text ( float x, float y, const std::string& string, uint32_t color, text_flags_t flags );
	};

	std::optional<font> create_font ( const uint8_t* font_data, std::string_view font_name, float size );
	void begin ( );
	void end ( );
} // namespace truetype