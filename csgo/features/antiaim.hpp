#pragma once
#include "../sdk/sdk.hpp"

namespace features {
	namespace antiaim {
		inline int side = -1;
		inline int desync_side = -1;

		extern bool antiaiming;

		void simulate_lby( );
		void fakelag ( ucmd_t* ucmd, player_t* target );
		void run( ucmd_t* ucmd, bool was_shooting );
	}
}