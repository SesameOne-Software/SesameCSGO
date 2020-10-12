#include <shlobj_core.h>
#include <vector>

#include "../sdk/sdk.hpp"
#include "font.hpp"

#include "../renderer/d3d9.hpp"

#include <codecvt>

constexpr auto bitmap_width = 1024;
constexpr auto bitmap_height = 1024;

//std::map<uint32_t/*font_id*/, IDirect3DTexture9*/*tex_ptr*/> font_textures {};
std::map<uint32_t/*font_id*/, ID3DXFont*/*tex_ptr*/> font_textures {};

__forceinline uint32_t cp_len ( uint8_t* str, uint8_t* last ) {
	int cplen = 1;

	if ( ( str [ 0 ] & 0xf8 ) == 0xf0 )
		cplen = 4;
	else if ( ( str [ 0 ] & 0xf0 ) == 0xe0 )
		cplen = 3;
	else if ( ( str [ 0 ] & 0xe0 ) == 0xc0 )
		cplen = 2;

	if ( str + cplen > last )
		cplen = 1;

	return cplen;
}

void truetype::font::set_font_size ( float x ) {
	size = x;
}

std::optional<truetype::font> truetype::create_font ( const uint8_t* font_data, std::string_view font_name, float size, bool extended_range ) {
	static uint32_t font_counter = 0;

	font_counter++;

	/* TEMPORARY UNTIL FIX */
	return font { font_counter, font_name.data ( ), size, {}, {}, nullptr, nullptr };
	
	stbtt_fontinfo font_info;
	
	if ( stbtt_InitFont ( &font_info, font_data, 0 ) ) {
		const auto bitmap = new uint8_t [ bitmap_width * bitmap_height ];
		
		std::vector<std::pair<uint16_t, uint16_t>> char_ranges;

		/* CATEGORIES */
		/* latin + latin suppliment */
		char_ranges.push_back ( { 0x0020, 0x00FF } );
		/* latin extended a + b */
		char_ranges.push_back ( { 0x0100, 0x024F } );

		if ( extended_range ) {
			/* cyrillic */
			char_ranges.push_back ( { 0x0400, 0x052F } );
			char_ranges.push_back ( { 0x2DE0, 0x2DFF } );
			char_ranges.push_back ( { 0xA640, 0xA69F } );
			/* thai */
			char_ranges.push_back ( { 0x0E00, 0x0E7F } );
			/* punctuations */
			char_ranges.push_back ( { 0x2010, 0x205E } );
			/* cjk symbols, punctuation, hiragana, and katakana */
			char_ranges.push_back ( { 0x3000, 0x30FF } );
			/* katakana phonetic extensions */
			char_ranges.push_back ( { 0x31F0, 0x31FF } );
			/* cjk ideograms */
			char_ranges.push_back ( { 0x4e00, 0x9FAF } );
			/* half-width characters */
			char_ranges.push_back ( { 0xFF00, 0xFFEF } );
		}
		
		stbtt_pack_context pc;
		stbtt_PackBegin ( &pc, bitmap, bitmap_width, bitmap_height, 0, 1, nullptr );
		
		stbtt_PackSetOversampling ( &pc, 1, 1 );
		
		const auto chars = new stbtt_packedchar [ 0xFFFF ];
		const auto pr = new stbtt_pack_range [ char_ranges.size() ];
		
		for ( auto i = 0; i < char_ranges.size ( ); i++ ) {
			pr [ i ].chardata_for_range = chars + char_ranges [ i ].first;
			pr [ i ].array_of_unicode_codepoints = nullptr;
			pr [ i ].first_unicode_codepoint_in_range = char_ranges [ i ].first;
			pr [ i ].num_chars = char_ranges [ i ].second- char_ranges [ i ].first;
			pr [ i ].font_size = size;
		}
		
		stbtt_PackFontRanges ( &pc, font_data, 0, pr, char_ranges.size ( ) );
		
		delete [ ] pr;

		stbtt_PackEnd ( &pc );

		return font { ++font_counter, font_name.data ( ), size, font_info, pc, chars, bitmap };
	}

	return std::nullopt;
}

