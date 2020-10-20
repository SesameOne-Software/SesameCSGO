#include <shlobj_core.h>
#include <vector>

#include "truetype.hpp"
#include "utf8.h"

#include <sdk.hpp>

std::map<uint32_t/*font_id*/, IDirect3DTexture9* /*tex_ptr*/> font_textures{};

IDirect3DVertexBuffer9* g_vertex_buffers = nullptr;
ID3DXSprite* g_sprite = nullptr;

struct font_vertex_t {
	float x, y, z, rhw;
	uint32_t color;
	float tx, ty;
};

void truetype::font::set_font_size( float x ) {
	size = x;
}

int round_up_multiple( double x, double multiple ) {
	return round( x / multiple + 0.5 ) * multiple;
}

std::optional<truetype::font> truetype::create_font( const uint8_t* font_data, std::string_view font_name, float size, bool extended_range ) {
	static uint32_t font_counter = 0;

	font_counter++;

	stbtt_fontinfo font_info;

	if ( stbtt_InitFont( &font_info, font_data, 0 ) ) {
		std::vector<std::pair<uint16_t, uint16_t>> char_ranges;

		/* CATEGORIES */
		/* latin + latin suppliment */
		char_ranges.push_back( { 0x0020, 0x00FF } );
		/* latin extended a + b */
		char_ranges.push_back( { 0x0100, 0x024F } );

		if ( extended_range ) {
			/* cyrillic */
			char_ranges.push_back( { 0x0400, 0x052F } );
			char_ranges.push_back( { 0x2DE0, 0x2DFF } );
			char_ranges.push_back( { 0xA640, 0xA69F } );
			/* thai */
			char_ranges.push_back( { 0x0E00, 0x0E7F } );
			/* punctuations */
			char_ranges.push_back( { 0x2010, 0x205E } );
			/* cjk symbols, punctuation, hiragana, and katakana */
			char_ranges.push_back( { 0x3000, 0x30FF } );
			/* katakana phonetic extensions */
			char_ranges.push_back( { 0x31F0, 0x31FF } );
			/* cjk ideograms */
			char_ranges.push_back( { 0x4E00, 0x9FAF } );
			/* half-width characters */
			char_ranges.push_back( { 0xFF00, 0xFFEF } );
			/* korean alphabet */
			char_ranges.push_back( { 0x3131, 0x3163 } );
			/* korean charcaters */
			char_ranges.push_back( { 0xAC00, 0xD7A3 } );
		}

		/* try to save some space and create a bitmap with the maximum size we would need */
		unsigned int char_count = 0;

		for ( auto i = 0; i < char_ranges.size( ); i++ )
			for ( auto j = char_ranges[i].first; j <= char_ranges[i].second; j++ )
				if ( !stbtt_IsGlyphEmpty( &font_info, j ) )
					char_count++;

		unsigned int bitmap_width = round_up_multiple( sqrt( char_count ) * size, 4096 );
		unsigned int bitmap_height = bitmap_width;

		const auto bitmap = new uint8_t[bitmap_width * bitmap_height];

		stbtt_pack_context pc;
		stbtt_PackBegin( &pc, bitmap, bitmap_width, bitmap_height, 0, 1, nullptr );

		stbtt_PackSetOversampling( &pc, 1, 1 );

		const auto chars = new stbtt_packedchar[0xFFFF];
		const auto pr = new stbtt_pack_range[char_ranges.size( )];

		for ( auto i = 0; i < char_ranges.size( ); i++ ) {
			pr[i].chardata_for_range = chars + char_ranges[i].first;
			pr[i].array_of_unicode_codepoints = nullptr;
			pr[i].first_unicode_codepoint_in_range = char_ranges[i].first;
			pr[i].num_chars = char_ranges[i].second - char_ranges[i].first;
			pr[i].font_size = size;
		}

		stbtt_PackFontRanges( &pc, font_data, 0, pr, char_ranges.size( ) );

		delete [ ] pr;

		stbtt_PackEnd( &pc );

		return font{ font_counter, font_name.data( ), size, font_info, pc, chars, bitmap, bitmap_width, bitmap_height };
	}

	return std::nullopt;
}

