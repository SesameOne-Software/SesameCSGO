#pragma once
#include "../sdk/sdk.hpp"

namespace hooks {
    /* cbrs */
    int __fastcall run_simulation ( REG, int current_command, ucmd_t* cmd, player_t* localplayer );

    namespace old {
        extern decltype( &hooks::run_simulation ) run_simulation;
    }
}