void truetype::font::text_size ( const std::string& string, float& x_out, float& y_out ) {
	/* TEMPORARY UNTIL FIX */
	const auto texture = font_textures.find ( id );

	if ( texture == font_textures.end ( ) )
		render::create_font ( reinterpret_cast< void** >( &font_textures [ id ] ), string, size, false );

	render::dim dim_out;
	render::text_size ( font_textures [ id ], string, dim_out );

	x_out = dim_out.w;
	y_out = dim_out.h;

	return;

	x_out = 0.0f;
	y_out = 0.0f;
	
	for ( auto i = 0; i < string.size ( ); ++i ) {
		stbtt_aligned_quad quad;
		stbtt_GetPackedQuad ( chars, bitmap_width, bitmap_height, string [ i ], &x_out, &y_out, &quad, 0 );
	}
}

void truetype::font::text_size ( const std::wstring& string, float& x_out, float& y_out ) {
	/* TEMPORARY UNTIL FIX */
	const auto texture = font_textures.find ( id );

	if ( texture == font_textures.end ( ) )
		render::create_font ( reinterpret_cast< void** >( &font_textures [ id ] ), string, size, false );

	render::dim dim_out;
	render::text_size ( font_textures [ id ], string, dim_out );

	x_out = dim_out.w;
	y_out = dim_out.h;
}

void truetype::font::draw_text ( float x, float y, const std::wstring& string, uint32_t color, text_flags_t flags ) {
	/* TEMPORARY UNTIL FIX */
	const auto texture = font_textures.find ( id );

	if ( texture == font_textures.end ( ) )
		render::create_font ( reinterpret_cast< void** >( &font_textures [ id ] ), string, size, false );

	render::text ( x, y, color, font_textures [ id ], string, flags == text_flags_t::text_flags_dropshadow, flags == text_flags_t::text_flags_outline );
}

