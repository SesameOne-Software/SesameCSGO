#pragma once
#include <d3d9.h>
#include <d3dx9.h>
#include <string>
#include <map>
#include <optional>
#include <memory>

namespace truetype
{
	enum text_flags_t
	{
		text_flags_none = 0,
		text_flags_dropshadow,
		text_flags_outline
	};

	struct font_vec_t
	{
		float x, y;
	};

	struct font
	{
		std::string font_name;
		float size;
		stbtt_fontinfo font_info;

		IDirect3DTexture9 *create_texture(const std::string &string);
		void draw_text(float x, float y, const std::string &string, uint32_t color, text_flags_t flags);
	};

	std::optional<font> create_font(const std::vector<uint8_t> &font_data, std::string_view font_name, float size);
	void begin();
	void end();
} // namespace truetype