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
    if ( !localplayer || localplayer != g::local || !g::local )
        return old::run_simulation ( REG_OUT, current_command, cmd, localplayer );

    if ( cmd->m_tickcount == INT_MAX ) {
        cmd->m_hasbeenpredicted = true;
        localplayer->tick_base ( )++;
        return;
    }

    if ( exploits::shifted_command ( ) == current_command )
        localplayer->tick_base ( ) -= exploits::shifted_tickbase ( );

    auto curtime = cs::i::globals->m_curtime = cs::ticks2time( localplayer->tick_base ( ) ); 
    __asm movss xmm2, curtime

    old::run_simulation ( REG_OUT, current_command, cmd, localplayer );

    if ( exploits::shifted_command ( ) == current_command )
        localplayer->tick_base ( ) += exploits::shifted_tickbase ( );

    static float last_tick = cs::time2ticks ( cs::i::globals->m_curtime );

    if ( cs::time2ticks ( cs::i::globals->m_curtime ) != last_tick )
        anims::update_anims ( localplayer, lby::in_update ? g::sent_cmd.m_angs : g::angles );

    last_tick = cs::time2ticks ( cs::i::globals->m_curtime );

    features::prediction::fix_viewmodel ( true );
}