void truetype::font::draw_text ( float x, float y, const std::string& string, uint32_t color, text_flags_t flags ) {
	/* TEMPORARY UNTIL FIX */
	const auto texture = font_textures.find ( id );
	
	if ( texture == font_textures.end ( ) )
		render::create_font ( reinterpret_cast<void**>( &font_textures [ id ] ), string, size, false );
	
	render::text ( x, y, color, font_textures [ id ], string, flags == text_flags_t::text_flags_dropshadow, flags == text_flags_t::text_flags_outline );

	return;

	////////////////////////////////////////////////////////////////

	/* create texture if it doesnt exist already */
	//const auto texture = font_textures.find ( id );
	
	//if ( texture == font_textures.end ( ) )
	//	D3DXCreateTexture ( csgo::i::dev, bitmap_width, bitmap_height, 1, D3DUSAGE_DYNAMIC, D3DFMT_A8, D3DPOOL_DEFAULT, &font_textures[id] );

	struct font_vertex_t {
		float x, y, z, rhw;
		uint32_t color;
		float tx, ty;
	};

	csgo::i::dev->SetFVF ( D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_TEX1 );
	csgo::i::dev->SetStreamSource ( 0, nullptr, 0, sizeof ( font_vertex_t ) );

	csgo::i::dev->SetTextureStageState ( 0, D3DTSS_COLOROP, D3DTOP_SELECTARG1 );
	csgo::i::dev->SetTextureStageState ( 0, D3DTSS_COLORARG1, D3DTA_CURRENT );
	csgo::i::dev->SetTextureStageState ( 0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1 );
	csgo::i::dev->SetTextureStageState ( 0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE );

	csgo::i::dev->SetTextureStageState ( 1, D3DTSS_COLOROP, D3DTOP_MODULATE );
	csgo::i::dev->SetTextureStageState ( 1, D3DTSS_COLORARG1, D3DTA_CURRENT );
	csgo::i::dev->SetTextureStageState ( 1, D3DTSS_COLORARG2, D3DTA_TEXTURE );
	
	//csgo::i::dev->SetTexture ( 0, font_textures [ id ] );
	
	IDirect3DVertexBuffer9* vertex_buffer = nullptr;
	csgo::i::dev->CreateVertexBuffer ( sizeof ( font_vertex_t ) * 12, 0, D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_TEX1, D3DPOOL_DEFAULT, &vertex_buffer, nullptr );

	float qx = 0.0f;
	float qy = 0.0f;

	for ( auto i = 0; i < string.size ( ); ++i ) {
		stbtt_aligned_quad quad;
		stbtt_GetPackedQuad ( chars, bitmap_width, bitmap_height, string [ i ], &qx, &qy, &quad, 0 );
		
		font_vertex_t* vertices = nullptr;

		const auto add_vertices = [ & ] ( float xpos, float ypos, uint32_t clr ) {
			vertex_buffer->Lock ( 0, 0, ( void** ) &vertices, 0 );
			
			*vertices++ = { xpos, ypos + quad.y1 - quad.y0, 1.0f, 1.0f, clr, 0.0f, 1.0f };
			*vertices++ = { xpos + quad.x1 - quad.x0, ypos + quad.y1 - quad.y0, 1.0f, 1.0f, clr, 1.0f, 1.0f };
			*vertices++ = { xpos, ypos, 1.0f, 1.0f, clr, 0.0f, 0.0f };

			*vertices++ = { xpos + quad.x1 - quad.x0, ypos + quad.y1 - quad.y0, 1.0f, 1.0f, clr, 1.0f, 1.0f };
			*vertices++ = { xpos + quad.x1 - quad.x0, ypos, 1.0f, 1.0f, clr, 1.0f, 0.0f };
			*vertices++ = { xpos, ypos, 1.0f, 1.0f, clr, 0.0f, 0.0f };

			vertex_buffer->Unlock ( );

			csgo::i::dev->DrawPrimitive ( D3DPT_TRIANGLELIST, 0, 2 );
		};
		
		switch ( flags ) {
		case text_flags_t::text_flags_none: {

		}break;
		case text_flags_t::text_flags_dropshadow: {
			auto shadow_offset = ( quad.y1 - quad.y0 ) * 0.035f;
			shadow_offset = shadow_offset < 1.1f ? 1.0f : shadow_offset;
			add_vertices ( x + shadow_offset, y + shadow_offset, D3DCOLOR_RGBA ( 0, 0, 0, color >> 24 ) );
		} break;
		case text_flags_t::text_flags_outline: {
			auto outline_offset = ( quad.y1 - quad.y0 ) * 0.035f;
			outline_offset = outline_offset < 1.1f ? 1.0f : outline_offset;
			add_vertices ( x + outline_offset, y, D3DCOLOR_RGBA ( 0, 0, 0, color >> 24 ) );
			add_vertices ( x, y + outline_offset, D3DCOLOR_RGBA ( 0, 0, 0, color >> 24 ) );
			add_vertices ( x, y - outline_offset, D3DCOLOR_RGBA ( 0, 0, 0, color >> 24 ) );
			add_vertices ( x - outline_offset, y, D3DCOLOR_RGBA ( 0, 0, 0, color >> 24 ) );
		} break;
		}

		add_vertices ( x, y, color );
	}

	vertex_buffer->Release ( );
	vertex_buffer = nullptr;

	csgo::i::dev->SetTexture ( 0, nullptr );
}

void truetype::font::draw_atlas ( float x, float y ) {
	IDirect3DTexture9* texture = nullptr;

	if ( D3DXCreateTexture ( csgo::i::dev, bitmap_width, bitmap_height, 1, D3DUSAGE_DYNAMIC, D3DFMT_A8/*D3DFMT_L8*/, D3DPOOL_DEFAULT, &texture ) < 0 ) {
		return;
	}

	D3DLOCKED_RECT rect;

	if ( !texture ) {
		return;
	}

	if ( texture->LockRect ( 0, &rect, nullptr, 0 ) < 0 ) {
		texture->Release ( );
		return;
	}

	memcpy ( rect.pBits, font_map, bitmap_width * bitmap_height * sizeof ( uint8_t ) );

	if ( texture ) {
		texture->UnlockRect ( 0 );
		texture->Release ( );
		texture = nullptr;
	}
}

void truetype::begin ( ) {

}

void truetype::end ( ) {
	if ( !font_textures.empty() ) {
		for ( auto& texture : font_textures ) {
			if ( texture.second ) {
				texture.second->Release ( );
				texture.second = nullptr;
			}
		}
		
		font_textures.clear ( );
	}
}