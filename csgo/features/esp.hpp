#pragma once
#include <sdk.hpp>

namespace features {
	namespace esp {
		extern ID3DXFont* indicator_font;
		extern ID3DXFont* esp_font;

		void draw_esp_box( int x, int y, int w, int h );
		void draw_nametag( int x, int y, int w, int h, const std::wstring_view& name );
		void draw_bars( int x, int y, int w, int h, int health_amount, float desync_amount, player_t* pl );
		void render( );
	}
}