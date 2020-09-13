#pragma once
#include "../sdk/sdk.hpp"

namespace hooks {
    void run_simulation_proxy( int current_command, ucmd_t* ucmd, player_t* local, int post_simulation );
    void run_simulation( );

    namespace old {
        extern decltype( &hooks::run_simulation ) run_simulation;
    }
}