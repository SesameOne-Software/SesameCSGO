#include "end_scene.hpp"
#include "../globals.hpp"
#include "../menu/menu.hpp"

#include "../features/antiaim.hpp"
#include "../features/chams.hpp"
#include "../features/glow.hpp"
#include "../features/nade_prediction.hpp"
#include "../features/other_visuals.hpp"
#include "../features/esp.hpp"
#include "../animations/animations.hpp"
#include "../animations/resolver.hpp"
#include "../javascript/js_api.hpp"
#include "../menu/options.hpp"

decltype( &hooks::end_scene ) hooks::old::end_scene = nullptr;

long __fastcall hooks::end_scene ( REG, IDirect3DDevice9* device ) {
	static auto& removals = options::vars [ _ ( "visuals.other.removals" ) ].val.l;

	static auto ret = _ReturnAddress ( );

	if ( ret != _ReturnAddress ( ) )
		return old::end_scene ( REG_OUT, device );

	IDirect3DStateBlock9* pixel_state = nullptr;
	IDirect3DVertexDeclaration9* vertex_decleration = nullptr;
	IDirect3DVertexShader9* vertex_shader = nullptr;

	DWORD rs_anti_alias = 0;

	device->GetRenderState ( D3DRS_MULTISAMPLEANTIALIAS, &rs_anti_alias );
	device->CreateStateBlock ( D3DSBT_PIXELSTATE, &pixel_state );
	device->GetVertexDeclaration ( &vertex_decleration );
	device->GetVertexShader ( &vertex_shader );
	device->SetRenderState ( D3DRS_MULTISAMPLEANTIALIAS, false );
	device->SetVertexShader ( nullptr );
	device->SetPixelShader ( nullptr );

	device->SetVertexShader ( nullptr );
	device->SetPixelShader ( nullptr );
	device->SetFVF ( D3DFVF_XYZRHW | D3DFVF_DIFFUSE );
	device->SetRenderState ( D3DRS_LIGHTING, false );
	device->SetRenderState ( D3DRS_FOGENABLE, false );
	device->SetRenderState ( D3DRS_CULLMODE, D3DCULL_NONE );
	device->SetRenderState ( D3DRS_FILLMODE, D3DFILL_SOLID );
	device->SetRenderState ( D3DRS_ZENABLE, D3DZB_FALSE );
	device->SetRenderState ( D3DRS_SCISSORTESTENABLE, true );
	device->SetRenderState ( D3DRS_ZWRITEENABLE, false );
	device->SetRenderState ( D3DRS_STENCILENABLE, false );
	device->SetRenderState ( D3DRS_MULTISAMPLEANTIALIAS, true );
	device->SetRenderState ( D3DRS_ANTIALIASEDLINEENABLE, true );
	device->SetRenderState ( D3DRS_ALPHABLENDENABLE, true );
	device->SetRenderState ( D3DRS_ALPHATESTENABLE, false );
	device->SetRenderState ( D3DRS_SEPARATEALPHABLENDENABLE, true );
	device->SetRenderState ( D3DRS_SRCBLEND, D3DBLEND_SRCALPHA );
	device->SetRenderState ( D3DRS_SRCBLENDALPHA, D3DBLEND_INVDESTALPHA );
	device->SetRenderState ( D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA );
	device->SetRenderState ( D3DRS_DESTBLENDALPHA, D3DBLEND_ONE );
	device->SetRenderState ( D3DRS_SRGBWRITEENABLE, false );
	device->SetRenderState ( D3DRS_COLORWRITEENABLE, D3DCOLORWRITEENABLE_RED | D3DCOLORWRITEENABLE_GREEN | D3DCOLORWRITEENABLE_BLUE | D3DCOLORWRITEENABLE_ALPHA );

	security_handler::update ( );

	RUN_SAFE (
		"features::nade_prediction::draw",
		features::nade_prediction::draw ( );
	);

	RUN_SAFE (
		"features::esp::render",
		features::esp::render ( );
	);

	RUN_SAFE (
		"animations::resolver::render_impacts",
		animations::resolver::render_impacts ( );
	);

	if ( removals[2] && g::local && g::local->scoped ( ) ) {
		int w, h;
		render::screen_size ( w, h );
		render::line ( w / 2, 0, w / 2, h, D3DCOLOR_RGBA ( 0, 0, 0, 255 ) );
		render::line ( 0, h / 2, w, h / 2, D3DCOLOR_RGBA ( 0, 0, 0, 255 ) );
	}

	RUN_SAFE (
		"features::spread_circle::draw",
		features::spread_circle::draw ( );
	);

	RUN_SAFE (
		"features::offscreen_esp::draw",
		features::offscreen_esp::draw ( );
	);

	RUN_SAFE (
		"js::process_render_callbacks",
		js::process_render_callbacks ( );
	);

	RUN_SAFE (
		"menu::draw",
		gui::draw ( );
	);

	pixel_state->Apply ( );
	pixel_state->Release ( );

	device->SetVertexDeclaration ( vertex_decleration );
	device->SetVertexShader ( vertex_shader );
	device->SetRenderState ( D3DRS_MULTISAMPLEANTIALIAS, rs_anti_alias );

	return old::end_scene ( REG_OUT, device );
}