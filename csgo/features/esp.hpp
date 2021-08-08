#pragma once
#include "../sdk/sdk.hpp"
#include <deque>

namespace features {
	namespace esp {
		inline std::array< std::deque< std::pair< vec3_t , bool > > , 65 > ps_points;

		struct esp_data_t {
			player_t* m_pl = nullptr;
			vec3_t m_pos = vec3_t( 0.0f , 0.0f , 0.0f );
			vec3_t m_sound_pos = vec3_t( 0.0f , 0.0f , 0.0f );
			vec3_t m_radar_pos = vec3_t ( 0.0f, 0.0f, 0.0f );
			float m_health = 0;
			RECT m_box { 0 };
			float m_first_seen = 0.0f, m_last_seen = 0.0f, m_spotted_time = 0.0f;
			bool m_dormant = false , m_fakeducking = false , m_fatal = false , m_reloading = false, m_scoped = false;
			std::string m_weapon_name;
		};

		inline std::array< esp_data_t , 65 > esp_data;

		struct struct_dword18 {
			char gap0 [ 16 ];
			size_t size;
		};

		struct voice_data {
			char gap0 [ 8 ];
			int client;
			char gapC [ 12 ];
			struct_dword18* voice_data;
			int gap1C;
			int dword20;
			int dword24;
			int dword28;
			int dword2C;
			char gap30 [ 4 ];
			int caster_flags;
		};

		void send_data_to_clients ( );
		bool recieve_voice_data ( voice_data* msg );

		void render( );
		void handle_dynamic_updates( );
		void reset_dormancy( event_t* event );
	}
}