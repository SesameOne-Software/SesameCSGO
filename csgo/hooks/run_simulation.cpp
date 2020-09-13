#include "run_simulation.hpp"

#include "../animations/resolver.hpp"

decltype( &hooks::run_simulation ) hooks::old::run_simulation = nullptr;

__declspec( naked ) void hooks::run_simulation( ) {
    __asm {
        /* call proxy func */
        pushad
        pushfd
        push 0
        push [ ebp + 16 ]
        push [ ebp + 12 ]
        push [ ebp + 8 ]
        call hooks::run_simulation_proxy
        popad
        popfd

        /* call original */
        push [ ebp + 16 ]
        push [ ebp + 12 ]
        //mov xmm2, xmm2
        push [ ebp + 8 ]
        //mov ecx, ecx
        call hooks::old::run_simulation

        /* call post proxy func */
        pushad
        pushfd
        push 1
        push [ ebp + 16 ]
        push [ ebp + 12 ]
        push [ ebp + 8 ]
        call hooks::run_simulation_proxy
        popad
        popfd
    }
}

void hooks::run_simulation_proxy( int current_command, ucmd_t* ucmd, player_t* local, int post_simulation ) {
    dbg_print( _( "run_simulation\n" ) );
}