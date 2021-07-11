#include "packet_start.hpp"

decltype( &hooks::packet_start ) hooks::old::packet_start = nullptr;

void __fastcall hooks::packet_start ( REG, int incoming, int outgoing ) {
    if ( !g::local || !g::local->alive( ) )
        return old::packet_start ( REG_OUT, incoming, outgoing );

    old::packet_start ( REG_OUT, incoming, outgoing );

    auto& outgoing_data = g::network_data [ outgoing % g::network_data.size( ) ];
    
    if ( outgoing_data.out_sequence == outgoing )
        outgoing = outgoing_data.last_out_cmd;

    cs::i::client_state->cur_seq ( ) = incoming;
    cs::i::client_state->last_command_ack ( ) = outgoing;
}