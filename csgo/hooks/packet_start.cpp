#include "packet_start.hpp"

decltype( &hooks::packet_start ) hooks::old::packet_start = nullptr;

void __fastcall hooks::packet_start ( REG, int incoming, int outgoing ) {
    if ( !g::local || !g::local->alive ( ) || g::outgoing_cmd_nums.empty ( ) )
        return old::packet_start ( REG_OUT, incoming, outgoing );

    for ( auto it = g::outgoing_cmd_nums.begin ( ); it != g::outgoing_cmd_nums.end ( ); it++ ) {
        if ( *it == outgoing ) {
            old::packet_start ( REG_OUT, incoming, outgoing );
            break;
        }
    }

    for ( auto it = g::outgoing_cmd_nums.begin ( ); it != g::outgoing_cmd_nums.end ( ); ) {
        if ( *it <= outgoing || outgoing - 150 > *it )
            it = g::outgoing_cmd_nums.erase ( it );
        else
            it++;
    }
}