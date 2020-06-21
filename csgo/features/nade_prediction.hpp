#pragma once
#include <sdk.hpp>

namespace features {
	namespace nade_prediction {
		void predict( ucmd_t* ucmd );
		bool detonated ( weapon_t* weapon, float time, trace_t& trace );
		void trace ( ucmd_t* ucmd );
		void draw ( );
	}
}