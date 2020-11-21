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
#include "../features/ragebot.hpp"
#include "../menu/options.hpp"
#include "../features/autopeek.hpp"

#include "../imgui/imgui.h"
#include "../imgui/imgui_impl_dx9.h"
#include "../imgui/imgui_impl_win32.h"
#include "../imgui/imgui_internal.h"

#include "../renderer/render.hpp"

#include "../fmt/format.h"

decltype( &hooks::end_scene ) hooks::old::end_scene = nullptr;

long __fastcall hooks::end_scene( REG, IDirect3DDevice9* device ) {
	static auto& removals = options::vars [ _( "visuals.other.removals" ) ].val.l;

	static auto ret = _ReturnAddress( );

	if ( ret != _ReturnAddress( ) )
		return old::end_scene( REG_OUT, device );

	IDirect3DStateBlock9* pixel_state = nullptr;
	IDirect3DVertexDeclaration9* vertex_decleration = nullptr;
	IDirect3DVertexShader9* vertex_shader = nullptr;

	cs::i::dev->GetVertexDeclaration ( &vertex_decleration );
	cs::i::dev->GetVertexShader ( &vertex_shader );

	cs::i::dev->CreateStateBlock ( D3DSBT_ALL, &pixel_state );
	pixel_state->Capture ( );

	device->SetRenderState ( D3DRS_COLORWRITEENABLE, 0xFFFFFFFF );
	device->SetRenderState ( D3DRS_CULLMODE, D3DCULL_NONE );
	device->SetRenderState ( D3DRS_LIGHTING, false );
	device->SetRenderState ( D3DRS_ZENABLE, false );
	device->SetRenderState ( D3DRS_ALPHABLENDENABLE, true );
	device->SetRenderState ( D3DRS_ALPHATESTENABLE, false );
	device->SetRenderState ( D3DRS_BLENDOP, D3DBLENDOP_ADD );
	device->SetRenderState ( D3DRS_SRCBLEND, D3DBLEND_SRCALPHA );
	device->SetRenderState ( D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA );
	device->SetRenderState ( D3DRS_SCISSORTESTENABLE, false );
	device->SetRenderState ( D3DRS_SHADEMODE, D3DSHADE_GOURAUD );
	device->SetRenderState ( D3DRS_FOGENABLE, false );
	device->SetTextureStageState ( 0, D3DTSS_COLOROP, D3DTOP_MODULATE );
	device->SetTextureStageState ( 0, D3DTSS_COLORARG1, D3DTA_TEXTURE );
	device->SetTextureStageState ( 0, D3DTSS_COLORARG2, D3DTA_DIFFUSE );
	device->SetTextureStageState ( 0, D3DTSS_ALPHAOP, D3DTOP_MODULATE );
	device->SetTextureStageState ( 0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE );
	device->SetTextureStageState ( 0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE );
	device->SetSamplerState ( 0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR );
	device->SetSamplerState ( 0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR );

	security_handler::update( );

	/* scale all rendering with selected DPI setting */
	gui::scale_dpi ( );

	// Start the Dear ImGui frame
	ImGui_ImplDX9_NewFrame ( );
	ImGui_ImplWin32_NewFrame ( );

	ImGui::NewFrame ( );

	/* use imgui to draw on screen directly */ {
		/* begin scene */
		ImGuiIO& io = ImGui::GetIO ( );

		ImGui::PushStyleVar ( ImGuiStyleVar_WindowBorderSize, 0.0f );
		ImGui::PushStyleVar ( ImGuiStyleVar_WindowPadding, { 0.0f, 0.0f } );
		ImGui::PushStyleColor ( ImGuiCol_WindowBg, { 0.0f, 0.0f, 0.0f, 0.0f } );
		ImGui::Begin ( "##Backbuffer", nullptr, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoInputs );

		ImGui::SetWindowPos ( ImVec2 ( 0, 0 ), ImGuiCond_Always );
		ImGui::SetWindowSize ( ImVec2 ( io.DisplaySize.x, io.DisplaySize.y ), ImGuiCond_Always );

		const auto draw_list = ImGui::GetCurrentWindow ( )->DrawList;

		/* draw stuff here */
		RUN_SAFE (
			"features::autopeek::draw",
			features::autopeek::draw ( );
		);

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

		//features::ragebot::scan_points.draw( );

		if ( removals [ 2 ] && g::local && g::local->scoped ( ) ) {
			float w, h;
			render::screen_size ( w, h );

			const auto crosshair_gap = 0.0296f * static_cast< float > ( h );

			render::gradient ( w / 2, h / 2 + crosshair_gap, 1, h / 2.5f, rgba ( 255, 251, 237, 150 ), rgba ( 255, 251, 237, 0 ), false );
			render::gradient ( w / 2, h / 2 - crosshair_gap, 1, -h / 2.5f, rgba ( 255, 251, 237, 150 ), rgba ( 255, 251, 237, 0 ), false );
			render::gradient ( w / 2 + crosshair_gap, h / 2, h / 2.5f, 1, rgba ( 255, 251, 237, 150 ), rgba ( 255, 251, 237, 0 ), true );
			render::gradient ( w / 2 - crosshair_gap, h / 2, -h / 2.5f, 1, rgba ( 255, 251, 237, 150 ), rgba ( 255, 251, 237, 0 ), true );
		}

		RUN_SAFE (
			"features::spread_circle::draw",
			features::spread_circle::draw ( );
		);

		RUN_SAFE (
			"features::offscreen_esp::draw",
			features::offscreen_esp::draw ( );
		);

		/* end scene */
		draw_list->PushClipRectFullScreen ( );

		ImGui::End ( );
		ImGui::PopStyleColor ( );
		ImGui::PopStyleVar ( 2 );
	}

	RUN_SAFE(
		"menu::draw",
		gui::draw( );
	);
	
	ImGui::EndFrame ( );
	ImGui::Render ( );

	ImGui_ImplDX9_RenderDrawData ( ImGui::GetDrawData ( ) );

	pixel_state->Apply ( );
	pixel_state->Release ( );

	cs::i::dev->SetVertexDeclaration ( vertex_decleration );
	cs::i::dev->SetVertexShader ( vertex_shader );

	//truetype::end ( );

	return old::end_scene( REG_OUT, device );
}