#pragma once
#include <string_view>
#include <d3d9.h>
#include <d3dx9.h>
#include "impl.h"
#include "../oxui.h"
#include "../../sdk/sdk.h"

struct vtx {
	float x, y, z, rhw;
	std::uint32_t color;
};

struct custom_vtx {
	float x, y, z, rhw;
	std::uint32_t color;
	float tu, tv;
};

oxui::font create_font( const std::string_view& name, int size, bool bold ) {
	ID3DXFont* d3d_font = nullptr;
	D3DXCreateFontA( csgo::i::dev, size, name == "Tapfont" ? 0 : /* static_cast< int >( static_cast< float >( size ) * 0.5f + 0.5f ) */ 0, bold ? FW_BOLD : FW_NORMAL, 0, false, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, name.data( ), & d3d_font );
	return reinterpret_cast< oxui::font >( d3d_font );
}

void destroy_font( oxui::font font ) {
	reinterpret_cast< ID3DXFont* >( font )->Release( );
}

void draw_text( oxui::point pos, oxui::color color, oxui::font font, const std::string_view& text, bool shadow ) {
	RECT rect;
	SetRect( &rect, pos.x, pos.y, pos.x, pos.y );

	RECT rect_shadow;
	SetRect( &rect_shadow, pos.x + 1, pos.y + 1, pos.x + 1, pos.y + 1 );

	if ( shadow )
		reinterpret_cast< ID3DXFont* >( font )->DrawTextA( nullptr, text.data( ), text.length( ), &rect_shadow, DT_LEFT | DT_TOP | DT_NOCLIP, D3DCOLOR_RGBA( 0, 0, 0, 175 ) );

	reinterpret_cast< ID3DXFont* >( font )->DrawTextA( nullptr, text.data( ), text.length( ), &rect, DT_LEFT | DT_TOP | DT_NOCLIP, D3DCOLOR_RGBA( color.r, color.g, color.b, color.a ) );
}

void draw_filled_rect( oxui::rect rect, oxui::color color ) {
	vtx vert [ 4 ] = {
		{ rect.x - 0.5f, rect.y - 0.5f, 0.0f, 1.0f, D3DCOLOR_RGBA( color.r, color.g, color.b, color.a ) },
		{ rect.x + rect.w - 0.5f, rect.y - 0.5f, 0.0f, 1.0f, D3DCOLOR_RGBA( color.r, color.g, color.b, color.a ) },
		{ rect.x - 0.5f, rect.y + rect.h - 0.5f, 0.0f, 1.0f, D3DCOLOR_RGBA( color.r, color.g, color.b, color.a ) },
		{ rect.x + rect.w - 0.5f, rect.y + rect.h - 0.5f, 0.0f, 1.0f, D3DCOLOR_RGBA( color.r, color.g, color.b, color.a ) }
	};

	csgo::i::dev->SetRenderState( D3DRS_ALPHABLENDENABLE, true );
	csgo::i::dev->SetTexture( 0, nullptr );
	csgo::i::dev->SetPixelShader( nullptr );
	csgo::i::dev->SetRenderState( D3DRS_SRCBLEND, D3DBLEND_SRCALPHA );
	csgo::i::dev->SetRenderState( D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA );
	csgo::i::dev->SetFVF( D3DFVF_XYZRHW | D3DFVF_DIFFUSE );
	csgo::i::dev->DrawPrimitiveUP( D3DPT_TRIANGLESTRIP, 2, &vert, sizeof( vtx ) );
}

void draw_rect( oxui::rect rect, oxui::color color ) {
	draw_filled_rect( oxui::rect( rect.x, rect.y, rect.w, 1 ), color );
	draw_filled_rect( oxui::rect( rect.x, rect.y + rect.h, rect.w, 1 ), color );
	draw_filled_rect( oxui::rect( rect.x, rect.y, 1, rect.h ), color );
	draw_filled_rect( oxui::rect( rect.x + rect.w, rect.y, 1, rect.h + 1 ), color );
}

oxui::dimension text_scale( oxui::font font, const std::string_view& text ) {
	RECT rect = { 0, 0, 0, 0 };
	reinterpret_cast< ID3DXFont* >( font )->DrawTextA( nullptr, text.data( ), text.length( ), &rect, DT_CALCRECT, D3DCOLOR_RGBA( 0, 0, 0, 0 ) );
	return oxui::dimension( rect.right - rect.left, rect.bottom - rect.top );
}

bool key_pressed( oxui::keys key ) {
	return GetAsyncKeyState( static_cast< int >( key ) );
}

oxui::point cursor_pos( ) {
	int x, y;
	csgo::i::surface->get_cursor_pos( x, y );

	return oxui::point( x, y );
}

