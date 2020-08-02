#pragma once
#include <sdk.hpp>
#include "../menu/menu.hpp"

namespace features {
	bool get_visuals ( player_t* pl, oxui::visual_editor::settings_t** out );

	namespace spread_circle {
		extern float total_spread;

		void draw ( );
	}

	namespace offscreen_esp {
		void draw ( );
	}
}