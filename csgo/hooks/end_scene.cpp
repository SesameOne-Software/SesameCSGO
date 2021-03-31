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

	if ( ret != _ReturnAddress ( ) )
		return old::end_scene( REG_OUT, device );

	MUTATE_START
	D3DVIEWPORT9 d3d_viewport;
	device->GetViewport ( &d3d_viewport );

	IDirect3DStateBlock9* pixel_state = NULL; IDirect3DVertexDeclaration9* vertDec; IDirect3DVertexShader9* vertShader;
	DWORD dwOld_D3DRS_COLORWRITEENABLE;
	DWORD srgbwrite;

	device->CreateStateBlock ( D3DSBT_ALL, &pixel_state );
	pixel_state->Capture ( );

	//cs::i::dev->GetVertexDeclaration ( &vertDec );
	//cs::i::dev->GetVertexShader ( &vertShader );

	device->GetRenderState ( D3DRS_COLORWRITEENABLE, &dwOld_D3DRS_COLORWRITEENABLE );
	device->GetRenderState ( D3DRS_SRGBWRITEENABLE, &srgbwrite );

	device->SetVertexShader ( nullptr );
	device->SetPixelShader ( nullptr );
	device->SetFVF ( D3DFVF_XYZRHW | D3DFVF_DIFFUSE );

	device->SetRenderState ( D3DRS_LIGHTING, FALSE );
	device->SetRenderState ( D3DRS_FOGENABLE, FALSE );
	device->SetRenderState ( D3DRS_CULLMODE, D3DCULL_NONE );
	device->SetRenderState ( D3DRS_FILLMODE, D3DFILL_SOLID );

	device->SetRenderState ( D3DRS_ZENABLE, D3DZB_FALSE );
	device->SetRenderState ( D3DRS_SCISSORTESTENABLE, TRUE );
	device->SetRenderState ( D3DRS_ZWRITEENABLE, FALSE );
	device->SetRenderState ( D3DRS_STENCILENABLE, FALSE );

	device->SetRenderState ( D3DRS_MULTISAMPLEANTIALIAS, TRUE );
	device->SetRenderState ( D3DRS_ANTIALIASEDLINEENABLE, TRUE );

	device->SetRenderState ( D3DRS_ALPHABLENDENABLE, TRUE );
	device->SetRenderState ( D3DRS_ALPHATESTENABLE, FALSE );
	device->SetRenderState ( D3DRS_SEPARATEALPHABLENDENABLE, TRUE );
	device->SetRenderState ( D3DRS_SRCBLEND, D3DBLEND_SRCALPHA );
	device->SetRenderState ( D3DRS_SRCBLENDALPHA, D3DBLEND_INVDESTALPHA );
	device->SetRenderState ( D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA );
	device->SetRenderState ( D3DRS_DESTBLENDALPHA, D3DBLEND_ONE );

	device->SetRenderState ( D3DRS_SRGBWRITEENABLE, FALSE );
	device->SetRenderState ( D3DRS_COLORWRITEENABLE, D3DCOLORWRITEENABLE_RED | D3DCOLORWRITEENABLE_GREEN | D3DCOLORWRITEENABLE_BLUE | D3DCOLORWRITEENABLE_ALPHA );

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
			anims::resolver::render_impacts ( );
		);

		/* temporary */
		//if ( g::local) {
		//	for ( auto i = 1; i < cs::i::globals->m_max_clients; i++ ) {
		//		auto ent = cs::i::ent_list->get<player_t*> ( i );
		//
		//		if ( !ent || !ent->is_player ( ) || !ent->alive ( ) || !ent->bone_cache ( ) )
		//			continue;
		//
		//		const auto head_pos = ent->bone_cache ( ) [ 8 ].origin ( ) + vec3_t ( 0.0f, 0.0f, 3.0f );
		//		const auto fwd = head_pos - g::local->eyes ( );
		//		const auto right_dir = fwd.normalized().cross_product ( vec3_t ( 0.0f, 0.0f, 1.0f ) );
		//
		//		auto top = head_pos + vec3_t ( 0.0f, 0.0f, 8.0f );
		//		auto right = head_pos + right_dir * 10.0f;
		//		auto left = head_pos - right_dir * 10.0f;
		//
		//		vec3_t srcn_top, srcn_right, srcn_left;
		//
		//		if ( cs::render::world_to_screen ( srcn_top, top ) && cs::render::world_to_screen ( srcn_right, right ) && cs::render::world_to_screen ( srcn_left, left ) ) {
		//			render::line ( srcn_left.x, srcn_left.y, srcn_top.x, srcn_top.y, rgba ( 255, 255, 0, 255 ), 3.0f );
		//			render::line ( srcn_right.x, srcn_right.y, srcn_top.x, srcn_top.y, rgba ( 255, 255, 0, 255 ), 3.0f );
		//			render::polygon ( { srcn_left, srcn_top, srcn_right }, rgba ( 255, 255, 0, 50 ) );
		//			render::circle3d ( head_pos, 10.0f, 32, rgba ( 255, 255, 0, 255 ), true, 3.0f );
		//			render::circle3d ( head_pos, 10.0f, 32, rgba ( 255, 255, 0, 50 ) );
		//		}
		//	}
		//}

		//features::ragebot::scan_points.draw( );

		if ( removals [ 2 ] && g::local && g::local->scoped ( ) ) {
			float w, h;
			render::screen_size ( w, h );

			const auto crosshair_gap = 0.025f * static_cast< float > ( h );

			render::gradient ( w / 2, h / 2 + crosshair_gap, 1, h / 3.3f, rgba ( 255, 251, 237, 150 ), rgba ( 255, 251, 237, 0 ), false );
			render::gradient ( w / 2, h / 2 - crosshair_gap, 1, -h / 3.3f, rgba ( 255, 251, 237, 150 ), rgba ( 255, 251, 237, 0 ), false );
			render::gradient ( w / 2 + crosshair_gap, h / 2, h / 3.3f, 1, rgba ( 255, 251, 237, 150 ), rgba ( 255, 251, 237, 0 ), true );
			render::gradient ( w / 2 - crosshair_gap, h / 2, -h / 3.3f, 1, rgba ( 255, 251, 237, 150 ), rgba ( 255, 251, 237, 0 ), true );
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

	device->SetRenderState ( D3DRS_COLORWRITEENABLE, dwOld_D3DRS_COLORWRITEENABLE );
	device->SetRenderState ( D3DRS_SRGBWRITEENABLE, srgbwrite );

	//cs::i::dev->SetVertexDeclaration ( vertDec );
	//cs::i::dev->SetVertexShader ( vertShader );
	//
	//vertDec->Release ( );
	//vertShader->Release ( );

	pixel_state->Apply ( );
	pixel_state->Release ( );

	//truetype::end ( );

	MUTATE_END

	return old::end_scene( REG_OUT, device );
}