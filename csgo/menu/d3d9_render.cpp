#include "d3d9_render.hpp"
#include <sdk.hpp>
#include "../renderer/d3d9.hpp"

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
		reinterpret_cast< ID3DXFont* > ( font.data )->Release ( );
		font.data = nullptr;
	}

	if ( font.data )
		return;

	D3DXCreateFontW ( csgo::i::dev, static_cast< int > ( static_cast< float >( font.size )* sesui::globals::dpi ), 0, font.weight, 0, font.italic, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, font.family.get ( ), reinterpret_cast< ID3DXFont** > ( &font.data ) );
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

void sesui::binds::text ( const sesui::vec2& pos, const sesui::font& font, const sesui::ses_string& text, const sesui::color& color ) noexcept {
	if ( !font.data )
		return;

	RECT rect;
	SetRect ( &rect, pos.x, pos.y, pos.x, pos.y );
	reinterpret_cast< ID3DXFont* >( font.data )->DrawTextW ( nullptr, text.get ( ), text.len ( ), &rect, DT_LEFT | DT_NOCLIP, D3DCOLOR_RGBA ( static_cast< int >( color.r * 255.0f ), static_cast< int >( color.g * 255.0f ), static_cast< int >( color.b * 255.0f ), static_cast< int >( color.a * 255.0f ) ) );
}

void sesui::binds::get_text_size ( const sesui::font& font, const sesui::ses_string& text, sesui::vec2& dim_out ) noexcept {
	RECT rect = { 0, 0, 0, 0 };
	reinterpret_cast< ID3DXFont* >( font.data )->DrawTextW ( nullptr, text.get ( ), text.len ( ), &rect, DT_CALCRECT, 0 );
	dim_out = { rect.right - rect.left, rect.bottom - rect.top };
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