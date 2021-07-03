#pragma once
#include "../sdk/sdk.hpp"

namespace features {
	namespace nade_prediction {
		void predict( ucmd_t* ucmd );
		bool detonated ( weapon_t* weapon, float time, trace_t& trace, const vec3_t& vel );
		void trace ( ucmd_t* ucmd );
		void draw ( );
		void draw_beam ( );
	}
}