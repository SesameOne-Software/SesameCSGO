#include "run_command.hpp"
#include "../globals.hpp"

#include "../features/prediction.hpp"
#include "../animations/anims.hpp"

decltype( &hooks::run_command ) hooks::old::run_command = nullptr;

void __fastcall hooks::run_command( REG, player_t* ent, ucmd_t* ucmd, c_move_helper* move_helper ) {
	old::run_command ( REG_OUT, ent, ucmd, move_helper );
}