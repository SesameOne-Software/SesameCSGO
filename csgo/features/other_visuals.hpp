#pragma once
#include <sdk.hpp>
#include "../menu/menu.hpp"

namespace features {
	enum esp_placement_t {
		esp_placement_left = 0,
		esp_placement_right,
		esp_placement_bottom,
		esp_placement_top
	};

	struct visual_config_t {
		bool chams;
		bool desync_chams;
		bool desync_chams_fakelag;
		bool desync_chams_rimlight;
		bool chams_flat;
		bool chams_xqz;
		bool backtrack_chams;
		bool hit_matrix;
		bool glow;
		bool rimlight_overlay;
		bool esp_box;
		bool health_bar;
		bool ammo_bar;
		bool desync_bar;
		bool value_text;
		bool nametag;
		bool weapon_name;

		int health_bar_placement;
		int ammo_bar_placement;
		int desync_bar_placement;
		int value_text_placement;
		int nametag_placement;
		int weapon_name_placement;

		float reflectivity;
		float phong;

		sesui::color chams_color;
		sesui::color chams_xqz_color;
		sesui::color backtrack_chams_color;
		sesui::color hit_matrix_color;
		sesui::color glow_color;
		sesui::color rimlight_color;
		sesui::color box_color;
		sesui::color health_bar_color;
		sesui::color ammo_bar_color;
		sesui::color desync_bar_color;
		sesui::color name_color;
		sesui::color weapon_color;
		sesui::color desync_chams_color;
		sesui::color desync_rimlight_color;
	};

	bool get_visuals ( player_t* pl, visual_config_t& out );

	namespace spread_circle {
		extern float total_spread;

		void draw ( );
	}

	namespace offscreen_esp {
		void draw ( );
	}
}