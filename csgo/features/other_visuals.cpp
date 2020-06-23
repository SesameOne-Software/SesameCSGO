#include "other_visuals.hpp"
#include "../menu/menu.hpp"
#include "../hooks.hpp"
#include "../globals.hpp"
#include <deque>
#include <mutex>

float features::spread_circle::total_spread = 0.0f;

void features::spread_circle::draw ( ) {
	OPTION ( double, custom_fov, "Sesame->C->Other->Removals->Custom FOV", oxui::object_slider );
	OPTION ( oxui::color, spread_circle_clr, "Sesame->C->Other->World->Spread Circle Color", oxui::object_colorpicker );
	OPTION ( bool, spread_circle, "Sesame->C->Other->World->Spread Circle", oxui::object_checkbox );
	OPTION ( bool, gradient_spread_circle, "Sesame->C->Other->World->Gradient Spread Circle", oxui::object_checkbox );

	int w = 0, h = 0;
	render::screen_size ( w, h );

	if ( !g::local || !g::local->alive() || !g::local->weapon ( ) || !spread_circle || !total_spread )
		return;

	const auto weapon = g::local->weapon ( );
	const auto radius = ( total_spread * 320.0f ) / std::tanf ( csgo::deg2rad( custom_fov ) * 0.5f );

	if ( gradient_spread_circle ) {
		auto x = w / 2;
		auto y = h / 2;

		std::vector< render::vtx_t > circle ( 48 + 2 );

		auto pi = D3DX_PI;
		const auto angle = 0.0f;

		circle [ 0 ].x = static_cast< float > ( x ) - 0.5f;
		circle [ 0 ].y = static_cast< float > ( y ) - 0.5f;
		circle [ 0 ].z = 0;
		circle [ 0 ].rhw = 1;
		circle [ 0 ].color = D3DCOLOR_RGBA ( 0, 0, 0, 0 );

		for ( auto i = 1; i < 48 + 2; i++ ) {
			circle [ i ].x = ( float ) ( x - radius * std::cosf ( pi * ( ( i - 1 ) / ( 48.0f / 2.0f ) ) ) ) - 0.5f;
			circle [ i ].y = ( float ) ( y - radius * std::sinf ( pi * ( ( i - 1 ) / ( 48.0f / 2.0f ) ) ) ) - 0.5f;
			circle [ i ].z = 0;
			circle [ i ].rhw = 1;
			circle [ i ].color = D3DCOLOR_RGBA ( spread_circle_clr.r, spread_circle_clr.g, spread_circle_clr.b, spread_circle_clr.a );
		}

		const auto _res = 48 + 2;

		for ( auto i = 0; i < _res; i++ ) {
			circle [ i ].x = x + std::cosf ( angle ) * ( circle [ i ].x - x ) - std::sinf ( angle ) * ( circle [ i ].y - y ) - 0.5f;
			circle [ i ].y = y + std::sinf ( angle ) * ( circle [ i ].x - x ) + std::cosf ( angle ) * ( circle [ i ].y - y ) - 0.5f;
		}

		IDirect3DVertexBuffer9* vb = nullptr;

		csgo::i::dev->CreateVertexBuffer ( ( 48 + 2 ) * sizeof ( render::vtx_t ), D3DUSAGE_WRITEONLY, D3DFVF_XYZRHW | D3DFVF_DIFFUSE, D3DPOOL_DEFAULT, &vb, nullptr );

		void* verticies;
		vb->Lock ( 0, ( 48 + 2 ) * sizeof ( render::vtx_t ), ( void** ) &verticies, 0 );
		std::memcpy ( verticies, &circle [ 0 ], ( 48 + 2 ) * sizeof ( render::vtx_t ) );
		vb->Unlock ( );

		csgo::i::dev->SetStreamSource ( 0, vb, 0, sizeof ( render::vtx_t ) );
		csgo::i::dev->DrawPrimitive ( D3DPT_TRIANGLEFAN, 0, 48 );

		if ( vb )
			vb->Release ( );
	}
	else {
		render::circle ( w / 2, h / 2, radius, 48, D3DCOLOR_RGBA ( spread_circle_clr.r, spread_circle_clr.g, spread_circle_clr.b, spread_circle_clr.a ) );
		render::circle ( w / 2, h / 2, radius, 48, D3DCOLOR_RGBA( spread_circle_clr.r, spread_circle_clr.g, spread_circle_clr.b, spread_circle_clr.a ), true );
	}
}