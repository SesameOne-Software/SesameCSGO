#pragma once
#include "../sdk/sdk.hpp"

namespace features {
	namespace antiaim {
		static int side = -1;
		static int desync_side = -1;
		extern bool antiaiming;

		void simulate_lby( );
		void fakelag ( ucmd_t* ucmd, player_t* target );
		void run( ucmd_t* ucmd, float& old_smove, float& old_fmove, bool was_shooting );
	}
}