void clip( oxui::rect rect ) {
	const auto new_rect = RECT { rect.x, rect.y, rect.x + rect.w, rect.y + rect.h };

	csgo::i::dev->SetRenderState( D3DRS_SCISSORTESTENABLE, true );
	csgo::i::dev->SetScissorRect( &new_rect );
}

void draw_circle( oxui::point rect, int radius, int segments, oxui::color color ) {
	std::vector< vtx > circle( segments + 2 );

	const auto angle = 360.0f * D3DX_PI / 180.0f;

	circle [ 0 ].x = rect.x - 0.5f;
	circle [ 0 ].y = rect.y - 0.5f;
	circle [ 0 ].z = 0;
	circle [ 0 ].rhw = 1;
	circle [ 0 ].color = D3DCOLOR_RGBA( color.r, color.g, color.b, color.a );

	for ( int i = 1; i < segments + 2; i++ ) {
		circle [ i ].x = ( float ) ( rect.x - radius * std::cosf( D3DX_PI * ( ( i - 1 ) / ( segments / 2.0f ) ) ) ) - 0.5f;
		circle [ i ].y = ( float ) ( rect.y - radius * std::sinf( D3DX_PI * ( ( i - 1 ) / ( segments / 2.0f ) ) ) ) - 0.5f;
		circle [ i ].z = 0;
		circle [ i ].rhw = 1;
		circle [ i ].color = D3DCOLOR_RGBA( color.r, color.g, color.b, color.a );
	}

	const auto _res = segments + 2;

	for ( int i = 0; i < _res; i++ ) {
		circle [ i ].x = rect.x + std::cosf( angle ) * ( circle [ i ].x - rect.x ) - std::sinf( angle ) * ( circle [ i ].y - rect.y ) - 0.5f;
		circle [ i ].y = rect.y + std::sinf( angle ) * ( circle [ i ].x - rect.x ) + std::cosf( angle ) * ( circle [ i ].y - rect.y ) - 0.5f;
	}

	IDirect3DVertexBuffer9* vb = nullptr;

	csgo::i::dev->CreateVertexBuffer( ( segments + 2 ) * sizeof( vtx ), D3DUSAGE_WRITEONLY, D3DFVF_XYZRHW | D3DFVF_DIFFUSE, D3DPOOL_DEFAULT, &vb, nullptr );

	void* verticies;
	vb->Lock( 0, ( segments + 2 ) * sizeof( vtx ), ( void** ) &verticies, 0 );
	std::memcpy( verticies, &circle [ 0 ], ( segments + 2 ) * sizeof( vtx ) );
	vb->Unlock( );

	csgo::i::dev->SetTexture( 0, nullptr );
	csgo::i::dev->SetPixelShader( nullptr );
	csgo::i::dev->SetRenderState( D3DRS_ALPHABLENDENABLE, true );
	csgo::i::dev->SetRenderState( D3DRS_SRCBLEND, D3DBLEND_SRCALPHA );
	csgo::i::dev->SetRenderState( D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA );
	csgo::i::dev->SetStreamSource( 0, vb, 0, sizeof( vtx ) );
	csgo::i::dev->SetFVF( D3DFVF_XYZRHW | D3DFVF_DIFFUSE );
	csgo::i::dev->DrawPrimitive( D3DPT_TRIANGLEFAN, 0, segments );

	if ( vb )
		vb->Release( );
}

void remove_clip( ) {
	csgo::i::dev->SetRenderState( D3DRS_SCISSORTESTENABLE, false );
}

void draw_line( oxui::point p1, oxui::point p2, oxui::color color ) {
	vtx vtx [ 2 ] = { { p1.x, p1.y, 0.0f, 1.0f, D3DCOLOR_RGBA( color.r, color.g, color.b, color.a ) }, { p2.x, p2.y, 0.0f, 1.0f, D3DCOLOR_RGBA( color.r, color.g, color.b, color.a ) } };

	csgo::i::dev->SetTexture( 0, nullptr );
	csgo::i::dev->DrawPrimitiveUP( D3DPT_LINELIST, 1, &vtx, sizeof( ::vtx ) );
}

void oxui::impl::init( ) {
	oxui::impl::create_font = ::create_font;
	oxui::impl::destroy_font = ::destroy_font;
	oxui::impl::draw_text = ::draw_text;
	oxui::impl::draw_rect = ::draw_rect;
	oxui::impl::draw_filled_rect = ::draw_filled_rect;
	oxui::impl::text_scale = ::text_scale;
	oxui::impl::key_pressed = ::key_pressed;
	oxui::impl::cursor_pos = ::cursor_pos;
	oxui::impl::clip = ::clip;
	oxui::impl::draw_circle = ::draw_circle;
	oxui::impl::remove_clip = ::remove_clip;
	oxui::impl::draw_line = ::draw_line;
}