void truetype::font::text_size( const std::string& string, float& x_out, float& y_out ) {
	x_out = 0.0f;
	y_out = 0.0f;

	std::string max_size_char = "M";

	stbtt_aligned_quad max_quad { };
	float max_qx = 0.0f, max_qy = 0.0f;
	stbtt_GetPackedQuad ( chars, bitmap_width, bitmap_height, max_size_char [ 0 ], &max_qx, &max_qy, &max_quad, 0 );

	y_out = ( max_quad.y1 - max_quad.y0 );

	for ( size_t i = 0; i < string.size( ); ) {
		utf8_int32_t cp = 0;

		/* check for invalid codepoints */
		if ( !utf8codepoint( string.data( ) + i, &cp ) )
			break;

		stbtt_aligned_quad quad;
		stbtt_GetPackedQuad( chars, bitmap_width, bitmap_height, string[i], &x_out, &y_out, &quad, 0 );

		i += utf8codepointsize( cp );
	}
}

void truetype::font::create_texture( ) {
	const auto texture = font_textures.find( id );

	if ( texture == font_textures.end( ) ) {
		IDirect3DTexture9* texture_d3d9 = nullptr;

		if ( D3DXCreateTexture( csgo::i::dev, bitmap_width, bitmap_height, 1, D3DUSAGE_DYNAMIC, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &texture_d3d9 ) < 0 )
			return;

		D3DLOCKED_RECT rect;

		if ( texture_d3d9->LockRect( 0, &rect, nullptr, 0 ) < 0 )
			return;

		auto expanded = new uint32_t[bitmap_width * bitmap_height]{ 0 };

		for ( int y = 0; y < bitmap_width; y++ ) {
			for ( int cx = 0; cx < bitmap_height; cx++ ) {
				const auto index = (y * bitmap_width) + cx;
				const auto value = ( uint32_t )font_map[index];

				expanded[index] = value << 24;
				expanded[index] |= 0x00FFFFFF;
			}
		}

		memcpy( rect.pBits, expanded, bitmap_width * bitmap_height * sizeof( uint32_t ) );

		delete [ ] expanded;

		texture_d3d9->UnlockRect( 0 );
		font_textures[id] = texture_d3d9;
	}
}

