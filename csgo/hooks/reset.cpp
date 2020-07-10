#include "reset.hpp"
#include "../globals.hpp"
#include "../menu/menu.hpp"

#include "../features/esp.hpp"
#include "../javascript/js_api.hpp"

decltype( &hooks::reset ) hooks::old::reset = nullptr;

long __fastcall hooks::reset ( REG, IDirect3DDevice9* device, D3DPRESENT_PARAMETERS* presentation_params ) {
	features::esp::esp_font->Release ( );
	features::esp::indicator_font->Release ( );
	features::esp::dbg_font->Release ( );
	features::esp::watermark_font->Release ( );
	js::destroy_fonts ( );

	menu::destroy ( );

	auto hr = old::reset ( REG_OUT, device, presentation_params );

	if ( SUCCEEDED ( hr ) ) {
		render::create_font ( ( void** ) &features::esp::dbg_font, _ ( L"Segoe UI" ), N ( 12 ), false );
		render::create_font ( ( void** ) &features::esp::esp_font, _ ( L"Segoe UI" ), N ( 18 ), false );
		render::create_font ( ( void** ) &features::esp::indicator_font, _ ( L"Segoe UI" ), N ( 14 ), true );
		render::create_font ( ( void** ) &features::esp::watermark_font, _ ( L"Segoe UI" ), N ( 18 ), false );
		menu::reset ( );
		js::reset_fonts ( );
	}

	return hr;
}