#pragma once
#include <cstdint>
#include <string_view>
#include <functional>

namespace font
{
	struct font
	{
	};

	font *add_font_from_memory(void *ttf_data, int ttf_size, float size_pixels, const ImFontConfig *font_cfg_template, const ImWchar *glyph_ranges);
} // namespace font