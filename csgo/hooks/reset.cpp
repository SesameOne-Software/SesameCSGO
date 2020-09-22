#include "reset.hpp"
#include "../globals.hpp"
#include "../menu/menu.hpp"

#include "../features/esp.hpp"
#include "../javascript/js_api.hpp"

#include "../menu/sesui_custom.hpp"

#include "../renderer/font.hpp"

decltype( &hooks::reset ) hooks::old::reset = nullptr;

long __fastcall hooks::reset( REG, IDirect3DDevice9* device, D3DPRESENT_PARAMETERS* presentation_params ) {
	features::esp::esp_font->Release( );
	features::esp::indicator_font->Release( );
	features::esp::dbg_font->Release( );
	features::esp::watermark_font->Release( );
	js::destroy_fonts( );

	if ( sesui::custom::tab_font.data ) {
		reinterpret_cast< ID3DXFont* > ( sesui::custom::tab_font.data )->Release( );
		sesui::custom::tab_font.data = nullptr;
	}

	if ( sesui::style.control_font.data ) {
		reinterpret_cast< ID3DXFont* > ( sesui::style.control_font.data )->Release( );
		sesui::style.control_font.data = nullptr;
	}

	if ( sesui::style.tab_font.data ) {
		reinterpret_cast< ID3DXFont* > ( sesui::style.tab_font.data )->Release( );
		sesui::style.tab_font.data = nullptr;
	}

	truetype::end ( );

	auto hr = old::reset( REG_OUT, device, presentation_params );

	if ( SUCCEEDED( hr ) ) {
		render::create_font( ( void** )&features::esp::dbg_font, _( L"sesame_ui" ), N( 12 ), false );
		render::create_font( ( void** )&features::esp::esp_font, _( L"sesame_ui" ), N( 15 ), false );
		render::create_font( ( void** )&features::esp::indicator_font, _( L"sesame_ui" ), N( 32 ), false );
		render::create_font( ( void** )&features::esp::watermark_font, _( L"sesame_ui" ), N( 18 ), false );
		
		truetype::begin ( );

		js::reset_fonts( );
	}

	return hr;
}