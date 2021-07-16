#pragma once
#include "../sdk/sdk.hpp"

namespace features {
	namespace movement {
		void directional_strafer ( ucmd_t* cmd, vec3_t& old_angs );
		void run( ucmd_t* ucmd, vec3_t& old_angs );
	}
}