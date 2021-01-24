#include "run_simulation.hpp"

#include "../animations/resolver.hpp"

#include "../animations/anims.hpp"
#include "../features/exploits.hpp"
#include "../features/prediction.hpp"

decltype( &hooks::run_simulation ) hooks::old::run_simulation = nullptr;

namespace lby {
    extern bool in_update;
}

/* cbrs */
void __fastcall hooks::run_simulation ( REG, int current_command, ucmd_t* cmd, player_t* localplayer ) {
        if ( !localplayer || localplayer != g::local || !g::local ) {
                return old::run_simulation ( REG_OUT, current_command, cmd, localplayer );
    }
       

    if ( cmd->m_tickcount == INT_MAX ) {
        MUTATE_START
        cmd->m_hasbeenpredicted = true;
        localplayer->tick_base ( )++;
        MUTATE_END
        return;
    }

    MUTATE_START
    if ( exploits::shifted_command ( ) == current_command )
        localplayer->tick_base ( ) -= exploits::shifted_tickbase ( );

    auto curtime = cs::ticks2time( localplayer->tick_base ( ) ); 
    __asm movss xmm2, curtime

    old::run_simulation ( REG_OUT, current_command, cmd, localplayer );

    if ( exploits::shifted_command ( ) == current_command )
        localplayer->tick_base ( ) += exploits::shifted_tickbase ( );

    anims::update_anims ( localplayer, lby::in_update ? g::sent_cmd.m_angs : g::angles );
    features::prediction::fix_viewmodel ( true );
    MUTATE_END
}