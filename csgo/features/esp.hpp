#pragma once
#include <sdk.hpp>
#include <deque>

namespace features {
	namespace esp {
		extern ID3DXFont* indicator_font;
		extern ID3DXFont* esp_font;
		extern ID3DXFont* dbg_font;

		static std::array< std::deque< std::pair< vec3_t, bool > >, 65 > ps_points;

		struct esp_data_t {
			player_t* m_pl = nullptr;
			vec3_t m_pos = vec3_t( 0.0f, 0.0f, 0.0f );
			RECT m_box { 0 };
			float m_first_seen = 0.0f, m_last_seen = 0.0f;
			bool m_dormant = false;
			std::wstring m_weapon_name;
		};

		static std::array< esp_data_t, 65 > esp_data;

		void draw_esp_box( int x, int y, int w, int h, bool dormant );
		void draw_nametag( int x, int y, int w, int h, const std::wstring_view& name, bool dormant );
		uint32_t color_variable_weight( float val, float cieling );
		void draw_bars( int x, int y, int w, int h, int health_amount, float desync_amount, player_t* pl, bool dormant );
		void render( );
		void handle_dynamic_updates ( );
	}
}