#include <shlobj_core.h>
#include <vector>

#include "../sdk/sdk.hpp"
#include "font.hpp"

ID3DXSprite* font_sprite = nullptr;

void truetype::font::set_font_size ( float x ) {
	size = x;
}

std::optional<truetype::font> truetype::create_font ( const uint8_t* font_data, std::string_view font_name, float size ) {
	stbtt_fontinfo font_info;
	
	if ( stbtt_InitFont ( &font_info, font_data, 0 ) )
		return font { font_name.data ( ), size, font_info };

	return std::nullopt;
}

void truetype::font::text_size ( const std::string& string, float& x_out, float& y_out ) {
	x_out = 0.0f;
	y_out = 0.0f;

	auto scale = stbtt_ScaleForPixelHeight ( &font_info, size );
	
	for ( auto i = 0; i < string.size ( ); ++i ) {
		auto ax = 0;
		stbtt_GetCodepointHMetrics ( &font_info, string [ i ], &ax, 0 );
		x_out += ax * scale;
		
		if ( string [ i + 1 ] ) {
			auto kern = stbtt_GetCodepointKernAdvance ( &font_info, string [ i ], string [ i + 1 ] );
			x_out += kern * scale;
		}
	}

	y_out = static_cast< int >( size + 0.5f );
}

IDirect3DTexture9* truetype::font::create_texture ( const std::string& string ) {
	constexpr auto bitmap_width = 512;

	auto bitmap = std::make_unique<std::uint8_t [ ]> ( bitmap_width * static_cast< int >( size + 0.5f ) );

	auto scale = stbtt_ScaleForPixelHeight ( &font_info, size );

	auto x = 0, ascent = 0, descent = 0, lineGap = 0;
	stbtt_GetFontVMetrics ( &font_info, &ascent, &descent, &lineGap );

	ascent *= scale;
	descent *= scale;

	for ( auto i = 0; i < string.size ( ); ++i ) {
		auto c_x1 = 0, c_y1 = 0, c_x2 = 0, c_y2 = 0;
		stbtt_GetCodepointBitmapBox ( &font_info, string [ i ], scale, scale, &c_x1, &c_y1, &c_x2, &c_y2 );

		auto y = ascent + c_y1;

		auto byteOffset = x + ( y * bitmap_width );
		stbtt_MakeCodepointBitmap ( &font_info, bitmap.get ( ) + byteOffset, c_x2 - c_x1, c_y2 - c_y1, bitmap_width, scale, scale, string [ i ] );

		auto ax = 0;
		stbtt_GetCodepointHMetrics ( &font_info, string [ i ], &ax, 0 );
		x += ax * scale;

		if ( string [ i + 1 ] ) {
			auto kern = stbtt_GetCodepointKernAdvance ( &font_info, string [ i ], string [ i + 1 ] );
			x += kern * scale;
		}
	}

	IDirect3DTexture9* texture = nullptr;

	if ( D3DXCreateTexture ( csgo::i::dev, bitmap_width, static_cast< int >( size + 0.5f ), 1, D3DUSAGE_DYNAMIC, D3DFMT_A8R8G8B8/*D3DFMT_L8*/, D3DPOOL_DEFAULT, &texture ) < 0 )
		return nullptr;

	D3DLOCKED_RECT rect;

	if ( !texture )
		return nullptr;

	if ( texture->LockRect ( 0, &rect, nullptr, 0 ) < 0 ) {
		texture->Release ( );
		return nullptr;
	}

	auto expanded = std::make_unique< std::uint32_t [ ] > ( bitmap_width * static_cast< int >( size + 0.5f ) * sizeof(uint32_t) );
	
	for ( int y = 0; y < static_cast< int >( size + 0.5f ); y++ ) {
		for ( int cx = 0; cx < bitmap_width; cx++ ) {
			auto index = ( y * bitmap_width ) + cx;
			auto value = ( uint32_t ) bitmap [ index ];
			expanded [ index ] = value << 24;
			expanded [ index ] |= 0x00FFFFFF;
		}
	}
	
	memcpy ( rect.pBits, expanded.get ( ), bitmap_width * static_cast< int >( size + 0.5f ) * sizeof ( uint32_t ) );

	//memcpy ( rect.pBits, bitmap.get ( ), bitmap_width * static_cast< int >( size + 0.5f ) * sizeof ( uint8_t ) );

	texture->UnlockRect ( 0 );

	return texture;
}

void truetype::font::draw_text ( float x, float y, const std::string& string, uint32_t color, text_flags_t flags ) {
	auto texture = create_texture ( string );

	if ( !texture || !font_sprite ) {
		if ( texture ) {
			texture->Release ( );
			texture = nullptr;
		}

		return;
	}

	if ( font_sprite->Begin ( D3DXSPRITE_ALPHABLEND ) >= 0 ) {
		switch ( flags ) {
		case text_flags_t::text_flags_none:
			break;
		case text_flags_t::text_flags_dropshadow:
			font_sprite->Draw ( texture, nullptr, nullptr, &D3DXVECTOR3 { x + 1.0f, y + 1.0f, 0.0f }, D3DCOLOR_RGBA ( 0, 0, 0, color >> 24 ) );
			break;
		case text_flags_t::text_flags_outline:
			font_sprite->Draw ( texture, nullptr, nullptr, &D3DXVECTOR3 { x - 1.0f, y - 1.0f, 0.0f }, D3DCOLOR_RGBA ( 0, 0, 0, color >> 24 ) );
			font_sprite->Draw ( texture, nullptr, nullptr, &D3DXVECTOR3 { x + 1.0f, y + 1.0f, 0.0f }, D3DCOLOR_RGBA ( 0, 0, 0, color >> 24 ) );
			font_sprite->Draw ( texture, nullptr, nullptr, &D3DXVECTOR3 { x + 1.0f, y - 1.0f, 0.0f }, D3DCOLOR_RGBA ( 0, 0, 0, color >> 24 ) );
			font_sprite->Draw ( texture, nullptr, nullptr, &D3DXVECTOR3 { x - 1.0f, y + 1.0f, 0.0f }, D3DCOLOR_RGBA ( 0, 0, 0, color >> 24 ) );
			break;
		}

		font_sprite->Draw ( texture, nullptr, nullptr, &D3DXVECTOR3 { x, y, 0.0f }, color );

		font_sprite->End ( );
	}

	if ( texture ) {
		texture->Release ( );
		texture = nullptr;
	}
}

void truetype::begin ( ) {
	if ( !font_sprite )
		D3DXCreateSprite ( csgo::i::dev, &font_sprite );
}

void truetype::end ( ) {
	if ( font_sprite ) {
		font_sprite->Release ( );
		font_sprite = nullptr;
	}
}