void truetype::font::draw_text( float x, float y, const std::string& string, uint32_t color, text_flags_t flags ) {
	create_texture( );

	if ( !font_textures[id] )
		return;

	std::string max_size_char = "M";

	stbtt_aligned_quad max_quad{ };
	float max_qx = 0.0f, max_qy = 0.0f;
	stbtt_GetPackedQuad( chars, bitmap_width, bitmap_height, max_size_char[0], &max_qx, &max_qy, &max_quad, 0 );

	y += (max_quad.y1 - max_quad.y0);

	for ( size_t i = 0; i < string.size( ); ) {
		utf8_int32_t cp = 0;

		/* check for invalid codepoints */
		if ( !utf8codepoint( string.data( ) + i, &cp ) )
			break;

		float qx = x;
		float qy = y;

		stbtt_aligned_quad quad;
		stbtt_GetPackedQuad( chars, bitmap_width, bitmap_height, cp, &qx, &qy, &quad, true );

		font_vertex_t* vertices = nullptr;

		csgo::i::dev->SetFVF( D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_TEX1 );
		csgo::i::dev->SetStreamSource( 0, g_vertex_buffers, 0, sizeof( font_vertex_t ) );
		csgo::i::dev->SetTextureStageState( 0, D3DTSS_COLOROP, D3DTOP_SELECTARG2 );
		csgo::i::dev->SetTexture( 0, font_textures[id] );

		static auto add_vertices = [ & ]( float xpos, float ypos, uint32_t clr ) {
			g_vertex_buffers->Lock( 0, 6 * sizeof ( font_vertex_t ), ( void** )&vertices, D3DLOCK_DISCARD );

			*vertices++ = { quad.x0, quad.y1, 1.0f, 1.0f, clr, quad.s0, quad.t1 };
			*vertices++ = { quad.x1, quad.y1, 1.0f, 1.0f, clr, quad.s1, quad.t1 };
			*vertices++ = { quad.x0, quad.y0, 1.0f, 1.0f, clr, quad.s0, quad.t0 };

			*vertices++ = { quad.x1, quad.y1, 1.0f, 1.0f, clr, quad.s1, quad.t1 };
			*vertices++ = { quad.x1, quad.y0, 1.0f, 1.0f, clr, quad.s1, quad.t0 };
			*vertices++ = { quad.x0, quad.y0, 1.0f, 1.0f, clr, quad.s0, quad.t0 };

			g_vertex_buffers->Unlock( );

			csgo::i::dev->DrawPrimitive( D3DPT_TRIANGLELIST, 0, 2 );
		};

		switch ( flags ) {
		case text_flags_t::text_flags_none: {

		}break;
		case text_flags_t::text_flags_dropshadow: {
			auto shadow_offset = (max_quad.y1 - max_quad.y0) * 0.03f;

			shadow_offset = shadow_offset < 1.1f ? 1.0f : shadow_offset;

			add_vertices( x + shadow_offset, y + shadow_offset, D3DCOLOR_RGBA( 0, 0, 0, color >> 24 ) );
		} break;
		case text_flags_t::text_flags_outline: {
			auto outline_offset = ( max_quad.y1 - max_quad.y0 ) * 0.03f;
			auto outline_color = D3DCOLOR_RGBA ( 0, 0, 0, color >> 24 );

			outline_offset = outline_offset < 1.1f ? 1.0f : outline_offset;

			add_vertices( x + outline_offset, y + outline_offset, outline_color );
			add_vertices( x, y + outline_offset, outline_color );
			add_vertices( x - outline_offset, y + outline_offset, outline_color );
			add_vertices( x - outline_offset, y, outline_color );
			add_vertices( x - outline_offset, y - outline_offset, outline_color );
			add_vertices( x, y - outline_offset, outline_color );
			add_vertices( x + outline_offset, y - outline_offset, outline_color );
			add_vertices( x + outline_offset, y, outline_color );

		} break;
		}

		add_vertices( x, y, color );

		x = qx;
		y = qy;

		i += utf8codepointsize( cp );
	}

	csgo::i::dev->SetTexture( 0, nullptr );
}

void truetype::font::draw_atlas( float x, float y ) {
	create_texture( );

	if ( !font_textures[id] )
		return;

	g_sprite->Begin( D3DXSPRITE_ALPHABLEND );
	g_sprite->Draw( font_textures[id], nullptr, nullptr, &D3DXVECTOR3{ x, y, 0.0f }, D3DCOLOR_RGBA( 255, 255, 255, 255 ) );
	g_sprite->End( );
}

void truetype::begin( ) {
	if (!g_vertex_buffers )
		csgo::i::dev->CreateVertexBuffer ( sizeof ( font_vertex_t ) * 12, D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY, D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_TEX1, D3DPOOL_DEFAULT, &g_vertex_buffers, nullptr );

	if ( !g_sprite )
		D3DXCreateSprite( csgo::i::dev, &g_sprite );
}

void truetype::end( ) {
	if ( !font_textures.empty( ) ) {
		for ( auto& texture : font_textures ) {
			if ( texture.second ) {
				texture.second->Release( );
				texture.second = nullptr;
			}
		}

		font_textures.clear( );
	}

	if ( g_sprite ) {
		g_sprite->Release ( );
		g_sprite = nullptr;
	}

	if ( g_vertex_buffers ) {
		g_vertex_buffers->Release ( );
		g_vertex_buffers = nullptr;
	}
}