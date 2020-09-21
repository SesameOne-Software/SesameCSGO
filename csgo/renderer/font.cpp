#include "font.hpp"

#define STB_TRUETYPE_IMPLEMENTATION
#include "../stb/stb_truetype.h"

std::optional<std::unique_ptr<font::font>> font::add_font_from_memory( const std::vector<uint8_t>& data, std::string_view name, int size ) {
	auto new_font = std::make_unique<font> ( );

	new_font->name = name;
	new_font->size = size;

	const auto ttf_size = data.size ( );

	/* prepare font */
	stbtt_fontinfo info;

	if ( !stbtt_InitFont ( &info, data.data ( ), 0 ) )
		return {};

	int b_w = 512; /* bitmap width */
	int b_h = 128; /* bitmap height */
	int l_h = 64;  /* line height */

	/* create a bitmap for the phrase */
	unsigned char *bitmap = calloc(b_w * b_h, sizeof(unsigned char));

	/* calculate font scaling */
	float scale = stbtt_ScaleForPixelHeight(&info, l_h);

	char *word = "the quick brown fox";

	int x = 0;

	int ascent, descent, lineGap;
	stbtt_GetFontVMetrics(&info, &ascent, &descent, &lineGap);

	ascent = roundf(ascent * scale);
	descent = roundf(descent * scale);

	csgo::i::dev->CreateVertexBuffer ( sizeof ( font_vertex_t ) * 12, NULL, D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_TEX1, D3DPOOL_MANAGED, &vertex_buffer, nullptr );

	for (auto i = 0; i < strlen(word); ++i) {
		/* get bounding box for character (may be offset to account for chars that dip above or below the line */
		int c_x1, c_y1, c_x2, c_y2;
		stbtt_GetCodepointBitmapBox ( &info, word [ i ], scale, scale, &c_x1, &c_y1, &c_x2, &c_y2 );

		/* compute y (different characters have different heights */
		int y = ascent + c_y1;

		/* render character (stride and offset is important here) */
		int byteOffset = x + ( y * b_w );
		stbtt_MakeCodepointBitmap ( &info, bitmap + byteOffset, c_x2 - c_x1, c_y2 - c_y1, b_w, scale, scale, word [ i ] );

		/* how wide is this character */
		int ax;
		stbtt_GetCodepointHMetrics ( &info, word [ i ], &ax, 0 );
		x += ax * scale;

		/* add kerning */
		int kern;
		kern = stbtt_GetCodepointKernAdvance ( &info, word [ i ], word [ i + 1 ] );
		x += kern * scale;
	}

	/* save out a 1 channel image */
	stbi_write_png("out.png", b_w, b_h, 1, bitmap, b_w);

	free(bitmap);

	return new_font;
}