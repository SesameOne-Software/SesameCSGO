#pragma once
#include "../sdk/sdk.hpp"

namespace hooks {
	void __fastcall run_command( REG , player_t* ent , ucmd_t* ucmd , c_move_helper* move_helper );

	namespace old {
		extern decltype( &hooks::run_command ) run_command;
	}
}