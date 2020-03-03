#pragma once
#include <sdk.hpp>

namespace features {
	namespace ragebot {
		void hitscan( player_t* pl, vec3_t& point, float& dmg );
		void run( ucmd_t* ucmd );
	}
}