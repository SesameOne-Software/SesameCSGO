#include "end_scene.hpp"
#include "../globals.hpp"
#include "../menu/menu.hpp"

#include "../features/antiaim.hpp"
#include "../features/chams.hpp"
#include "../features/glow.hpp"
#include "../features/nade_prediction.hpp"
#include "../features/other_visuals.hpp"
#include "../features/esp.hpp"
#include "../animations/resolver.hpp"
#include "../javascript/js_api.hpp"
#include "../features/ragebot.hpp"
#include "../menu/options.hpp"

#include "../renderer/font.hpp"

decltype( &hooks::end_scene ) hooks::old::end_scene = nullptr;

long __fastcall hooks::end_scene( REG, IDirect3DDevice9* device ) {
	static auto& removals = options::vars [ _( "visuals.other.removals" ) ].val.l;

	static auto ret = _ReturnAddress( );

	if ( ret != _ReturnAddress( ) )
		return old::end_scene( REG_OUT, device );

	IDirect3DStateBlock9* pixel_state = nullptr;
	IDirect3DVertexDeclaration9* vertex_decleration = nullptr;
	IDirect3DVertexShader9* vertex_shader = nullptr;

	DWORD rs_anti_alias = 0;

	device->GetRenderState( D3DRS_MULTISAMPLEANTIALIAS, &rs_anti_alias );
	device->CreateStateBlock( D3DSBT_PIXELSTATE, &pixel_state );
	device->GetVertexDeclaration( &vertex_decleration );
	device->GetVertexShader( &vertex_shader );
	device->SetRenderState( D3DRS_MULTISAMPLEANTIALIAS, false );
	device->SetVertexShader( nullptr );
	device->SetPixelShader( nullptr );

	device->SetVertexShader( nullptr );
	device->SetPixelShader( nullptr );
	device->SetFVF( D3DFVF_XYZRHW | D3DFVF_DIFFUSE );
	device->SetRenderState( D3DRS_LIGHTING, false );
	device->SetRenderState( D3DRS_FOGENABLE, false );
	device->SetRenderState( D3DRS_CULLMODE, D3DCULL_NONE );
	device->SetRenderState( D3DRS_FILLMODE, D3DFILL_SOLID );
	device->SetRenderState( D3DRS_ZENABLE, D3DZB_FALSE );
	device->SetRenderState( D3DRS_SCISSORTESTENABLE, true );
	device->SetRenderState( D3DRS_ZWRITEENABLE, false );
	device->SetRenderState( D3DRS_STENCILENABLE, false );
	device->SetRenderState( D3DRS_MULTISAMPLEANTIALIAS, true );
	device->SetRenderState( D3DRS_ANTIALIASEDLINEENABLE, true );
	device->SetRenderState( D3DRS_ALPHABLENDENABLE, true );
	device->SetRenderState( D3DRS_ALPHATESTENABLE, false );
	device->SetRenderState( D3DRS_SEPARATEALPHABLENDENABLE, true );
	device->SetRenderState( D3DRS_SRCBLEND, D3DBLEND_SRCALPHA );
	device->SetRenderState( D3DRS_SRCBLENDALPHA, D3DBLEND_INVDESTALPHA );
	device->SetRenderState( D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA );
	device->SetRenderState( D3DRS_DESTBLENDALPHA, D3DBLEND_ONE );
	device->SetRenderState( D3DRS_SRGBWRITEENABLE, false );
	device->SetRenderState( D3DRS_COLORWRITEENABLE, D3DCOLORWRITEENABLE_RED | D3DCOLORWRITEENABLE_GREEN | D3DCOLORWRITEENABLE_BLUE | D3DCOLORWRITEENABLE_ALPHA );
	
	truetype::begin ( );

	security_handler::update( );

	//features::esp::dbg_font.draw_atlas ( 20.0f, 20.0f );

	RUN_SAFE(
		"features::nade_prediction::draw",
		features::nade_prediction::draw( );
	);

	RUN_SAFE(
		"features::esp::render",
		features::esp::render( );
	);

	RUN_SAFE(
		"animations::resolver::render_impacts",
		animations::resolver::render_impacts( );
	);

	features::ragebot::scan_points.draw( );

	if ( removals [ 2 ] && g::local && g::local->scoped( ) ) {
		int w, h;
		render::screen_size( w, h );

		const auto crosshair_gap = 0.0296f * static_cast< float > ( h );

		render::gradient( w / 2, h / 2 + crosshair_gap, 1, h / 2.5f, D3DCOLOR_RGBA( 255, 251, 237, 150 ), D3DCOLOR_RGBA( 255, 251, 237, 0 ), false );
		render::gradient( w / 2, h / 2 - crosshair_gap, 1, -h / 2.5f, D3DCOLOR_RGBA( 255, 251, 237, 150 ), D3DCOLOR_RGBA( 255, 251, 237, 0 ), false );
		render::gradient( w / 2 + crosshair_gap, h / 2, h / 2.5f, 1, D3DCOLOR_RGBA( 255, 251, 237, 150 ), D3DCOLOR_RGBA( 255, 251, 237, 0 ), true );
		render::gradient( w / 2 - crosshair_gap, h / 2, -h / 2.5f, 1, D3DCOLOR_RGBA( 255, 251, 237, 150 ), D3DCOLOR_RGBA( 255, 251, 237, 0 ), true );
	}

	RUN_SAFE(
		"features::spread_circle::draw",
		features::spread_circle::draw( );
	);

	RUN_SAFE(
		"features::offscreen_esp::draw",
		features::offscreen_esp::draw( );
	);

	RUN_SAFE(
		"js::process_render_callbacks",
		js::process_render_callbacks( );
	);

	RUN_SAFE(
		"menu::draw",
		gui::draw( );
	);

	pixel_state->Apply( );
	pixel_state->Release( );

	device->SetVertexDeclaration( vertex_decleration );
	device->SetVertexShader( vertex_shader );
	device->SetRenderState( D3DRS_MULTISAMPLEANTIALIAS, rs_anti_alias );

	//truetype::end ( );

	return old::end_scene( REG_OUT, device );
}