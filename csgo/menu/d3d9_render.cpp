#include <codecvt>

#include "d3d9_render.hpp"
#include "../sdk/sdk.hpp"
#include "../renderer/d3d9.hpp"
#include "../renderer/font.hpp"

#include "../segoeui.h"
#include "../icons/generated_font/sesame_icons.hpp"

struct vertex_t {
	float x, y, z, rhw;
	uint32_t color;
};

float sesui::binds::frame_time = 0.0f;

void sesui::binds::draw_texture( const std::vector< uint8_t >& data, const vec2& pos, const vec2& dim, const vec2& scale, const color& color ) noexcept {
	ID3DXSprite* sprite = nullptr;
	IDirect3DTexture9* tex = nullptr;

	D3DXCreateSprite ( csgo::i::dev, &sprite );
	D3DXCreateTextureFromFileInMemory ( csgo::i::dev, data.data( ), sizeof ( uint32_t ) * dim.x * dim.y, &tex );

	render::texture ( sprite, tex, pos.x, pos.y, dim.x, dim.y, scale.x, scale.y );

	sprite->Release ( );
	tex->Release ( );
}

void sesui::binds::create_font ( sesui::font& font, bool force ) noexcept {
	if ( force && font.data ) {
		delete font.data;
		font.data = nullptr;
	}

	if ( font.data )
		return;
	
	if ( font.family == _ ( "sesame_ui" ) ) {
		if ( auto font_out = truetype::create_font ( resources::sesame_ui_font, _ ( "sesame_ui" ), static_cast< float >( font.size )* sesui::globals::dpi ) )
			font.data = new truetype::font { font_out.value ( ) };
	}
	else if ( font.family == _ ( "sesame_icons" ) ) {
		if ( auto font_out = truetype::create_font ( resources::sesame_icons_font, _ ( "sesame_icons" ), static_cast< float >( font.size )* sesui::globals::dpi ) )
			font.data = new truetype::font { font_out.value ( ) };
	}
}

void sesui::binds::polygon ( const std::vector< sesui::vec2 >& verticies, const sesui::color& color, bool filled ) noexcept {
	vertex_t* vtx = new vertex_t [ filled ? verticies.size ( ) : ( verticies.size ( ) + 1 ) ];

	for ( auto i = 0; i < verticies.size ( ); i++ ) {
		vtx[i].x = verticies [ i ].x;
		vtx[i].y = verticies [ i ].y;
		vtx [ i ].color = D3DCOLOR_RGBA ( static_cast< int >( color.r * 255.0f ), static_cast< int >( color.g * 255.0f ), static_cast< int >( color.b * 255.0f ), static_cast< int >( color.a * 255.0f ) );
		vtx[i].z = 0.0f;
		vtx[i].rhw = 1.0f;
	}

	if ( !filled ) {
		vtx[ verticies.size ( ) ] = vtx [ 0 ];
	}

	csgo::i::dev->SetRenderState ( D3DRS_ALPHABLENDENABLE, true );
	csgo::i::dev->SetFVF ( D3DFVF_XYZRHW | D3DFVF_DIFFUSE );
	csgo::i::dev->SetTexture ( 0, nullptr );
	csgo::i::dev->DrawPrimitiveUP ( filled ? D3DPT_TRIANGLEFAN : D3DPT_LINESTRIP, filled ? verticies.size ( ) - 2 : verticies.size ( ), vtx, sizeof( vertex_t ) );

	delete [ ] vtx;
}

void sesui::binds::multicolor_polygon ( const std::vector< sesui::vec2 >& verticies, const std::vector< sesui::color >& colors, bool filled ) noexcept {
	vertex_t* vtx = new vertex_t [ filled ? verticies.size ( ) : ( verticies.size ( ) + 1 ) ];

	for ( auto i = 0; i < verticies.size ( ); i++ ) {
		vtx [ i ].x = verticies [ i ].x;
		vtx [ i ].y = verticies [ i ].y;
		vtx [ i ].color = D3DCOLOR_RGBA ( static_cast< int >( colors [ i ].r * 255.0f ), static_cast< int >( colors [ i ].g * 255.0f ), static_cast< int >( colors [ i ].b * 255.0f ), static_cast< int >( colors [ i ].a * 255.0f ) );
		vtx [ i ].z = 0.0f;
		vtx [ i ].rhw = 1.0f;
	}

	if ( !filled ) {
		vtx [ verticies.size ( ) ] = vtx [ 0 ];
	}

	csgo::i::dev->SetRenderState ( D3DRS_ALPHABLENDENABLE, true );
	csgo::i::dev->SetFVF ( D3DFVF_XYZRHW | D3DFVF_DIFFUSE );
	csgo::i::dev->SetTexture ( 0, nullptr );
	csgo::i::dev->DrawPrimitiveUP ( filled ? D3DPT_TRIANGLEFAN : D3DPT_LINESTRIP, filled ? verticies.size ( ) - 2 : verticies.size ( ), vtx, sizeof ( vertex_t ) );

	delete [ ] vtx;
}

void sesui::binds::text ( const sesui::vec2& pos, const sesui::font& font, const std::string& text, const sesui::color& color ) noexcept {
	if ( !font.data )
		return;

	reinterpret_cast< truetype::font* >( font.data )->draw_text ( pos.x, pos.y, text, D3DCOLOR_RGBA ( static_cast< int >( color.r * 255.0f ), static_cast< int >( color.g * 255.0f ), static_cast< int >( color.b * 255.0f ), static_cast< int >( color.a * 255.0f ) ), truetype::text_flags_t::text_flags_none );
}

void sesui::binds::get_text_size ( const sesui::font& font, const std::string& text, sesui::vec2& dim_out ) noexcept {
	if ( !font.data )
		dim_out = { 0.0f, 0.0f };

	reinterpret_cast< truetype::font* >( font.data )->text_size ( text, dim_out.x, dim_out.y );
}

float sesui::binds::get_frametime ( ) noexcept {
	return frame_time;
}

void sesui::binds::begin_clip ( const sesui::rect& region ) noexcept {
	RECT rect { region.x, region.y, region.x + region.w, region.y + region.h };

	csgo::i::dev->SetRenderState ( D3DRS_SCISSORTESTENABLE, true );
	csgo::i::dev->SetScissorRect ( &rect );
}

void sesui::binds::end_clip ( ) noexcept {
	csgo::i::dev->SetRenderState ( D3DRS_SCISSORTESTENABLE, false );
}