#pragma once
#include "../sdk/sdk.hpp"
#include <deque>

namespace features {
	namespace esp {
		extern std::array< std::deque< std::pair< vec3_t, bool > >, 65 > ps_points;

		struct esp_data_t {
			player_t* m_pl = nullptr;
			vec3_t m_pos = vec3_t( 0.0f, 0.0f, 0.0f );
			float m_health = 0;
			RECT m_box { 0 };
			float m_first_seen = 0.0f, m_last_seen = 0.0f;
			bool m_dormant = false, m_fakeducking = false, m_fatal = false, m_reloading = false;
			std::string m_weapon_name;
		};

		extern std::array< esp_data_t, 65 > esp_data;

		void render( );
		void handle_dynamic_updates ( );
	}
}